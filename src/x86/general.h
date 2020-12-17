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

#ifndef GENERAL_H
#define GENERAL_H

#include "../types.h"

// XOR
uint8_t x86__mm_al_imm8_xor(void *, uint8_t);
uint16_t x86__mm_ax_imm16_xor(void *, uint16_t);
uint32_t x86__mm_eax_imm32_xor(void *, uint32_t);
uint8_t x86__mm_m8_imm8_xor(void *, addr_t, uint8_t, _Bool);
uint16_t x86__mm_m16_imm16_xor(void *, addr_t, uint16_t, _Bool);
uint32_t x86__mm_m32_imm32_xor(void *, addr_t, uint32_t, _Bool);
uint8_t x86__mm_r8_imm8_xor(void *, uint8_t, uint8_t);
uint16_t x86__mm_r16_imm16_xor(void *, uint8_t, uint16_t);
uint32_t x86__mm_r32_imm32_xor(void *, uint8_t, uint32_t);
uint8_t x86__mm_r8_r8_xor(void *, uint8_t, uint8_t);
uint16_t x86__mm_r16_r16_xor(void *, uint8_t, uint8_t);
uint32_t x86__mm_r32_r32_xor(void *, uint8_t, uint8_t);
uint8_t x86__mm_m8_r8_xor(void *, addr_t, uint8_t, _Bool);
uint16_t x86__mm_m16_r16_xor(void *, addr_t, uint8_t, _Bool);
uint32_t x86__mm_m32_r32_xor(void *, addr_t, uint8_t, _Bool);
uint8_t x86__mm_r8_m8_xor(void *, uint8_t, addr_t, _Bool);
uint16_t x86__mm_r16_m16_xor(void *, uint8_t, addr_t, _Bool);
uint32_t x86__mm_r32_m32_xor(void *, uint8_t, addr_t, _Bool);

// POP
uint16_t x86__mm_sreg_pop16(void *, uint8_t);
uint32_t x86__mm_sreg_pop32(void *, uint8_t);
uint32_t x86__mm_r32_pop(void *, uint8_t);
uint16_t x86__mm_r16_pop(void *, uint8_t);
uint32_t x86__mm_m32_pop(void *, addr_t);
uint16_t x86__mm_m16_pop(void *, addr_t);

// PUSH
void x86__mm_r32_push(void *, uint8_t);
void x86__mm_m32_push(void *, addr_t);
void x86__mm_r16_push(void *, uint8_t);
void x86__mm_m16_push(void *, addr_t);
void x86__mm_sreg_push(void *, uint8_t);
void x86__mm_imm16_push(void *, uint16_t);
void x86__mm_imm32_push(void *, uint32_t);

// AND
uint8_t x86__mm_al_imm8_and(void *, uint8_t);
uint16_t x86__mm_ax_imm16_and(void *, uint16_t);
uint32_t x86__mm_eax_imm32_and(void *, uint32_t);
uint8_t x86__mm_m8_imm8_and(void *, addr_t, uint8_t, _Bool);
uint8_t x86__mm_r8_imm8_and(void *, uint8_t, uint8_t);
uint16_t x86__mm_m16_imm16_and(void *, addr_t, uint16_t, _Bool);
uint16_t x86__mm_r16_imm16_and(void *, uint8_t, uint16_t);
uint32_t x86__mm_m32_imm32_and(void *, addr_t, uint32_t, _Bool);
uint32_t x86__mm_r32_imm32_and(void *, uint8_t, uint32_t);
uint8_t x86__mm_r8_r8_and(void *, uint8_t, uint8_t);
uint8_t x86__mm_m8_r8_and(void *, addr_t, uint8_t, _Bool);
uint8_t x86__mm_r8_m8_and(void *, uint8_t, addr_t, _Bool);
uint16_t x86__mm_r16_r16_and(void *, uint8_t, uint8_t);
uint16_t x86__mm_m16_r16_and(void *, addr_t, uint8_t, _Bool);
uint16_t x86__mm_r16_m16_and(void *, uint8_t, addr_t, _Bool);
uint32_t x86__mm_r32_r32_and(void *, uint8_t, uint8_t);
uint32_t x86__mm_m32_r32_and(void *, addr_t, uint8_t, _Bool);
uint32_t x86__mm_r32_m32_and(void *, uint8_t, addr_t, _Bool);


// CALL
void x86__mm_rel16_call(void *, int16_t);
void x86__mm_rel32_call(void *, int32_t);
void x86__mm_r16_call(void *, uint8_t);
void x86__mm_r32_call(void *, uint8_t);
void x86__mm_m16_call(void *, addr_t);
void x86__mm_m32_call(void *, addr_t);
void x86__mm_far_ptr16_call(void *, uint16_t, uint16_t);
void x86__mm_far_ptr32_call(void *, uint16_t, uint32_t);

// MOV
void x86__mm_r8_r8_mov(void *, uint8_t, uint8_t);
void x86__mm_r16_r16_mov(void *, uint8_t, uint8_t);
void x86__mm_r32_r32_mov(void *, uint8_t, uint8_t);
void x86__mm_m8_r8_mov(void *, addr_t, uint8_t);
void x86__mm_m16_r16_mov(void *, addr_t, uint8_t);
void x86__mm_m32_r32_mov(void *, addr_t, uint8_t);
void x86__mm_r8_m8_mov(void *, uint8_t, addr_t);
void x86__mm_r16_m16_mov(void *, uint8_t, addr_t);
void x86__mm_r32_m32_mov(void *, uint8_t, addr_t);
void x86__mm_r16_sreg_mov(void *, uint8_t, uint8_t);
void x86__mm_m16_sreg_mov(void *, addr_t, uint8_t);
void x86__mm_sreg_r16_mov(void *, uint8_t, uint8_t);
void x86__mm_sreg_m16_mov(void *, uint8_t, addr_t);
void x86__mm_r8_imm8_mov(void *, uint8_t, uint8_t);
void x86__mm_r16_imm16_mov(void *, uint8_t, uint16_t);
void x86__mm_r32_imm32_mov(void *, uint8_t, uint32_t);
void x86__mm_m8_imm8_mov(void *, uint8_t, uint8_t);
void x86__mm_m16_imm16_mov(void *, uint8_t, uint8_t);
void x86__mm_m32_imm32_mov(void *, uint8_t, uint8_t);


#endif /* GENERAL_H */