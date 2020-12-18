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
#include "x86/disassembler.h"

static void B_mmu_set_error(BasicMMU *, int, const char *, ...);
static table_record_t *lookup(BasicMMU*, addr_t);
static void *translate_virtaddr(BasicMMU *, addr_t, uint8_t);
static uint64_t readx(BasicMMU *, addr_t, int);
static void map_fuctions2range(BasicMMU *, GenericELF *);

static int conf_b_mmu_initial_npages = 4;
static int conf_b_mmu_page_alloc_incr = 2;
static addr_t conf_b_mmu_top_stack_address = 0xffffa000;
static size_t conf_b_mmu_stack_size = 4 * 4096;
static size_t conf_b_mmu_fmap_shortcut_threshold = 200;
static size_t conf_b_mmu_fmap_cache_size = 20;


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

static table_record_t *lookup(BasicMMU *b_mmu, addr_t virtaddr)
{
    table_record_t *record = NULL;
    // find the record with our address
    for (size_t i = 0; i < b_mmu->max_records; i++) {
        record = &b_mmu->records[i];
        if (!record->c_inuse) {
            record = NULL;
            continue;
        }

        if (virtaddr >= record->c_virtaddr && virtaddr < record->c_virtaddr_end)
            break;
        record = NULL;
    }

    return record;
}


/*
 * translates the virtaddr to an actual address in memory
 */
static void *translate_virtaddr(BasicMMU *b_mmu, addr_t virtaddr, uint8_t size)
{
    table_record_t *record = lookup(b_mmu, virtaddr);
    void *buffer = NULL;
    addr_t offset;

    if (record) {
        offset = virtaddr - record->c_virtaddr;
        buffer = record->c_buffer + offset;


        if (record->c_pages[record->c_npages - 1].p_end < (addr_t)buffer + size)
            return NULL;

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

    b_mmu->fmap_cache = xcalloc(conf_b_mmu_fmap_cache_size, sizeof(func_rangemap_t));
    b_mmu->fmap_cachewridx = 0;
    B_mmu_set_error(b_mmu, 0, NULL);
}

void b_mmu_release_recs(BasicMMU *b_mmu)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record;

    for (size_t i = 0; i < b_mmu->max_records; i++) {
        record = &b_mmu->records[i];
        xfree(record->c_fmap);
        if (record->c_inuse) {
            xfree(record->c_pages);
            if (munmap(record->c_buffer, record->c_memsz) == -1)
                B_mmu_set_error(b_mmu, errno, "%s: %s", __FUNCTION__, strerror(errno));
        }
    }

    xfree(b_mmu->fmap_cache);
    xfree(b_mmu->records);
}


_Bool b_mmu_iswritable(BasicMMU *b_mmu, addr_t virtaddr)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record = lookup(b_mmu, virtaddr);

    if (record)
        return record->c_perms & PAGEWRITE;
    return 0;
}

_Bool b_mmu_isreadable(BasicMMU *b_mmu, addr_t virtaddr)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record = lookup(b_mmu, virtaddr);

    if (record)
        return record->c_perms & PAGEREAD;
    return 0;
}

_Bool b_mmu_isexecutable(BasicMMU *b_mmu, addr_t virtaddr)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record = lookup(b_mmu, virtaddr);

    if (record)
        return record->c_perms & PAGEEXEC;
    return 0;
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
        perms |= PAGEWRITE | PAGEREAD;
    if (prot & PROT_EXEC)
        perms |= PAGEEXEC | PAGEREAD;

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
    record->c_virtaddr_end = virtaddr + memsz;
    record->c_buffer = buffer;
    record->c_perms = perms;
    record->c_memsz = memsz;

    record->c_npages = memsz / pagesize;
    record->c_pages = xcalloc(record->c_npages, sizeof(page_info_t));
    for (size_t i = 0, k = 0; i < memsz; i += pagesize, k++) {
        record->c_pages[k].p_addr = (addr_t) buffer + i;
        record->c_pages[k].p_end = (addr_t) buffer + i + pagesize;
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

    record->c_fmap = NULL;
    record->c_fmapsz = 0;
}

/*
 * Setup the basic execution environment by mmap'ing the segments to memory.
 */
