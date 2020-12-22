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
#include "x86-mmu.h"

enum x86DecoderFlags {
    STOP_ON_RET
};

// decodes the bytes from the given buffer and returns a instruction ready for execution
struct instruction x86_decode(x86MMU *, moffset32_t);
// finds the last instruction before the given address
moffset32_t x86_decodeuntil(x86MMU *, moffset32_t, moffset32_t);
// find the target address of a call
moffset32_t x86_findbranchtarget(void *cpu, struct exec_data data);
char *x86_disassemble(struct instruction);

#endif /* DISASSEMBLER_H */
