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

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "instructions.h"
#include "cpu.h"

enum x86DecoderFlags {
    STOP_ON_RET
};

// decodes the bytes from the given buffer and returns a instruction ready for execution
struct instruction x86_decode(x86CPU *, moffset32_t);
// find the target address of a call
moffset32_t x86_findbranchtarget_relative(x86CPU *, moffset32_t, struct exec_data);
moffset32_t x86_findbranchtarget(x86CPU *, struct exec_data);
char *x86_disassemble(x86CPU *, struct instruction);

#endif /* DISASSEMBLER_H */
