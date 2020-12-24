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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "string-utils.h"

#include "memory.h"

char *coolstrcat(char *dest, size_t argc, ...)
{
    va_list ap;

    va_start(ap, argc);
    for (size_t i = 0; i < argc; i++)
        strcat(dest, va_arg(ap, const char *));
    va_end(ap);

    return dest;
}

char *strcatall(size_t argc, ...)
{
    va_list ap;
    char *s;
    size_t size = 0;

    va_start(ap, argc);
    for (size_t i = 0; i < argc; i++)
        size += strlen(va_arg(ap, const char *));
    va_end(ap);

    s = xcalloc(size + 1, sizeof(*s));

    va_start(ap, argc);
    for (size_t i = 0; i < argc; i++)
        strcat(s, va_arg(ap, const char *));
    va_end(ap);

    return s;
}


char *int2hexstr(uint32_t n, uint8_t padding)
{
    char *s = xcalloc(12, sizeof(*s));  // 0xdeadbeef = 10 characters + null byte
    if (padding > 8)
        padding = 8;

    snprintf(s, 11, "0x%0*x", padding, n);
    return s;
}

char *int2str(uint32_t n)
{
    char *s = xcalloc(15, sizeof(*s));  // should be big enough for UINT32_MAX

    if (!n)
        return xstrdup("0");

    snprintf(s, 10, "%d", n);
    return s;
}
