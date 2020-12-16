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
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint64_t addr_t;

struct loadable_segment {
    addr_t s_offset; // offset from the beggining of the file
    addr_t s_vaddr; // the virtual address of the segment in memory
    uint64_t s_filesz;  // the number of bytes in the file
    uint64_t s_memsz;  // the number of bytes in memory
    uint32_t s_perms;   // the permissions of the segment in memory
};

// I think this is small enough. Remember that this is a description, not a full log.
#define ERROR_DESCRIPTION_MAX_SIZE 80
struct error_description {
    int errnum;
    char description[ERROR_DESCRIPTION_MAX_SIZE];
};


#endif /* TYPES_H */
