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

#include "general.h"
#include "cpu.h"
#include "x86-utils.h"

// XOR

static uint32_t x86__mm_rX_immX_xor(void *, uint8_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_xor(void *, moffset32_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_locked_xor(void *, moffset32_t, uint32_t, int);

static uint32_t x86__mm_rX_immX_xor(void *cpu, uint8_t reg, uint32_t imm, int size)
{
    uint32_t result;
    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_rdreg8(cpu, reg) ^ lsb(imm);
        x86_wrreg8(cpu, reg, lsb(result));
    } else if (size == 16) {
        result = x86_rdreg16(cpu, reg) ^ low16(imm);
        x86_wrreg16(cpu, reg, low16(result));
    } else {
        result = x86_rdreg32(cpu, reg) ^ imm;
        x86_wrreg32(cpu, reg, result);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

static uint32_t x86__mm_mX_immX_xor(void *cpu, moffset32_t effctvaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_rdmem8(cpu, effctvaddr) ^ lsb(imm);
        x86_wrmem8(cpu, effctvaddr, lsb(result));
    } else if (size == 16) {
        result = x86_rdmem16(cpu, effctvaddr) ^ low16(imm);
        x86_wrmem16(cpu, effctvaddr, low16(result));
    } else {
        result = x86_rdmem32(cpu, effctvaddr) ^ imm;
        x86_wrmem32(cpu, effctvaddr, result);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_xor(void *cpu, moffset32_t effctvaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_atomic_rdmem8(cpu, effctvaddr) ^ lsb(imm);
        x86_atomic_wrmem8(cpu, effctvaddr, lsb(result));
    } else if (size == 16) {
        result = x86_atomic_rdmem16(cpu, effctvaddr) ^ low16(imm);
        x86_atomic_wrmem16(cpu, effctvaddr, low16(result));
    } else {
        result = x86_atomic_rdmem32(cpu, effctvaddr) ^ imm;
        x86_atomic_wrmem32(cpu, effctvaddr, result);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

uint8_t x86__mm_al_imm8_xor(void *cpu, uint8_t imm)
{
    return x86__mm_r8_imm8_xor(cpu, AL, imm);
}

uint16_t x86__mm_ax_imm16_xor(void *cpu, uint16_t imm)
{
    return x86__mm_r16_imm16_xor(cpu, AX, imm);
}

uint32_t x86__mm_eax_imm32_xor(void *cpu, uint32_t imm)
{
    return x86__mm_r32_imm32_xor(cpu, EAX, imm);
}

uint8_t x86__mm_m8_imm8_xor(void *cpu, moffset32_t effctvaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_xor(cpu, effctvaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_xor(cpu, effctvaddr, imm, 8);
}

uint16_t x86__mm_m16_imm16_xor(void *cpu, moffset32_t effctvaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_xor(cpu, effctvaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_xor(cpu, effctvaddr, imm, 16);
}

uint32_t x86__mm_m32_imm32_xor(void *cpu, moffset32_t effctvaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_xor(cpu, effctvaddr, imm, 32);
    return x86__mm_mX_immX_xor(cpu, effctvaddr, imm, 32);
}


uint8_t x86__mm_r8_imm8_xor(void *cpu, uint8_t reg, uint8_t imm)
{
    return (uint8_t)x86__mm_rX_immX_xor(cpu, reg, imm, 8);
}

uint16_t x86__mm_r16_imm16_xor(void *cpu, uint8_t reg, uint16_t imm)
{
    return (uint16_t)x86__mm_rX_immX_xor(cpu, reg, imm, 16);
}

uint32_t x86__mm_r32_imm32_xor(void *cpu, uint8_t reg, uint32_t imm)
{
    return x86__mm_rX_immX_xor(cpu, reg, imm, 32);
}

uint8_t x86__mm_r8_r8_xor(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint8_t)x86__mm_rX_immX_xor(cpu, dest, x86_rdreg8(cpu, reg2), 8);
}

uint16_t x86__mm_r16_r16_xor(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint16_t)x86__mm_rX_immX_xor(cpu, dest, x86_rdreg16(cpu, reg2), 16);
}

uint32_t x86__mm_r32_r32_xor(void *cpu, uint8_t dest, uint8_t reg2)
{
    return x86__mm_rX_immX_xor(cpu, dest, x86_rdreg32(cpu, reg2), 32);
}

uint8_t x86__mm_m8_r8_xor(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_xor(cpu, effctvaddr, x86_rdreg8(cpu, reg), 8);
    return (uint8_t)x86__mm_mX_immX_xor(cpu, effctvaddr, x86_rdreg8(cpu, reg), 8);
}

uint16_t x86__mm_m16_r16_xor(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_xor(cpu, effctvaddr, x86_rdreg16(cpu, reg), 16);
    return (uint16_t)x86__mm_mX_immX_xor(cpu, effctvaddr, x86_rdreg16(cpu, reg), 16);
}

uint32_t x86__mm_m32_r32_xor(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_xor(cpu, effctvaddr, x86_rdreg32(cpu, reg), 32);
    return x86__mm_mX_immX_xor(cpu, effctvaddr, x86_rdreg32(cpu, reg), 32);
}

uint8_t x86__mm_r8_m8_xor(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_xor(cpu, reg, x86_atomic_rdmem8(cpu, effctvaddr), 8);
    return (uint8_t)x86__mm_rX_immX_xor(cpu, reg, x86_rdmem8(cpu, effctvaddr), 8);
}

uint16_t x86__mm_r16_m16_xor(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_xor(cpu, reg, x86_atomic_rdmem16(cpu, effctvaddr), 16);
    return (uint16_t)x86__mm_rX_immX_xor(cpu, reg, x86_rdmem16(cpu, effctvaddr), 16);
}

uint32_t x86__mm_r32_m32_xor(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_xor(cpu, reg, x86_atomic_rdmem32(cpu, effctvaddr), 32);
    return x86__mm_rX_immX_xor(cpu, reg, x86_rdmem32(cpu, effctvaddr), 32);
}

// POP

static uint32_t x86__mm_rX_popX(void *, uint8_t, int);
static uint32_t x86__mm_mX_popX(void *, moffset32_t, int);

static uint32_t x86__mm_rX_popX(void *cpu, uint8_t reg, int size)
{
    uint32_t value;

    if (size == 16) {
        value = x86_rdmem16(cpu, x86_rdreg32(cpu, ESP));
        x86_wrreg16(cpu, reg, value);
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) + 4);
    } else {
        value = x86_rdmem32(cpu, x86_rdreg32(cpu, ESP));
        x86_wrreg32(cpu, reg, value);
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) + 4);
    }

    return value;
}

static uint32_t x86__mm_mX_popX(void *cpu, moffset32_t effctvaddr, int size)
{
    uint32_t value;

    if (size == 16) {
        value = x86_rdmem16(cpu, x86_rdreg32(cpu, ESP));
        x86_wrmem16(cpu, effctvaddr, value);
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) + 4);
    } else {
        value = x86_rdmem32(cpu, x86_rdreg32(cpu, ESP));
        x86_wrmem32(cpu, effctvaddr, value);
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) + 4);
    }

    return value;
}

