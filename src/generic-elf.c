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

static void elf_set_error(GenericELF *, int, const char *, ...);
static void elf_load32(GenericELF *);
static void elf_load64(GenericELF *);

static void elf_set_error(GenericELF *elf, int errnum, const char *fmt, ...)
{
    va_list ap;

    // NOTE: do we isolate any dangerous format specifier? Or is this safe
    // enough as this will only be called in this file anyway

    elf->err.errnum = errnum;
    va_start(ap, fmt);
    vsnprintf(elf->err.description, ERROR_DESCRIPTION_MAX_SIZE, fmt, ap);
    va_end(ap);
}

static void elf_load32(GenericELF *elf)
{
    ASSERT(elf != NULL);
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;
    Elf32_Off phoff;
    int fd = elf->fd;
    uint16_t nloadable = 0;

    if (pread(fd, &ehdr, sizeof(ehdr), 0) == -1) {
        elf_set_error(elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        return;
    }

    // check supported architectures
    switch (ehdr.e_machine) {
        case EM_386:
            break;
        default:
            elf_set_error(elf, UNSUPPORTED, "emulator: unsupported architecture");
            return;
    }

    elf->machine = ehdr.e_machine;

    // first get the number of loadable (PT_LOAD) segments to allocate
    // space for those program headers
    phoff = ehdr.e_phoff;
    for (uint16_t i = 0; i < ehdr.e_phnum; i++, phoff+=ehdr.e_phentsize) {

        if (pread(fd, &phdr, sizeof(phdr), phoff) == -1) {
            elf_set_error(elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
            return;
        }

        if (phdr.p_type == PT_LOAD)
            nloadable++;

        if (phdr.p_type == PT_GNU_STACK)
            elf->execstack = phdr.p_flags & PF_X;
    }

    phoff = ehdr.e_phoff;

    elf->nloadable = nloadable;
    elf->loadable = (struct loadable_segment *)xcalloc(nloadable, sizeof(*elf->loadable));
    for (uint16_t i = 0, k = 0; (k + 1) < nloadable; i++, phoff+=ehdr.e_phentsize) {

        if (pread(fd, &phdr, sizeof(phdr), phoff) == -1) {
            elf_set_error(elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
            xfree(elf->loadable);
            return;
        }

        if (phdr.p_type == PT_LOAD) {
            elf->loadable[k].s_offset = phdr.p_offset;
            elf->loadable[k].s_filesz = phdr.p_filesz;
            elf->loadable[k].s_memsz = phdr.p_memsz;
            elf->loadable[k].s_perms = phdr.p_flags;
            elf->loadable[k++].s_vaddr = phdr.p_vaddr;
        }
    }

    elf->entryp = ehdr.e_entry;

}

static void elf_load64(GenericELF *elf)
{
    ASSERT(elf != NULL);
    elf_set_error(elf, UNSUPPORTED, " TODO : support 64-bit binaries");
    return;
}

/*
 * Do the basic file checking to garantee <executable> is an ELF file
 * an we properly support it.
 * If everything is right, read the program headers for information on
 * loadable segments.
 *
 * the actual mapping is done with elf_mmap_loadable()
 *
 */
void elf_load(GenericELF *elf, const char *executable)
{
    ASSERT(elf != NULL);

    unsigned char e_ident[EI_NIDENT];
    int fd;

    elf_set_error(elf, 0, NULL);

    if (!executable) {
        elf_set_error(elf, INVALREF, "emulator: invalid argument (NULL)");
        return;
    }
    
    if (access(executable, X_OK) == -1) {
        elf_set_error(elf, NOTAEXEC, "emulator: file is not an executable");
        return;
    }

    fd = open(executable, O_RDONLY);
    if (fd == -1) {
        elf_set_error(elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        return;
    }

    if (pread(fd, e_ident, sizeof(e_ident), 0) == -1) {
        elf_set_error(elf, errno,  "%s: %s", __FUNCTION__, strerror(errno));
        close(fd);
        return;
    }

    if (e_ident[EI_MAG0] != ELFMAG0 && e_ident[EI_MAG1] != ELFMAG1
            && e_ident[EI_MAG2] != ELFMAG2 && e_ident[EI_MAG3] != ELFMAG3) {

        elf_set_error(elf, NOTAEXEC, "emulator: file is not an ELF");
        close(fd);
        return;
    }

    elf->name = xstrdup(executable);
    elf->fd = fd;

    // make sure we're using the correct ABI
    switch (e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV:
            // supported
            break;
        default:
            elf_set_error(elf, UNSUPPORTED, "emulator: unsupported ABI");
            return;
    }

    if (e_ident[EI_CLASS] == ELFCLASS32) {
        elf_load32(elf);
    } else if (e_ident[EI_CLASS] == ELFCLASS64) {
        elf_load64(elf);
    } else {
        elf_set_error(elf, UNSUPPORTED, "emulator: unsupported architecture (only 32-bit and 64-bit are supported)");
        close(fd);
        return;
    }

    if (elf_error(elf)) {
        close(fd);
        return;
    }
}

/*
 * Free allocated memory and close the underlying file descriptor.
 */
void elf_unload(GenericELF *elf)
{
    ASSERT(elf != NULL);

    xfree(elf->name);
    xfree(elf->loadable);
    close(elf->fd);
}

