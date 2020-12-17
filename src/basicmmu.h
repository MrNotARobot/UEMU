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
 *  simulate memory management done by the kernel.
 */

#ifndef BASICMMU_H
#define BASICMMU_H

#include <sys/mman.h>
#include <sys/types.h>

#include "types.h"
#include "generic-elf.h"

typedef struct {
    size_t fr_name; // this is a index to the table where the symbol was from
    addr_t fr_start;
    addr_t fr_end;
} func_rangemap_t;

// keep track of in-use pages to catch any invalid memory reads/writes
typedef struct page_info {
    addr_t p_addr;
    addr_t p_end;
    int p_inuse;
} page_info_t;

typedef struct {
    addr_t c_virtaddr;    // virtual address
    addr_t c_virtaddr_end;    // end of the mapping
    void * c_buffer;    // allocated memory (in this context, it helps visualizing as
                        // the physical memory)
    size_t c_memsz;
    page_info_t *c_pages;
    func_rangemap_t *c_fmap;
    size_t c_npages;
    size_t c_fmapsz;
    int c_perms;
    int c_inuse;
} table_record_t;

enum TableRecordPerms {
    PAGEEXEC = 1,
    PAGEREAD = 2,
    PAGEWRITE = 4
};

typedef struct {
    table_record_t *records;
    size_t max_records;

    func_rangemap_t *fmap_cache;
    size_t fmap_cachewridx;     // the write index for out function map cache
    struct error_description err;
} BasicMMU;


#define B_mmu_error(b_mmu) ((b_mmu)->err.errnum)
#define B_mmu_errstr(b_mmu) ((b_mmu)->err.description)
#define B_mmu_clrerror(b_mmu) ((b_mmu)->err.errnum = 0)


enum BasicMMUErrors {
    ENONE,
    ESEGFAULT,
    EPROT
};

// initialize the data structures needed.
void b_mmu_init(BasicMMU *);
void b_mmu_release_recs(BasicMMU *);

_Bool b_mmu_iswritable(BasicMMU *, addr_t);
_Bool b_mmu_isreadable(BasicMMU *, addr_t);
_Bool b_mmu_isexecutable(BasicMMU *, addr_t);

addr_t b_mmu_mmap(BasicMMU *, addr_t, size_t, int, int, off_t, size_t);
void b_mmu_munmap(BasicMMU *, addr_t, size_t);

addr_t b_mmu_create_stack(BasicMMU *, int);

enum BasicMMUStackFlags {
    B_STACKEXEC
};

// map the loadable segments from the file detailed by the GenericELF struct
void b_mmu_mmap_loadable(BasicMMU *, GenericELF *g_elf);

uint8_t b_mmu_fetch(BasicMMU *, addr_t);
const func_rangemap_t *b_mmu_findfunction(BasicMMU *, addr_t);

uint8_t b_mmu_read8(BasicMMU *, addr_t);
uint16_t b_mmu_read16(BasicMMU *, addr_t);
uint32_t b_mmu_read32(BasicMMU *, addr_t);
uint64_t b_mmu_read64(BasicMMU *, addr_t);

void b_mmu_write8(BasicMMU *, uint8_t, addr_t);
void b_mmu_write16(BasicMMU *, uint16_t, addr_t);
void b_mmu_write32(BasicMMU *, uint32_t, addr_t);
void b_mmu_write64(BasicMMU *, uint64_t, addr_t);

#endif /* BASICMMU_H */