uint16_t x86__mm_sreg_pop(void *cpu, uint8_t sreg)
{
    uint16_t value = x86_rdmem16(cpu, ESP);

    x86_wrsreg(cpu, sreg, value);
    x86_wrsreg(cpu, ESP, x86_rdreg32(cpu, ESP) - 4);

    return value;
}

uint32_t x86__mm_r32_pop(void *cpu, uint8_t reg)
{
    return x86__mm_rX_popX(cpu, reg, 32);
}

uint16_t x86__mm_r16_pop(void *cpu, uint8_t reg)
{
    return (uint16_t)x86__mm_rX_popX(cpu, reg, 16);
}

uint32_t x86__mm_m32_pop(void *cpu, moffset32_t effctvaddr)
{
    return x86__mm_mX_popX(cpu, effctvaddr, 32);
}

uint16_t x86__mm_m16_pop(void *cpu, moffset32_t effctvaddr)
{
    return (uint16_t)x86__mm_mX_popX(cpu, effctvaddr, 16);
}

// PUSH
static void x86__mm_immX_push(void *, uint32_t, int);

static void x86__mm_immX_push(void *cpu, uint32_t imm, int size)
{
    reg32_t esp = x86_rdreg32(cpu, ESP);

    if (size == 16) {
        x86_wrmem16(cpu, esp - 2, imm);
        x86_wrreg32(cpu, ESP, esp - 2);
    } else  {
        x86_wrmem32(cpu, esp - 4, imm);
        x86_wrreg32(cpu, ESP, esp - 4);
    }
}

