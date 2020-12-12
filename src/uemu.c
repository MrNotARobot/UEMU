/* Copyright (c) 2020 Gabriel Manoel 
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
 * DESCRIPTION: Main program loop.
 */

#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "generic-elf.h"
#include "system.h"
#include "basicmmu.h"

int main(int argc, char **argv)
{
    char *executable, *program_name;
    GenericELF g_elf;
    BasicMMU b_mmu;

    if (argc < 2) {
        printf("usage: uemu <program>\n");
        exit(0);
    }

    program_name = argv[1];
    executable = realpath(argv[1], NULL);

    /* realpath(3) did not get the full pathname. Search through PATH. */
    if (!executable) {
        executable = find_executable(program_name);
    }

    // map the executable segments to memory.
    // no shared library is loaded just yet.
    g_elf_load(&g_elf, executable);

    // we don't need it anymore.
    xfree(executable);

    if (G_elf_error(&g_elf))
        s_error(1, "%s", G_elf_errstr(&g_elf));

    b_mmu_init(&b_mmu);

    b_mmu_mmap_loadable(&g_elf, &b_mmu);
    if (B_mmu_error(&b_mmu))
        s_error(1, "%s", B_mmu_errstr(&b_mmu));

    g_elf_unload(&g_elf);
    b_mmu_release_recs(&b_mmu);
}
