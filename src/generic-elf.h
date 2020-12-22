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
 * Define machine-independent ELF.
 */

#ifndef GENERIC_ELF_H
#define GENERIC_ELF_H

#include <elf.h>

#include "types.h"

typedef struct {
    Elf32_Off pt_offset;
    Elf32_Addr pt_vaddr;
    uint32_t pt_filesz;
    uint32_t pt_memsz;
    uint32_t pt_flags;
} pt_load_segment_t;

typedef struct {
    pt_load_segment_t *loadable;
    moffset32_t entryp;
    uint16_t nloadable;
    uint16_t machine; // the architecture of the file. See elf.e_machine in elf(5) for values.
    char *name;
    int execstack;
    int fd;

    struct error_description err;
} GenericELF;

enum GenericELF_ERRORS {
    UNSUPPORTED,
    INVALREF,
    NOTAEXEC
};

#define elf_error(elf) ((elf)->err.errnum)
#define elf_errstr(elf) ((elf)->err.description)

#define elf_loadable(elf) ((elf)->loadable)
#define elf_entrypoint(elf) ((elf)->entryp)
#define elf_nloadable(elf) ((elf)->nloadable)
#define elf_underlfd(elf) ((elf)->fd)
#define elf_execstack(elf) ((elf)->execstack)

// load program information needed for execution.
void elf_load(GenericELF *, const char *);

// free any memory allocated.
void g_elf_unload(GenericELF *);
void elf_unload(GenericELF *);

#endif /* GENERIC_ELF.H */
