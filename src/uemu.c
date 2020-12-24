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

#include "x86/cpu.h"
#include "system.h"

int main(int argc, char **argv, char **envp)
{
    char *executable, *program_name;

    if (argc < 2) {
        printf("usage: uemu <program>\n");
        exit(0);
    }

    program_name = argv[1];
    executable = realpath(argv[1], NULL);

    /* realpath(3) did not get the full pathname. Search through PATH. */
    if (!executable)
        executable = find_executable(program_name);

    x86_cpu_exec(executable, argc-1, &argv[1], envp);

    ASSERT_NOTREACHED();
}
