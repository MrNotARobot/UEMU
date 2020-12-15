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
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "basicmmu.h"
#include "memory.h"
#include "system.h"

static void B_mmu_set_error(BasicMMU *, int, const char *, ...);
static void *translate_virtaddr(BasicMMU *, addr_t);
uint64_t readx(BasicMMU *, addr_t, int);


static int conf_b_mmu_initial_npages = 4;
static int conf_b_mmu_page_alloc_incr = 2;
static addr_t conf_b_mmu_top_stack_address = 0xfffff000;
static size_t conf_b_mmu_stack_size = 4 * 4096;


static void B_mmu_set_error(BasicMMU *b_mmu, int errnum, const char *fmt, ...)
{
    va_list ap;

    // NOTE: do we isolate any dangerous format specifier? Or is this safe
    // enough as this will only be called in this file anyway

    b_mmu->err.errnum = errnum;
    va_start(ap, fmt);
    vsnprintf(b_mmu->err.description, ERROR_DESCRIPTION_MAX_SIZE, fmt, ap);
    va_end(ap);
}


/*
 * translates the virtaddr to an actual address in memory
 */
static void *translate_virtaddr(BasicMMU *b_mmu, addr_t virtaddr)
{
    table_record_t *record = NULL;
    void *buffer = NULL;
    addr_t offset;

    for (size_t i = 0; i < b_mmu->max_records; i++) {
        record = &b_mmu->records[i];
        if (!record->c_inuse) {
            record = NULL;
            continue;
        }

        if ((record->c_virtaddr & virtaddr) == record->c_virtaddr)
            break;
    }

    if (record) {
        offset = (~record->c_virtaddr) & virtaddr;
        buffer = record->c_buffer + offset;

        // check if page are in-use. if not then its a invalid address
        for (size_t k = 0; k < record->c_npages; k++) {
            if ((addr_t)buffer < record->c_pages[k].p_end) {
                if (!record->c_pages[k].p_inuse)
                    buffer = NULL;
                break;
            }
        }

    }

    return buffer;
}

void b_mmu_init(BasicMMU *b_mmu)
{
    ASSERT(b_mmu != NULL);

    b_mmu->max_records = conf_b_mmu_initial_npages;
    b_mmu->records = xcalloc(b_mmu->max_records, sizeof(*b_mmu->records));

    B_mmu_set_error(b_mmu, 0, NULL);
}

