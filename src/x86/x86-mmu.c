/* Copyright (c) 2020 Gabriel Manoel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * DESCRIPTION:
 *  simulate memory management.
 */
#include <stdio.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "x86-mmu.h"
#include "../memory.h"

static void mmu_set_error(x86MMU *, int, const char *, ...);

static _Bool mmu_iswritable(x86MMU *, moffset32_t);
static _Bool mmu_isreadable(x86MMU *, moffset32_t);
static _Bool mmu_isexecutable(x86MMU *, moffset32_t);

enum x86MMUMmapFlags {
    MF_STACK = 1
};

enum x86MMUQueries {
    SD_LIMIT,
    SD_BASE,
    SD_TYPE
};

uint64_t mmu_query(const x86MMU *, moffset32_t, int);

static void *mmu_mmap(x86MMU *, moffset32_t, size_t, int, int, int, off_t, size_t);

static void *translate(x86MMU *, moffset32_t);
static uint64_t readx(x86MMU *, moffset32_t, int);

static size_t conf_mmu_pagesize = 0;
static moffset32_t conf_mmu_start_mapping_address = 0x08045000;
static moffset32_t conf_mmu_top_stack_address = 0x7fff0000;
static size_t conf_mmu_stack_size = 4 * 4096;

#define STACK_MASK 0x7f000000


static void mmu_set_error(x86MMU *mmu, int errnum, const char *fmt, ...)
{
    va_list ap;

    // NOTE: do we isolate any dangerous format specifier? Or is this safe
    // enough as this will only be called in this file anyway

    mmu->err.errnum = errnum;
    va_start(ap, fmt);
    vsnprintf(mmu->err.description, ERROR_DESCRIPTION_MAX_SIZE, fmt, ap);
    va_end(ap);
}

#define base(addr) (addr & 0xfffff000)
inline static void *translate(x86MMU *mmu, moffset32_t virtaddr)
{
    if ((virtaddr & STACK_MASK) == STACK_MASK) {
        if (virtaddr >= mmu->mm_stack->s_start && virtaddr < mmu->mm_stack->s_limit)
            return mmu->mm_stack->buffer_ + (virtaddr - mmu->mm_stack->s_start);
        return NULL;
    }

    for (size_t i = 0; i < mmu->mm_segments; i++) {
        if (virtaddr >= mmu->mm_segment_tbl[i].s_start && virtaddr < mmu->mm_segment_tbl[i].s_limit)
            return mmu->mm_segment_tbl[i].buffer_ + (virtaddr - mmu->mm_segment_tbl[i].s_start);
    }

    return NULL;
}

//
// Initialization/cleanup
//

void mmu_init(x86MMU *mmu)
{
    if (!mmu)
        return;

    conf_mmu_pagesize = sysconf(_SC_PAGESIZE);

    mmu->mm_segment_tbl = NULL;
    mmu->mm_segments = 0;
    mmu_set_error(mmu, 0, NULL);
}

void mmu_unloadall(x86MMU *mmu)
{
    if (!mmu)
        return;

    for (size_t i = 0; i < mmu->mm_segments; i++) {
        size_t size = mmu->mm_segment_tbl[i].s_limit - mmu->mm_segment_tbl[i].s_start;
        munmap(mmu->mm_segment_tbl[i].buffer_, size);
    }

    xfree(mmu->mm_segment_tbl);
}


//
//  Information Query
//

uint64_t mmu_query(const x86MMU *mmu, moffset32_t virtaddr, int query)
{
    const segment_t *segment = NULL;
    if (!mmu)
        return 0;

    for (size_t i = 0; i < mmu->mm_segments; i++) {
        if (virtaddr >= mmu->mm_segment_tbl[i].s_start && virtaddr < mmu->mm_segment_tbl[i].s_limit) {
            segment = &mmu->mm_segment_tbl[i];
            break;
        }
    }

    if (!segment)
        return 0;

    switch (query) {
        case SD_LIMIT: return segment->s_limit;
        case SD_BASE:  return segment->s_start;
        case SD_TYPE:  return segment->s_type;
        default: return 0;
    }
}


inline static _Bool mmu_iswritable(x86MMU *mmu, moffset32_t virtaddr)
{
    int type = mmu_query(mmu, virtaddr, SD_TYPE);
    return type != ST_RODATA && type != ST_XOCODE;
}

inline static _Bool mmu_isreadable(x86MMU *mmu, moffset32_t virtaddr)
{
    int type = mmu_query(mmu, virtaddr, SD_TYPE);
    return type != ST_XOCODE;
}