void x86__mm_r32_push(void *cpu, uint8_t reg)
{
    x86__mm_immX_push(cpu, x86_rdreg32(cpu, reg), 32);
}

void x86__mm_m32_push(void *cpu, moffset32_t effctvaddr)
{
    x86__mm_immX_push(cpu, x86_rdmem32(cpu, effctvaddr), 32);
}

void x86__mm_r16_push(void *cpu, uint8_t reg)
{
    x86__mm_immX_push(cpu, x86_rdreg16(cpu, reg), 16);
}

void x86__mm_m16_push(void *cpu, moffset32_t effctvaddr)
{
    x86__mm_immX_push(cpu, x86_rdmem16(cpu, effctvaddr), 16);
}

void x86__mm_sreg_push(void *cpu, uint8_t reg)
{
    reg16_t esp = x86_rdreg16(cpu, ESP);
    x86_wrmem16(cpu, esp, x86_rdsreg(cpu, reg));
    x86_wrreg32(cpu, ESP, esp - 2);
}

void x86__mm_imm16_push(void *cpu, uint16_t imm)
{
    x86__mm_immX_push(cpu, imm, 16);
}

void x86__mm_imm32_push(void *cpu, uint32_t imm)
{
    x86__mm_immX_push(cpu, imm, 32);
}

// AND

