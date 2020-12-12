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
 *  Memory Allocation helpers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

static void do_OutOfMemory(void) __attribute__((noreturn));

void xfree(void *ptr)
{
    free(ptr);
}

void *xmalloc(size_t size)
{
    void *buf = malloc(size);

    if (!buf)
        do_OutOfMemory();

    return buf;
}

void *xcalloc(size_t nmemb, size_t size)
{
    void *buf = calloc(nmemb, size);

    if (!buf)
        do_OutOfMemory();

    return buf;
}

void *xreallocarray(void *ptr, size_t nmemb, size_t size)
{
    void *buf = reallocarray(ptr, nmemb, size);

    if (!buf)
        do_OutOfMemory();

    return buf;
}

char *xstrdup(const char *s)
{
    char *new_s = strdup(s);

    if (!new_s)
        do_OutOfMemory();

    return new_s;
}

static void do_OutOfMemory(void)
{
    fprintf(stderr, "\033[1;31merror:\033[0m Out Of Memory\n");
    exit(1);
}

