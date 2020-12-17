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
    char *s_name;
    addr_t s_value;
} ELF_Sym;

typedef struct {
    struct loadable_segment *g_loadable;
    addr_t g_entryp;
    uint16_t g_nloadable;
    uint16_t g_machine; // the architecture of the file. See elf.e_machine in elf(5) for values.
    char *g_name;
    int g_execstack;
    int g_fd;

    ELF_Sym *g_symtab;
    size_t g_symtabsz;
    struct error_description err;
} GenericELF;

enum GenericELF_ERRORS {
    UNSUPPORTED,
    INVALREF,
    NOTAEXEC
};

#define G_elf_error(g_elf) ((g_elf)->err.errnum)
#define G_elf_errstr(g_elf) ((g_elf)->err.description)

#define G_elf_loadable(g_elf) ((g_elf)->g_loadable)
#define G_elf_nloadable(g_elf) ((g_elf)->g_nloadable)
#define G_elf_underlfd(g_elf) ((g_elf)->g_fd)
#define G_elf_execstack(g_elf) ((g_elf)->g_execstack)
#define G_elf_symtabsz(g_elf) ((g_elf)->g_symtabsz)

// load program information needed for execution.
void g_elf_load(GenericELF *, const char *);

// free any memory allocated.
void g_elf_unload(GenericELF *);

ELF_Sym g_elf_getsym(GenericELF *, size_t);
const char *g_elf_getfromstrtab(GenericELF *, size_t);
const char *g_elf_lookup(GenericELF *, addr_t);

#endif /* GENERIC_ELF.H */
