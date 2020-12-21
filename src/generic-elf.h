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
<<<<<<< HEAD
    struct loadable_segment *g_loadable;
    moffset32_t g_entryp;
    uint16_t g_nloadable;
    uint16_t g_machine; // the architecture of the file. See elf.e_machine in elf(5) for values.
    char *g_name;
    int g_execstack;
    int g_fd;
=======
    struct loadable_segment *loadable;
    moffset32_t entryp;
    uint16_t nloadable;
    uint16_t machine; // the architecture of the file. See elf.e_machine in elf(5) for values.
    char *name;
    int execstack;
    int fd;
>>>>>>> x86MMU

    struct error_description err;
} GenericELF;

enum GenericELF_ERRORS {
    UNSUPPORTED,
    INVALREF,
    NOTAEXEC
};

#define elf_error(elf) ((elf)->err.errnum)
#define elf_errstr(elf) ((elf)->err.description)

<<<<<<< HEAD
#define G_elf_loadable(g_elf) ((g_elf)->g_loadable)
#define G_elf_nloadable(g_elf) ((g_elf)->g_nloadable)
#define G_elf_underlfd(g_elf) ((g_elf)->g_fd)
#define G_elf_execstack(g_elf) ((g_elf)->g_execstack)
=======
#define elf_loadable(elf) ((elf)->loadable)
#define elf_entrypoint(elf) ((elf)->entryp)
#define elf_nloadable(elf) ((elf)->nloadable)
#define elf_underlfd(elf) ((elf)->fd)
#define elf_execstack(elf) ((elf)->execstack)
>>>>>>> x86MMU

// load program information needed for execution.
void elf_load(GenericELF *, const char *);

// free any memory allocated.
<<<<<<< HEAD
void g_elf_unload(GenericELF *);
=======
void elf_unload(GenericELF *);
>>>>>>> x86MMU

#endif /* GENERIC_ELF.H */