inline static _Bool mmu_isexecutable(x86MMU *mmu, moffset32_t virtaddr)
{
    int type = mmu_query(mmu, virtaddr, SD_TYPE);
    return type == ST_XOCODE || type == ST_RXCODE || type == ST_RWXCODE || type == ST_RWXSTACK;
}

//
//  Segment creation
//


static void *mmu_mmap(x86MMU *mmu, moffset32_t virtaddr, size_t memsz, int prot, int flags, int fd, off_t offset, size_t filesz)
{
    void *buffer;
    int type;

    if (!mmu || memsz == 0)
        return 0;

    if (fd != -1) {
        if (filesz == 0 || filesz > memsz)
        return 0;
    }


    // rounding up the size of the mapping to a multiple of page size
    // it will be easier for us to keep track of all pages this way
    memsz = (memsz + conf_mmu_pagesize - 1) & ~(conf_mmu_pagesize - 1);

    buffer = mmap(NULL, memsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        mmu_set_error(mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
        return 0;
    }

    // TODO: give a random virtual address here. Prevent the user from acessing
    // the underlying buffer directly
    if (virtaddr == 0) {
        virtaddr = conf_mmu_start_mapping_address;
        conf_mmu_start_mapping_address += memsz;
    } else {
        conf_mmu_start_mapping_address = virtaddr + memsz;
    }


    if (fd != -1) {
        if (pread(fd, buffer, filesz, offset) == -1) {
            mmu_set_error(mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
            return 0;
        }
    }
    // set the actual protection requested
    //mprotect(buffer, memsz, prot);

    if ((prot & PROT_READ) && !(prot & PROT_WRITE) && !(prot & PROT_EXEC) && !(flags & MF_STACK))
        type = ST_RODATA;
    else if ((prot & PROT_READ) && (prot & PROT_WRITE) && !(prot & PROT_EXEC) && !(flags & MF_STACK))
        type = ST_RWDATA;
    else if ((prot & PROT_READ) && !(prot & PROT_WRITE) && (prot & PROT_EXEC) && !(flags & MF_STACK))
        type = ST_RXCODE;
    else if ((prot & PROT_READ) && (prot & PROT_WRITE) && (prot & PROT_EXEC) && !(flags & MF_STACK))
        type = ST_RWXCODE;
    else if ((prot & PROT_READ) && (prot & PROT_WRITE) && !(prot & PROT_EXEC) && (flags & MF_STACK))
        type = ST_RWSTACK;
    else
        type = ST_RWXSTACK;

    mmu->mm_segment_tbl = xreallocarray(mmu->mm_segment_tbl, ++mmu->mm_segments, sizeof(*mmu->mm_segment_tbl));

    mmu->mm_segment_tbl[mmu->mm_segments-1].buffer_ = buffer;
    mmu->mm_segment_tbl[mmu->mm_segments-1].s_limit = virtaddr + memsz;
    mmu->mm_segment_tbl[mmu->mm_segments-1].s_start = virtaddr;
    mmu->mm_segment_tbl[mmu->mm_segments-1].s_type = type;

    if (flags & MF_STACK)
        mmu->mm_stack = &mmu->mm_segment_tbl[mmu->mm_segments-1];

    return buffer;
}


void mmu_mmap_loadable(x86MMU *mmu, GenericELF *elf)
{
    pt_load_segment_t *segment;
    int prot = 0;

    if (!mmu || !elf)
        return;

    for (size_t i = 0; i < elf_nloadable(elf); i++, prot = 0) {
        segment = &elf_loadable(elf)[i];

        if (segment->pt_flags & PF_R)
            prot |= PROT_READ;
        if (segment->pt_flags & PF_W)
            prot |= PROT_WRITE;
        if (segment->pt_flags & PF_X)
            prot |= PROT_EXEC;

        mmu_mmap(mmu, segment->pt_vaddr, segment->pt_memsz, prot, 0,
                    elf_underlfd(elf), segment->pt_offset, segment->pt_filesz);

        if (mmu_error(mmu))
            return;
    }
}

moffset32_t mmu_create_stack(x86MMU *mmu, int flags)
{
    moffset32_t addr;
    int prot = PROT_READ | PROT_WRITE;

    if (!mmu)
        return 0;

    if (flags & B_STACKEXEC)
        prot |= PROT_EXEC;

    mmu_mmap(mmu, conf_mmu_top_stack_address, conf_mmu_stack_size, prot, MF_STACK, -1, 0, 0);

    addr = conf_mmu_top_stack_address + conf_mmu_stack_size;

    if (mmu_error(mmu))
        return 0;

    return addr;
}


//
// Read/Write functions
//

uint8_t mmu_fetch(x86MMU *mmu, moffset32_t virtaddr)
{
    void *buffer;

    if (!mmu)
        return 0;

    buffer = translate(mmu, virtaddr);
    if (!buffer) {
        mmu_set_error(mmu, ESEGFAULT, "Segmentation Fault at 0x%lx", virtaddr);
        return 0;
    }

    if (!mmu_isexecutable(mmu, virtaddr)) {
        mmu_set_error(mmu, EPROT, "attempted to execute code from a non-executable segment at 0x%lx", virtaddr);
        return 0;
    }


    return *(uint8_t *)buffer;
}


uint64_t readx(x86MMU *mmu, moffset32_t virtaddr, int size)
{
    void *buffer;
    u_int64_t bytes = 0;

    if (!mmu)
        return 0;

    buffer = translate(mmu, virtaddr);

    if (!buffer) {
        mmu_set_error(mmu, ESEGFAULT, "Segmentation Fault at 0x%lx", virtaddr);
        return 0;
    }

    if (!mmu_isreadable(mmu, virtaddr)) {
        mmu_set_error(mmu, EPROT, "attempted read at non-readable segment at 0x%lx", virtaddr);
        return 0;
    }

    switch (size) {
        case 8:
            bytes = *((uint8_t *)buffer);
            break;
        case 16:
            bytes = *((uint16_t *)buffer);
            break;
        case 32:
            bytes = *((uint32_t *)buffer);
            break;
        case 64:
            bytes = *((uint64_t *)buffer);
            break;
        default:
            break;
    }

    return bytes;
}

uint8_t mmu_read8(x86MMU *mmu, moffset32_t virtaddr)
{
    return readx(mmu, virtaddr, 8);
}

uint16_t mmu_read16(x86MMU *mmu, moffset32_t virtaddr)
{
    return  readx(mmu, virtaddr, 16);
}

uint32_t mmu_read32(x86MMU *mmu, moffset32_t virtaddr)
{
    return  readx(mmu, virtaddr, 32);
}

uint64_t mmu_read64(x86MMU *mmu, moffset32_t virtaddr)
{
    return  readx(mmu, virtaddr, 64);
}


// write

void writex(x86MMU *mmu, uint64_t bytes, moffset32_t virtaddr, int size)
{
    void *buffer = NULL;

    if (!mmu)
        return;

    buffer = translate(mmu, virtaddr);
    if (!buffer) {
        mmu_set_error(mmu, ESEGFAULT, "Segmentation Fault at 0x%lx", virtaddr);
        return;
    }
    if (!mmu_iswritable(mmu, virtaddr)) {
        mmu_set_error(mmu, EPROT, "attempted write at non-writable segment at 0x%lx", virtaddr);
        return;
    }

    switch (size) {
        case 8:
            *((uint8_t *)buffer) = bytes;
            break;
        case 16:
            *((uint16_t *)buffer) = bytes;
            break;
        case 32:
            *((uint32_t *)buffer) = bytes;
            break;
        case 64:
            *((uint64_t *)buffer) = bytes;
            break;
        default:
            break;
    }
}

void mmu_write8(x86MMU *mmu, uint8_t byte, moffset32_t virtaddr)
{
    writex(mmu, byte, virtaddr, 8);
}

void mmu_write16(x86MMU *mmu, uint16_t byte, moffset32_t virtaddr)
{
    writex(mmu, byte, virtaddr, 16);
}

void mmu_write32(x86MMU *mmu, uint32_t byte, moffset32_t virtaddr)
{
    writex(mmu, byte, virtaddr, 32);
}

void mmu_write64(x86MMU *mmu, uint64_t byte, moffset32_t virtaddr)
{
    writex(mmu, byte, virtaddr, 64);
}

inline const uint8_t *mmu_getptr(x86MMU *mmu, moffset32_t virtaddr)
{
    if (!mmu)
        return NULL;
    return (const uint8_t *)translate(mmu, virtaddr);
}


inline int mmu_ptrtype(x86MMU *mmu, moffset32_t virtaddr)
{
    if (!mmu)
        return 0;
    return mmu_query(mmu, virtaddr, SD_TYPE);
}