static uint32_t x86__mm_rX_immX_and(void *, uint8_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_and(void *, moffset32_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_locked_and(void *, moffset32_t, uint32_t, int);

#include "../system.h"
static uint32_t x86__mm_rX_immX_and(void *cpu, uint8_t reg, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_rdreg8(cpu, reg) & sign8to32(lsb(imm));
        x86_wrreg8(cpu, reg, lsb(result));
    } else if (size == 16) {
        result = x86_rdreg16(cpu, reg) & sign16to32(low16(imm));
        x86_wrreg16(cpu, reg, low16(result));
    } else {
        result = x86_rdreg32(cpu, reg) & imm;
        x86_wrreg32(cpu, reg, result);
    }

    if (result == 0)
        x86_setflag(cpu, CF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

static uint32_t x86__mm_mX_immX_and(void *cpu, moffset32_t effctvaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_rdmem8(cpu, effctvaddr) & sign8to32(lsb(imm));
        x86_wrmem8(cpu, effctvaddr, lsb(result));
    } else if (size == 16) {
        result = x86_rdmem16(cpu, effctvaddr) & sign16to32(low16(imm));
        x86_wrmem16(cpu, effctvaddr, low16(result));
    } else {
        result = x86_rdmem32(cpu, effctvaddr) & imm;
        x86_wrmem32(cpu, effctvaddr, result);
    }

    if (result == 0)
        x86_setflag(cpu, CF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_and(void *cpu, moffset32_t effctvaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);

    if (size == 8) {
        result = x86_atomic_rdmem8(cpu, effctvaddr) & sign8to32(lsb(imm));
        x86_atomic_wrmem8(cpu, effctvaddr, lsb(result));
    } else if (size == 16) {
        result = x86_atomic_rdmem16(cpu, effctvaddr) & sign16to32(low16(imm));
        x86_atomic_wrmem16(cpu, effctvaddr, low16(result));
    } else {
        result = x86_atomic_rdmem32(cpu, effctvaddr) & imm;
        x86_atomic_wrmem32(cpu, effctvaddr, result);
    }

    if (result == 0)
        x86_setflag(cpu, CF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (signbit(result))
        x86_setflag(cpu, SF);

    return result;
}

uint8_t x86__mm_al_imm8_and(void *cpu, uint8_t imm)
{

    return x86__mm_r8_imm8_and(cpu, AL, imm);
}

uint16_t x86__mm_ax_imm16_and(void *cpu, uint16_t imm)
{
    return x86__mm_r16_imm16_and(cpu, AX, imm);
}

uint32_t x86__mm_eax_imm32_and(void *cpu, uint32_t imm)
{
    return x86__mm_r32_imm32_and(cpu, EAX, imm);
}

uint8_t x86__mm_r8_imm8_and(void *cpu, uint8_t reg, uint8_t imm)
{
    return (uint8_t)x86__mm_rX_immX_and(cpu, reg, imm, 8);
}

uint16_t x86__mm_r16_imm16_and(void *cpu, uint8_t reg, uint16_t imm)
{
    return (uint16_t)x86__mm_rX_immX_and(cpu, reg, imm, 16);
}

uint32_t x86__mm_r32_imm32_and(void *cpu, uint8_t reg, uint32_t imm)
{
    return x86__mm_rX_immX_and(cpu, reg, imm, 32);
}

uint8_t x86__mm_m8_imm8_and(void *cpu, moffset32_t effctvaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_and(cpu, effctvaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_and(cpu, effctvaddr, imm, 8);
}

uint16_t x86__mm_m16_imm16_and(void *cpu, moffset32_t effctvaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_and(cpu, effctvaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_and(cpu, effctvaddr, imm, 16);
}

uint32_t x86__mm_m32_imm32_and(void *cpu, moffset32_t effctvaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_and(cpu, effctvaddr, imm, 32);
    return x86__mm_mX_immX_and(cpu, effctvaddr, imm, 32);
}

uint8_t x86__mm_r8_r8_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint8_t)x86__mm_rX_immX_and(cpu, dest, x86_rdreg8(cpu, reg2), 8);
}

uint16_t x86__mm_r16_r16_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint16_t)x86__mm_rX_immX_and(cpu, dest, x86_rdreg16(cpu, reg2), 16);
}

uint32_t x86__mm_r32_r32_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return x86__mm_rX_immX_and(cpu, dest, x86_rdreg32(cpu, reg2), 32);
}

uint8_t x86__mm_m8_r8_and(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_and(cpu, effctvaddr, x86_rdreg8(cpu, reg), 8);
    return (uint8_t)x86__mm_mX_immX_and(cpu, effctvaddr, x86_rdreg8(cpu, reg), 8);
}

uint16_t x86__mm_m16_r16_and(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_and(cpu, effctvaddr, x86_rdreg16(cpu, reg), 16);
    return (uint16_t)x86__mm_mX_immX_and(cpu, effctvaddr, x86_rdreg16(cpu, reg), 16);
}

uint32_t x86__mm_m32_r32_and(void *cpu, moffset32_t effctvaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_and(cpu, effctvaddr, x86_rdreg32(cpu, reg), 32);
    return x86__mm_mX_immX_and(cpu, effctvaddr, x86_rdreg32(cpu, reg), 16);
}

uint8_t x86__mm_r8_m8_and(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_and(cpu, reg, x86_atomic_rdmem8(cpu, effctvaddr), 8);
    return (uint8_t)x86__mm_rX_immX_and(cpu, reg, x86_rdmem8(cpu, effctvaddr), 8);
}

uint16_t x86__mm_r16_m16_and(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_and(cpu, reg, x86_atomic_rdmem16(cpu, effctvaddr), 16);
    return (uint16_t)x86__mm_rX_immX_and(cpu, reg, x86_rdmem16(cpu, effctvaddr), 16);
}

uint32_t x86__mm_r32_m32_and(void *cpu, uint8_t reg, moffset32_t effctvaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_and(cpu, reg, x86_atomic_rdmem32(cpu, effctvaddr), 32);
    return x86__mm_rX_immX_and(cpu, reg, x86_rdmem32(cpu, effctvaddr), 32);
}


// CALL
static void x86__mm_abs_call(void *, moffset32_t);

static void x86__mm_abs_call(void *cpu, moffset32_t absladdr)
{
    x86__mm_r32_push(cpu, EIP);

    x86_cpustat_push_callstack(cpu, absladdr);

    x86_wrreg32(cpu, EIP, absladdr);
}

void x86__mm_rel16_call(void *cpu, int16_t moffset)
{
    x86__mm_abs_call(cpu, x86_rdreg32(cpu, EIP) + moffset);
}

void x86__mm_rel32_call(void *cpu, int32_t moffset)
{
    x86__mm_abs_call(cpu, x86_rdreg32(cpu, EIP) + moffset);
}

void x86__mm_r16_call(void *cpu, uint8_t reg)
{
    x86__mm_abs_call(cpu, x86_rdreg16(cpu, reg));
}

void x86__mm_r32_call(void *cpu, uint8_t reg)
{
    x86__mm_abs_call(cpu, x86_rdreg32(cpu, reg));
}

void x86__mm_m16_call(void *cpu, moffset32_t effctvaddr)
{
    x86__mm_abs_call(cpu, x86_rdmem16(cpu, effctvaddr));
}

void x86__mm_m32_call(void *cpu, moffset32_t effctvaddr)
{
    x86__mm_abs_call(cpu, x86_rdmem32(cpu, effctvaddr));
}

void x86__mm_far_ptr16_call(void *cpu, uint16_t segselector, uint16_t moffset)
{
    x86__mm_imm32_push(cpu, sign16to32(x86_rdsreg(cpu, CS)));
    x86_wrreg16(cpu, CS, segselector);
    x86__mm_abs_call(cpu, moffset);
}

void x86__mm_far_ptr32_call(void *cpu, uint16_t segselector, uint32_t moffset)
{
    x86__mm_imm32_push(cpu, sign16to32(x86_rdsreg(cpu, CS)));
    x86_wrreg16(cpu, CS, segselector);
    x86__mm_abs_call(cpu, moffset);
}


// MOV

void x86__mm_r8_r8_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrreg8(cpu, dest, x86_rdreg8(cpu, src));
}

