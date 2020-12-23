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

#ifndef X86_MMU_H
#define X86_MMU_H

#include <sys/types.h>
#include <stdint.h>

#include "../types.h"
#include "../generic-elf.h"

enum SegmentTypes {
    ST_NONE,
    ST_RODATA,
    ST_RWDATA,
    ST_XOCODE,
    ST_RXCODE,
    ST_RWXCODE,
    ST_RWSTACK,
    ST_RWXSTACK
};

typedef struct {
    void *buffer_;
    moffset32_t s_start;
    moffset32_t s_limit;

    int s_type;
} segment_t;

typedef struct {
    segment_t *mm_stack;
    segment_t *mm_segment_tbl;
    size_t mm_segments;

    struct error_description err;
} x86MMU;


#define mmu_error(b_mmu) ((b_mmu)->err.errnum)
#define mmu_errstr(b_mmu) ((b_mmu)->err.description)
#define mmu_clrerror(b_mmu) ((b_mmu)->err.errnum = 0)


enum x86MMUErrors {
    ENONE,
    ESEGFAULT,
    EPROT
};

// initialize the data structures needed.
void mmu_init(x86MMU *);
void mmu_unloadall(x86MMU *);

moffset32_t mmu_create_stack(x86MMU *, int);

enum x86MMUStackFlags {
    B_STACKEXEC
};

// map the loadable segments from the file detailed by the GenericELF struct
void mmu_mmap_loadable(x86MMU *, GenericELF *);

uint8_t mmu_fetch(x86MMU *, moffset32_t);

uint8_t mmu_read8(x86MMU *, moffset32_t);
uint16_t mmu_read16(x86MMU *, moffset32_t);
uint32_t mmu_read32(x86MMU *, moffset32_t);
uint64_t mmu_read64(x86MMU *, moffset32_t);

void mmu_write8(x86MMU *, uint8_t, moffset32_t);
void mmu_write16(x86MMU *, uint16_t, moffset32_t);
void mmu_write32(x86MMU *, uint32_t, moffset32_t);
void mmu_write64(x86MMU *, uint64_t, moffset32_t);

// returns a read-only ptr
const uint8_t *mmu_getptr(x86MMU *, moffset32_t);
int mmu_ptrtype(x86MMU *, moffset32_t);

#endif /* X86_MMU_H */