void b_mmu_release_recs(BasicMMU *b_mmu)
{
    table_record_t *record;

    for (size_t i = 0; i < b_mmu->max_records; i++) {
        record = &b_mmu->records[i];
        if (record->c_inuse) {
            xfree(record->c_pages);
            if (munmap(record->c_buffer, record->c_memsz) == -1)
                B_mmu_set_error(b_mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
        }
    }

    xfree(b_mmu->records);
}

addr_t b_mmu_mmap(BasicMMU *b_mmu, addr_t virtaddr, size_t memsz, int prot, int fd, off_t offset, size_t filesz)
{
    ASSERT(b_mmu != NULL);

    const int pagesize = sysconf(_SC_PAGESIZE);
    void *buffer;
    int perms = 0;
    table_record_t *record = NULL;

    if (memsz == 0)
        return 0;

    if (fd != -1) {
        if (filesz == 0 || filesz > memsz)
        return 0;
    }

    // rounding up the size of the mapping to page size
    // it will be easier for us to keep track of all pages this way
    memsz = (memsz + pagesize - 1) & ~(pagesize - 1);

    // offset not page-aligned. thats a big no-no.
    if ((offset % pagesize) != 0)
        return 0;

    buffer = mmap(NULL, memsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        B_mmu_set_error(b_mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
        return 0;
    }

    // TODO: give a random virt address here. Prevent the user from acessing
    // the underlying buffer directly
    if (virtaddr == 0)
        virtaddr = (addr_t) buffer;


    if (fd != -1) {
        if (pread(fd, buffer, filesz, offset) == -1) {
            B_mmu_set_error(b_mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
            return 0;
        }
    }

    // set the actual protection requested
    mprotect(buffer, memsz, prot);

    if (prot & PROT_READ)
        perms |= PAGEREAD;
    if (prot & PROT_WRITE)
        perms |= PAGEWRITE;
    if (prot & PROT_EXEC)
        perms |= PAGEEXEC;

    // search for a unused record
    for (size_t i = 0; i < b_mmu->max_records; i++) {
        if (!b_mmu->records[i].c_inuse) {
            record = &b_mmu->records[i];
            break;
        }
    }

    // no unused record found. allocate space for more
    if (!record) {
        ASSERT(conf_b_mmu_page_alloc_incr > 0);

        b_mmu->records = xreallocarray(b_mmu->records,
                                        b_mmu->max_records + conf_b_mmu_page_alloc_incr,
                                        sizeof(*b_mmu->records));
        record = &b_mmu->records[b_mmu->max_records];
        b_mmu->max_records += conf_b_mmu_page_alloc_incr;
    }

    record->c_inuse = 1;

    // page-align address if not already aligned
    virtaddr = virtaddr & ~(pagesize - 1);
    record->c_virtaddr = virtaddr;
    record->c_buffer = buffer;
    record->c_perms = perms;
    record->c_memsz = memsz;

    record->c_npages = memsz / pagesize;
    record->c_pages = xcalloc(record->c_npages, sizeof(page_info_t));
    for (size_t i = 0, k = 0; i < memsz; i += pagesize, k++) {
        record->c_pages[k].p_addr = (addr_t) buffer + i;
        record->c_pages[k].p_addr = (addr_t) buffer + i + pagesize;
        record->c_pages[k].p_inuse = 1;
    }

    return virtaddr;
}


void b_mmu_munmap(BasicMMU *b_mmu, addr_t addr, size_t length)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record = NULL;
    addr_t offset;
    int full_unmapped;

    for (size_t i = 0; i < b_mmu->max_records; i++) {
        if (b_mmu->records[i].c_inuse && b_mmu->records[i].c_virtaddr == addr) {
            record = &b_mmu->records[i];
            break;
            // addr might not be the start address of the segment but a page inside the segment
        } else if (addr > b_mmu->records[i].c_virtaddr  &&
                addr <= (b_mmu->records[i].c_virtaddr + b_mmu->records[i].c_memsz)) {
            record = &b_mmu->records[i];
            break;
        }
    }

    // according to munmap(2) is not a error if there is no mapped address
    // I think is reasonable as it doesn't affect program state at all
    if (!record)
        return;

    if (munmap(record->c_buffer, length) == -1) {
        B_mmu_set_error(b_mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
        return;
    }

    // invalidate the pages in the record so that we catch invalid reads/writes
    ASSERT(record->c_npages > 0);
    offset = (~record->c_virtaddr & addr) + length;

    // set any page that is in the range as invalid
    for (int i = record->c_npages - 1; i >= 0; i--) {
        page_info_t page = record->c_pages[i];

        if ( ((addr_t)record->c_buffer + offset) >= page.p_addr )
            record[i].c_pages->p_inuse = 0;
    }

    // check if the entire segment was unmapped
    full_unmapped = 1;
    for (int i = record->c_npages - 1; i >= 0; i--) {
        if (record->c_pages[i].p_inuse) {
            full_unmapped = 0;
            break;
        }
    }

    // no in-use page is left, clean the table record
    if (full_unmapped) {
        // record->c_buffer is already gone with munmap(2)
        xfree(record->c_pages);
        memset(record, 0, sizeof(*record));
    }
}

/*
 * Setup the basic execution environment by mmap'ing the segments to memory.
 */
void b_mmu_mmap_loadable(GenericELF *g_elf, BasicMMU *b_mmu)
{
    ASSERT(b_mmu != 0);
    struct loadable_segment segment;
    int prot = 0;

    if (!g_elf) {
        B_mmu_set_error(b_mmu, INVALREF, "emulator: invalid reference to emulated MMU");
        return;
    }

    for (size_t i = 0; i < G_elf_nloadable(g_elf); i++) {
        segment = G_elf_loadable(g_elf)[i];

        if (segment.s_perms & PF_R)
            prot |= PROT_READ;
        if (segment.s_perms & PF_W)
            prot |= PROT_WRITE;
        if (segment.s_perms & PF_X)
            prot |= PROT_EXEC;

        b_mmu_mmap(b_mmu, segment.s_vaddr, segment.s_memsz, prot,
                    G_elf_underlfd(g_elf), segment.s_offset, segment.s_filesz);

        if (B_mmu_error(b_mmu))
            return;
    }
}

addr_t b_mmu_create_stack(BasicMMU *b_mmu, int flags)
{
    ASSERT(b_mmu != NULL);

    addr_t stack;
    int prot = PROT_READ | PROT_WRITE;
    addr_t virtaddr = conf_b_mmu_top_stack_address;

    if (flags & B_STACKEXEC)
        prot |= PROT_EXEC;

    stack = b_mmu_mmap(b_mmu, virtaddr, conf_b_mmu_stack_size, prot, -1, 0, 0);
    if (B_mmu_error(b_mmu))
        return 0;

    return stack + conf_b_mmu_stack_size;
}

// Read/Write functions

uint64_t readx(BasicMMU *b_mmu, addr_t virtaddr, int size)
{
    ASSERT(b_mmu != NULL);
    void *buffer;

    u_int64_t bytes = 0;

    buffer = translate_virtaddr(b_mmu, virtaddr);
    if (!buffer) {
        B_mmu_set_error(b_mmu, ESEGFAULT, "emulator: invalid memory read at 0x%lx", virtaddr);
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

uint8_t b_mmu_read8(BasicMMU *b_mmu, addr_t virtaddr)
{
    return readx(b_mmu, virtaddr, 8);
}

uint16_t b_mmu_read16(BasicMMU *b_mmu, addr_t virtaddr)
{
    return  readx(b_mmu, virtaddr, 16);
}

uint32_t b_mmu_read32(BasicMMU *b_mmu, addr_t virtaddr)
{
    return  readx(b_mmu, virtaddr, 32);
}

uint64_t b_mmu_read64(BasicMMU *b_mmu, addr_t virtaddr)
{
    return  readx(b_mmu, virtaddr, 64);
}


void writex(BasicMMU *b_mmu, uint64_t bytes, addr_t virtaddr, int size)
{
    ASSERT(b_mmu != NULL);
    void *buffer = translate_virtaddr(b_mmu, virtaddr);

    if (!buffer) {
        B_mmu_set_error(b_mmu, ESEGFAULT, "emulator: invalid memory read at %ld", virtaddr);
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

void b_mmu_write8(BasicMMU *b_mmu, uint8_t byte, addr_t virtaddr)
{
    writex(b_mmu, byte, virtaddr, 8);
}

void b_mmu_write16(BasicMMU *b_mmu, uint16_t byte, addr_t virtaddr)
{
    writex(b_mmu, byte, virtaddr, 16);
}

void b_mmu_write32(BasicMMU *b_mmu, uint32_t byte, addr_t virtaddr)
{
    writex(b_mmu, byte, virtaddr, 32);
}

void b_mmu_write64(BasicMMU *b_mmu, uint64_t byte, addr_t virtaddr)
{
    writex(b_mmu, byte, virtaddr, 64);
}