void x86__mm_r16_r16_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrreg16(cpu, dest, x86_rdreg16(cpu, src));
}

void x86__mm_r32_r32_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrreg32(cpu, dest, x86_rdreg32(cpu, src));
}

void x86__mm_m8_r8_mov(void *cpu, moffset32_t effctvaddr, uint8_t src)
{
    x86_wrmem8(cpu, effctvaddr, x86_rdreg8(cpu, src));
}

void x86__mm_m16_r16_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_wrmem16(cpu, dest, x86_rdreg16(cpu, src));
}

void x86__mm_m32_r32_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_wrmem32(cpu, dest, x86_rdreg32(cpu, src));
}

void x86__mm_r8_m8_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_wrreg8(cpu, dest, x86_rdmem8(cpu, src));
}

void x86__mm_r16_m16_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_wrreg16(cpu, dest, x86_rdmem16(cpu, src));
}

void x86__mm_r32_m32_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_wrreg32(cpu, dest, x86_rdmem32(cpu, src));
}

void x86__mm_r16_sreg_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrreg16(cpu, dest, x86_rdsreg(cpu, src));
}

void x86__mm_m16_sreg_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_wrmem16(cpu, dest, x86_rdsreg(cpu, src));
}

void x86__mm_sreg_r16_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrsreg(cpu, dest, x86_rdreg16(cpu, src));
}

void x86__mm_sreg_m16_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_wrsreg(cpu, dest, x86_rdmem16(cpu, src));
}

void x86__mm_r8_imm8_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_wrreg8(cpu, dest, imm);
}
void x86__mm_r16_imm16_mov(void *cpu, uint8_t dest, uint16_t imm)
{
    x86_wrreg16(cpu, dest, imm);
}

void x86__mm_r32_imm32_mov(void *cpu, uint8_t dest, uint32_t imm)
{
    x86_wrreg32(cpu, dest, imm);
}

void x86__mm_m8_imm8_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_wrmem8(cpu, dest, imm);
}

void x86__mm_m16_imm16_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_wrmem16(cpu, dest, imm);
}

void x86__mm_m32_imm32_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_wrmem32(cpu, dest, imm);
}

// RET

void x86__mm_near_ret16(void *cpu)
{
    x86__mm_imm16_near_ret16(cpu, 0);
}

void x86__mm_near_ret32(void *cpu)
{
    x86__mm_imm16_near_ret32(cpu, 0);
}

void x86__mm_far_ret16(void *cpu)
{
    x86__mm_imm16_far_ret16(cpu, 0);
}

void x86__mm_far_ret32(void *cpu)
{
    x86__mm_imm16_far_ret32(cpu, 0);
}

void x86__mm_imm16_near_ret16(void *cpu, uint16_t imm)
{
    x86__mm_r16_pop(cpu, EIP);

    x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - imm);
    x86_cpustat_pop_callstack(cpu);
}

void x86__mm_imm16_near_ret32(void *cpu, uint16_t imm)
{
    x86__mm_r32_pop(cpu, EIP);

    x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - imm);
    x86_cpustat_pop_callstack(cpu);
}

void x86__mm_imm16_far_ret16(void *cpu, uint16_t imm)
{
    x86__mm_r16_pop(cpu, EIP);
    x86__mm_sreg_pop(cpu, CS);

    x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - imm);
    x86_cpustat_pop_callstack(cpu);
}

void x86__mm_imm16_far_ret32(void *cpu, uint16_t imm)
{
    x86__mm_r32_pop(cpu, EIP);
    x86__mm_sreg_pop(cpu, CS);

    x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - imm);

    x86_cpustat_pop_callstack(cpu);
}
