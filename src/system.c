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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "system.h"
#include "memory.h"


char *find_executable(const char *name)
{
    char *path = xstrdup(getenv("PATH"));
    char *fullpath = NULL;
    char *next, *current;
    size_t path_size, name_len;

    name_len = strlen(name);
    current = path;

    /* this loop will check all entries but the last in PATH */
    while ( (next = index(current, ':')) ) {
        next++;
        path_size = next - current;

        fullpath = xmalloc(path_size + name_len);
        memset(fullpath, 0, path_size + name_len);

        strncpy(fullpath, current, path_size);
        fullpath[path_size -1] = '/';
        strncat(fullpath, name, name_len);

        if (access(fullpath, F_OK) == 0)
            break;

        xfree(fullpath);

        current = next;
        fullpath = NULL;
    }

    /* check the last entry in PATH */
    if (!fullpath) {
        path_size = strlen(current);

        fullpath = xmalloc(path_size + name_len + 2);
        memset(fullpath, 0, path_size + name_len + 2);

        strncpy(fullpath, current, path_size);
        fullpath[path_size] = '/';
        strncat(fullpath, name, name_len);

        if (access(fullpath, F_OK) != 0) {
            xfree(fullpath);
            fullpath = NULL;
        }
    }

    xfree(path);
    return fullpath;
}

char *coolstrcat(char *dest, size_t argc, ...)
{
    va_list ap;

    va_start(ap, argc);
    for (size_t i = 0; i < argc; i++)
        strcat(dest, va_arg(ap, const char *));
    va_end(ap);

    return dest;
}

char *int2hexstr(uint32_t n, uint8_t padding)
{
    char *s = xcalloc(11, sizeof(*s));  // 0xdeadbeef = 10 characters + null byte
    size_t index = 0;
    uint32_t mask = 0xf0000000;
    int nibble = 8;
    _Bool found_bits = 0;   // to not add leading bytes to the string
    size_t first_nibble = 0;

    s[index++] = '0';
    s[index++] = 'x';

    if (!n && !padding) {
        s[index++] = '0';
        return s;
    }

    if (padding) {
        for (size_t i = nibble, k = mask; i != 0; i--, k >>=4) {
            if ((n & k) >> 4*(i - 1)) {
                first_nibble = i;
                if (first_nibble >= padding)
                    padding = 0;
                else
                    padding -= first_nibble;
                break;
            }
        }
            if (index + padding >= 10)
                s = xreallocarray(s, index + padding + first_nibble + 1, sizeof(*s));

            for (size_t i = 0; i < padding; i++)
                s[index++] = '0';
    }

    if (!n)
        return s;

    while (nibble != 0) {

        if ((index - padding) > 2)
            found_bits = 1;

        switch ((n & mask) >> 4*(nibble - 1)) {
            case 0: (found_bits) ? s[index++] = '0' : 0; break;
            case 1: s[index++] = '1'; break;
            case 2: s[index++] = '2'; break; case 3: s[index++] = '3'; break;
            case 4: s[index++] = '4'; break; case 5: s[index++] = '5'; break;
            case 6: s[index++] = '6'; break; case 7: s[index++] = '7'; break;
            case 8: s[index++] = '8'; break; case 9: s[index++] = '9'; break;
            case 10: s[index++] = 'a'; break; case 11: s[index++] = 'b'; break;
            case 12: s[index++] = 'c'; break; case 13: s[index++] = 'd'; break;
            case 14: s[index++] = 'e'; break; case 15: s[index++] = 'f'; break;
        }
        nibble--;
        mask >>= 4;
    }

    return s;
}


void s_error(int err, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\033[31;1merror:\033[0m ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(err);
}

void s_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
