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
 *  Handles file information needed for execution.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "generic-elf.h"
#include "system.h"
#include "memory.h"

static void G_elf_set_error(GenericELF *, int, const char *, ...);
static void elf_load32(GenericELF *);
static void elf_load64(GenericELF *);

static void G_elf_set_error(GenericELF *g_elf, int errnum, const char *fmt, ...)
{
    va_list ap;

    // NOTE: do we isolate any dangerous format specifier? Or is this safe
    // enough as this will only be called in this file anyway

    g_elf->err.errnum = errnum;
    va_start(ap, fmt);
    vsnprintf(g_elf->err.description, ERROR_DESCRIPTION_MAX_SIZE, fmt, ap);
    va_end(ap);
}

static void elf_load32(GenericELF *g_elf)
{
    ASSERT(g_elf != NULL);
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;
    Elf32_Off phoff;
    int fd = g_elf->g_fd;
    uint16_t nloadable = 0;

    if (pread(fd, &ehdr, sizeof(ehdr), 0) == -1) {
        G_elf_set_error(g_elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        return;
    }

    // check supported architectures
    switch (ehdr.e_machine) {
        case EM_386:
            break;
        default:
            G_elf_set_error(g_elf, UNSUPPORTED, "emulator: unsupported architecture");
            return;
    }

    g_elf->g_machine = ehdr.e_machine;

    // first get the number of loadable (PT_LOAD) segments to allocate
    // space for those program headers
    phoff = ehdr.e_phoff;
    for (uint16_t i = 0; i < ehdr.e_phnum; i++, phoff+=ehdr.e_phentsize) {

        if (pread(fd, &phdr, sizeof(phdr), phoff) == -1) {
            G_elf_set_error(g_elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
            return;
        }

        if (phdr.p_type == PT_LOAD)
            nloadable++;

        if (phdr.p_type == PT_GNU_STACK)
            g_elf->g_execstack = phdr.p_flags & PF_X;
    }

    phoff = ehdr.e_phoff;

    g_elf->g_nloadable = nloadable;
    g_elf->g_loadable = (struct loadable_segment *)xcalloc(nloadable, sizeof(*g_elf->g_loadable));
    for (uint16_t i = 0, k = 0; (k + 1) < nloadable; i++, phoff+=ehdr.e_phentsize) {

        if (pread(fd, &phdr, sizeof(phdr), phoff) == -1) {
            G_elf_set_error(g_elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
            xfree(g_elf->g_loadable);
            return;
        }

        if (phdr.p_type == PT_LOAD) {
            g_elf->g_loadable[k].s_offset = phdr.p_offset;
            g_elf->g_loadable[k].s_filesz = phdr.p_filesz;
            g_elf->g_loadable[k].s_memsz = phdr.p_memsz;
            g_elf->g_loadable[k].s_perms = phdr.p_flags;
            g_elf->g_loadable[k++].s_vaddr = phdr.p_vaddr;
        }
    }

    g_elf->g_entryp = ehdr.e_entry;

}

static void elf_load64(GenericELF *g_elf)
{
    ASSERT(g_elf != NULL);
    G_elf_set_error(g_elf, UNSUPPORTED, " TODO : support 64-bit binaries");
    return;
}

/*
 * Do the basic file checking to garantee <executable> is an ELF file
 * an we properly support it.
 * If everything is right, read the program headers for information on
 * loadable segments.
 *
 * the actual mapping is done with g_elf_mmap_loadable()
 *
 */
void g_elf_load(GenericELF *g_elf, const char *executable)
{
    ASSERT(g_elf != NULL);

    unsigned char e_ident[EI_NIDENT];
    int fd;

    G_elf_set_error(g_elf, 0, NULL);

    if (!executable) {
        G_elf_set_error(g_elf, INVALREF, "emulator: invalid argument (NULL)");
        return;
    }
    
    if (access(executable, X_OK) == -1) {
        G_elf_set_error(g_elf, NOTAEXEC, "emulator: file is not an executable");
        return;
    }

    fd = open(executable, O_RDONLY);
    if (fd == -1) {
        G_elf_set_error(g_elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        return;
    }

    if (pread(fd, e_ident, sizeof(e_ident), 0) == -1) {
        G_elf_set_error(g_elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        close(fd);
        return;
    }

    if (e_ident[EI_MAG0] != ELFMAG0 && e_ident[EI_MAG1] != ELFMAG1
            && e_ident[EI_MAG2] != ELFMAG2 && e_ident[EI_MAG3] != ELFMAG3) {

        G_elf_set_error(g_elf, NOTAEXEC, "emulator: file is not an ELF");
        close(fd);
        return;
    }

    g_elf->g_name = xstrdup(executable);
    g_elf->g_fd = fd;

    // make sure we're using the correct ABI
    switch (e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV:
            // supported
            break;
        default:
            G_elf_set_error(g_elf, UNSUPPORTED, "emulator: unsupported ABI");
            return;
    }

    if (e_ident[EI_CLASS] == ELFCLASS32) {
        elf_load32(g_elf);
    } else if (e_ident[EI_CLASS] == ELFCLASS64) {
        elf_load64(g_elf);
    } else {
        G_elf_set_error(g_elf, UNSUPPORTED, "emulator: unsupported architecture (only 32-bit and 64-bit are supported)");
        close(fd);
        return;
    }

    if (G_elf_error(g_elf)) {
        close(fd);
        return;
    }
}

/*
 * Free allocated memory and close the underlying file descriptor.
 */
void g_elf_unload(GenericELF *g_elf)
{
    if (!g_elf)
        return;

    xfree(g_elf->g_name);
    xfree(g_elf->g_loadable);
    close(g_elf->g_fd);
}