void b_mmu_mmap_loadable(BasicMMU *b_mmu, GenericELF *g_elf)
{
    ASSERT(b_mmu != 0);
    struct loadable_segment segment;
    int prot = 0;

    if (!g_elf) {
        B_mmu_set_error(b_mmu, INVALREF, "invalid reference to emulated MMU");
        return;
    }

    for (size_t i = 0; i < G_elf_nloadable(g_elf); i++, prot = 0) {
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

    map_fuctions2range(b_mmu, g_elf);
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

    return (stack + conf_b_mmu_stack_size - 4) & 0xfffffff0;
}

static void map_fuctions2range(BasicMMU *b_mmu, GenericELF *g_elf)
{
    ASSERT(b_mmu != NULL);
    ELF_Sym sym;
    table_record_t *record;
    size_t recidx;

    for (size_t i = 1; i < G_elf_symtabsz(g_elf); i++) {
        sym = g_elf_getsym(g_elf, i);
        record = lookup(b_mmu, sym.s_value);

        if (record) {
            if (!record->c_fmap) {
                record->c_fmap = xcalloc(1, sizeof(func_rangemap_t));
                record->c_fmapsz = 1;
                record->c_fmap[0].fr_name = 0;   // points to NULL
                record->c_fmap[0].fr_start = 0;
                record->c_fmap[0].fr_end = 0;
            }

            recidx = ++record->c_fmapsz;
            record->c_fmap = xreallocarray(record->c_fmap, recidx, sizeof(func_rangemap_t));
            record->c_fmap[recidx-1].fr_name = i;
            record->c_fmap[recidx-1].fr_start = sym.s_value;
            record->c_fmap[recidx-1].fr_end = 0;
            // defer finding the end address to when we need
        }
    }
}

const func_rangemap_t *b_mmu_findfunction(BasicMMU *b_mmu, addr_t virtaddr)
{
    ASSERT(b_mmu != NULL);
    table_record_t *record = lookup(b_mmu, virtaddr);
    addr_t closest = 0;
    func_rangemap_t *closest_func = NULL;
    struct instruction last_instr;

    if (record) {
        if (!record->c_fmap)
            return NULL;

        // check our cache for recently entered functions
        // is guaranteed that the cache will always have this size
        for (size_t i = 0; i < conf_b_mmu_fmap_cache_size; i++) {
            if (virtaddr >= b_mmu->fmap_cache[i].fr_start && virtaddr <= b_mmu->fmap_cache[i].fr_end)
                return &b_mmu->fmap_cache[i];
        }

        // if the end address was already calculate then we
        // can simply compare if the addres is within bounds
        for (size_t i = 1; i < record->c_fmapsz; i++) {
            if (record->c_fmap[i].fr_start == virtaddr) {
                // save the end of the function
                for (size_t k = 1; k < record->c_fmapsz; k++) {
                    if (!closest && record->c_fmap[k].fr_start > record->c_fmap[i].fr_start) {
                        closest = record->c_fmap[k].fr_start;
                        continue;
                    }

                    if (record->c_fmap[k].fr_start > record->c_fmap[i].fr_start &&
                            record->c_fmap[k].fr_start < closest)
                        closest = record->c_fmap[k].fr_start;
                }

                // if the function is too long then we want to avoid decoding the entire thing
                // so check if the last instruction is a RET, if it is then that is our end
                // may save us a lot of execution time
                if ((closest - record->c_fmap[i].fr_start) >
                        conf_b_mmu_fmap_shortcut_threshold) {
                    last_instr = x86_decode(b_mmu, closest-1);

                    if (!last_instr.fail_to_fetch) {
                        if (strcmp(last_instr.name, "RET") == 0) {
                            record->c_fmap[i].fr_end = closest - 1;

                            closest_func = &record->c_fmap[i];
                            goto update_cache_and_exit;
                        }
                    }
                    // its an invalid opcode... it happens
                }

                // decode the entire function and stop at the last instruction
                record->c_fmap[i].fr_end = x86_decodeuntil(b_mmu, record->c_fmap[i].fr_start, closest);
                closest_func = &record->c_fmap[i];
                goto update_cache_and_exit;
            }

            // its a function we have seen before but wasn't in our cache
            if (record->c_fmap[i].fr_end) {

                if (virtaddr >= record->c_fmap[i].fr_start && virtaddr <= record->c_fmap[i].fr_end) {
                    closest_func = &record->c_fmap[i];
                    goto update_cache_and_exit;
                }
            }
        }
        // we didn't find it before but don't panic yet
        // find the closest function to our address
        for (size_t i = 1; i < record->c_fmapsz; i++) {
            if (record->c_fmap[i].fr_start > closest_func->fr_start &&
                    record->c_fmap[i].fr_start < virtaddr)
                closest_func = &record->c_fmap[i];
        }

        // now we find the end of the function closest to us and check
        // to see if we are at its boundaries, if not... Oops
        for (size_t i = 1; i < record->c_fmapsz; i++) {
            if (!closest && record->c_fmap[i].fr_start > closest_func->fr_start) {
                closest = record->c_fmap[i].fr_start;
                continue;
            }

            if (record->c_fmap[i].fr_start > closest_func->fr_start &&
                    record->c_fmap[i].fr_start < closest)
                closest = record->c_fmap[i].fr_start;
        }

        // time-saver in big functions
        if ((closest - closest_func->fr_start) > conf_b_mmu_fmap_shortcut_threshold) {
            last_instr = x86_decode(b_mmu, closest-1);

            if (!last_instr.fail_to_fetch) {
                if (strcmp(last_instr.name, "RET") == 0) {
                    closest_func->fr_end = closest - 1;
                    goto update_cache_and_exit;
                }
            }
            // its an invalid opcode... it happens
        }
        // stop at the last instruction of the function
        closest_func->fr_end = x86_decodeuntil(b_mmu, closest_func->fr_start, closest);

        if (virtaddr > closest_func->fr_start && virtaddr < closest_func->fr_end)
            goto update_cache_and_exit;
        // if we reached here means the we fail somehow
    }

update_cache_and_exit:
    b_mmu->fmap_cache[b_mmu->fmap_cachewridx++] = *closest_func;
    if (b_mmu->fmap_cachewridx >= conf_b_mmu_fmap_cache_size)
        b_mmu->fmap_cachewridx = 0;

    return closest_func;
}

// Read/Write functions

uint8_t b_mmu_fetch(BasicMMU *b_mmu, addr_t virtaddr)
{
    ASSERT(b_mmu != NULL);
    void *buffer;

    buffer = translate_virtaddr(b_mmu, virtaddr, 8);
    if (!buffer) {
        B_mmu_set_error(b_mmu, ESEGFAULT, "invalid memory read at 0x%lx", virtaddr);
        return 0;
    }

    if (!b_mmu_isreadable(b_mmu, virtaddr)) {
        B_mmu_set_error(b_mmu, EPROT, "attempted read at non-readable page at 0x%lx", virtaddr);
        return 0;
    }

    if (!b_mmu_isexecutable(b_mmu, virtaddr)) {
        B_mmu_set_error(b_mmu, EPROT, "attempted to execute code from a non-executable page at 0x%lx", virtaddr);
        return 0;
    }


    return *(uint8_t *)buffer;
}


uint64_t readx(BasicMMU *b_mmu, addr_t virtaddr, int size)
{
    ASSERT(b_mmu != NULL);
    void *buffer;
    u_int64_t bytes = 0;

    buffer = translate_virtaddr(b_mmu, virtaddr, size);
    if (!buffer) {
        B_mmu_set_error(b_mmu, ESEGFAULT, "invalid memory read at 0x%lx", virtaddr);
        return 0;
    }

    if (!b_mmu_isreadable(b_mmu, virtaddr)) {
        B_mmu_set_error(b_mmu, EPROT, "attempted read at non-readable page at 0x%lx", virtaddr);
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
    void *buffer = NULL;

    buffer = translate_virtaddr(b_mmu, virtaddr, size);
    if (!buffer) {
        B_mmu_set_error(b_mmu, ESEGFAULT, "invalid memory read at 0x%lx", virtaddr);
        return;
    }

    if (!b_mmu_iswritable(b_mmu, virtaddr)) {
        B_mmu_set_error(b_mmu, EPROT, "attempted write at non-writable page at 0x%lx", virtaddr);
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
