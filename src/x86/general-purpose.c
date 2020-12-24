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

#include "general-purpose.h"

#include "cpu.h"
#include "x86-utils.h"
#include "../tracer.h"

// XOR

static uint32_t x86__mm_rX_immX_xor(void *, uint8_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_xor(void *, moffset32_t, uint32_t, int);
static uint32_t x86__mm_mX_immX_locked_xor(void *, moffset32_t, uint32_t, int);

static uint32_t x86__mm_rX_immX_xor(void *cpu, uint8_t reg, uint32_t imm, int size)
{
    uint32_t result = 0;
    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readR8(cpu, reg) ^ lsb(imm);
        x86_writeR8(cpu, reg, lsb(result));
        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readR16(cpu, reg) ^ low16(imm);
        x86_writeR16(cpu, reg, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);
    } else {
        result = x86_readR32(cpu, reg) ^ imm;
        x86_writeR32(cpu, reg, result);
        if (signbit32(result))
            x86_setflag(cpu, SF);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    return result;
}

static uint32_t x86__mm_mX_immX_xor(void *cpu, moffset32_t vaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readM8(cpu, vaddr) ^ lsb(imm);
        x86_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readM16(cpu, vaddr) ^ low16(imm);
        x86_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);
    } else {
        result = x86_readM32(cpu, vaddr) ^ imm;
        x86_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_xor(void *cpu, moffset32_t vaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_atomic_readM8(cpu, vaddr) ^ lsb(imm);
        x86_atomic_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);
    } else if (size == 16) {
        result = x86_atomic_readM16(cpu, vaddr) ^ low16(imm);
        x86_atomic_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);
    } else {
        result = x86_atomic_readM32(cpu, vaddr) ^ imm;
        x86_atomic_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

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

uint8_t x86__mm_m8_imm8_xor(void *cpu, moffset32_t vaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_xor(cpu, vaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_xor(cpu, vaddr, imm, 8);
}

uint16_t x86__mm_m16_imm16_xor(void *cpu, moffset32_t vaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_xor(cpu, vaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_xor(cpu, vaddr, imm, 16);
}

uint32_t x86__mm_m32_imm32_xor(void *cpu, moffset32_t vaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_xor(cpu, vaddr, imm, 32);
    return x86__mm_mX_immX_xor(cpu, vaddr, imm, 32);
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
    return (uint8_t)x86__mm_rX_immX_xor(cpu, dest, x86_readR8(cpu, reg2), 8);
}

uint16_t x86__mm_r16_r16_xor(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint16_t)x86__mm_rX_immX_xor(cpu, dest, x86_readR16(cpu, reg2), 16);
}

uint32_t x86__mm_r32_r32_xor(void *cpu, uint8_t dest, uint8_t reg2)
{
    return x86__mm_rX_immX_xor(cpu, dest, x86_readR32(cpu, reg2), 32);
}

uint8_t x86__mm_m8_r8_xor(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_xor(cpu, vaddr, x86_readR8(cpu, reg), 8);
    return (uint8_t)x86__mm_mX_immX_xor(cpu, vaddr, x86_readR8(cpu, reg), 8);
}

uint16_t x86__mm_m16_r16_xor(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_xor(cpu, vaddr, x86_readR16(cpu, reg), 16);
    return (uint16_t)x86__mm_mX_immX_xor(cpu, vaddr, x86_readR16(cpu, reg), 16);
}

uint32_t x86__mm_m32_r32_xor(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_xor(cpu, vaddr, x86_readR32(cpu, reg), 32);
    return x86__mm_mX_immX_xor(cpu, vaddr, x86_readR32(cpu, reg), 32);
}

uint8_t x86__mm_r8_m8_xor(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_xor(cpu, reg, x86_atomic_readM8(cpu, vaddr), 8);
    return (uint8_t)x86__mm_rX_immX_xor(cpu, reg, x86_readM8(cpu, vaddr), 8);
}

uint16_t x86__mm_r16_m16_xor(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_xor(cpu, reg, x86_atomic_readM16(cpu, vaddr), 16);
    return (uint16_t)x86__mm_rX_immX_xor(cpu, reg, x86_readM16(cpu, vaddr), 16);
}

uint32_t x86__mm_r32_m32_xor(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_xor(cpu, reg, x86_atomic_readM32(cpu, vaddr), 32);
    return x86__mm_rX_immX_xor(cpu, reg, x86_readM32(cpu, vaddr), 32);
}

// POP

static uint32_t x86__mm_rX_popX(void *, uint8_t, int);
static uint32_t x86__mm_mX_popX(void *, moffset32_t, int);

static uint32_t x86__mm_rX_popX(void *cpu, uint8_t reg, int size)
{
    uint32_t value;

    if (size == 16) {
        value = x86_readM16(cpu, x86_readR32(cpu, ESP));
        x86_writeR16(cpu, reg, value);
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) + 4);
    } else {
        value = x86_readM32(cpu, x86_readR32(cpu, ESP));
        x86_writeR32(cpu, reg, value);
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) + 4);
    }

    return value;
}

static uint32_t x86__mm_mX_popX(void *cpu, moffset32_t vaddr, int size)
{
    uint32_t value;

    if (size == 16) {
        value = x86_readM16(cpu, x86_readR32(cpu, ESP));
        x86_writeM16(cpu, vaddr, value);
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) + 4);
    } else {
        value = x86_readM32(cpu, x86_readR32(cpu, ESP));
        x86_writeM32(cpu, vaddr, value);
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) + 4);
    }

    return value;
}

uint16_t x86__mm_sreg_pop(void *cpu, uint8_t sreg)
{
    uint16_t value = x86_readM16(cpu, ESP);

    x86_wrsreg(cpu, sreg, value);
    x86_wrsreg(cpu, ESP, x86_readR32(cpu, ESP) - 4);

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

uint32_t x86__mm_m32_pop(void *cpu, moffset32_t vaddr)
{
    return x86__mm_mX_popX(cpu, vaddr, 32);
}

uint16_t x86__mm_m16_pop(void *cpu, moffset32_t vaddr)
{
    return (uint16_t)x86__mm_mX_popX(cpu, vaddr, 16);
}

// PUSH
static void x86__mm_immX_push(void *, uint32_t, int);

static void x86__mm_immX_push(void *cpu, uint32_t imm, int size)
{
    reg32_t esp = x86_readR32(cpu, ESP);

    if (size == 16) {
        x86_writeM16(cpu, esp - 2, imm);
        x86_writeR32(cpu, ESP, esp - 2);
    } else  {
        x86_writeM32(cpu, esp - 4, imm);
        x86_writeR32(cpu, ESP, esp - 4);
    }
}

void x86__mm_r32_push(void *cpu, uint8_t reg)
{
    x86__mm_immX_push(cpu, x86_readR32(cpu, reg), 32);
}

void x86__mm_m32_push(void *cpu, moffset32_t vaddr)
{
    x86__mm_immX_push(cpu, x86_readM32(cpu, vaddr), 32);
}

void x86__mm_r16_push(void *cpu, uint8_t reg)
{
    x86__mm_immX_push(cpu, x86_readR16(cpu, reg), 16);
}

void x86__mm_m16_push(void *cpu, moffset32_t vaddr)
{
    x86__mm_immX_push(cpu, x86_readM16(cpu, vaddr), 16);
}

void x86__mm_sreg_push(void *cpu, uint8_t reg)
{
    reg16_t esp = x86_readR16(cpu, ESP);
    x86_writeM16(cpu, esp, x86_rdsreg(cpu, reg));
    x86_writeR32(cpu, ESP, esp - 2);
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

static uint32_t x86__mm_rX_immX_and(void *cpu, uint8_t reg, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readR8(cpu, reg) & sign8to32(lsb(imm));
        x86_writeR8(cpu, reg, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readR16(cpu, reg) & sign16to32(low16(imm));
        x86_writeR16(cpu, reg, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

    } else {
        result = x86_readR32(cpu, reg) & imm;
        x86_writeR32(cpu, reg, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    return result;
}

static uint32_t x86__mm_mX_immX_and(void *cpu, moffset32_t vaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readM8(cpu, vaddr) & sign8to32(lsb(imm));
        x86_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readM16(cpu, vaddr) & sign16to32(low16(imm));
        x86_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

    } else {
        result = x86_readM32(cpu, vaddr) & imm;
        x86_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_and(void *cpu, moffset32_t vaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_atomic_readM8(cpu, vaddr) & sign8to32(lsb(imm));
        x86_atomic_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_atomic_readM16(cpu, vaddr) & sign16to32(low16(imm));
        x86_atomic_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

    } else {
        result = x86_atomic_readM32(cpu, vaddr) & imm;
        x86_atomic_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

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

uint8_t x86__mm_m8_imm8_and(void *cpu, moffset32_t vaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_and(cpu, vaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_and(cpu, vaddr, imm, 8);
}

uint16_t x86__mm_m16_imm16_and(void *cpu, moffset32_t vaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_and(cpu, vaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_and(cpu, vaddr, imm, 16);
}

uint32_t x86__mm_m32_imm32_and(void *cpu, moffset32_t vaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_and(cpu, vaddr, imm, 32);
    return x86__mm_mX_immX_and(cpu, vaddr, imm, 32);
}

uint8_t x86__mm_r8_r8_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint8_t)x86__mm_rX_immX_and(cpu, dest, x86_readR8(cpu, reg2), 8);
}

uint16_t x86__mm_r16_r16_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return (uint16_t)x86__mm_rX_immX_and(cpu, dest, x86_readR16(cpu, reg2), 16);
}

uint32_t x86__mm_r32_r32_and(void *cpu, uint8_t dest, uint8_t reg2)
{
    return x86__mm_rX_immX_and(cpu, dest, x86_readR32(cpu, reg2), 32);
}

uint8_t x86__mm_m8_r8_and(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_and(cpu, vaddr, x86_readR8(cpu, reg), 8);
    return (uint8_t)x86__mm_mX_immX_and(cpu, vaddr, x86_readR8(cpu, reg), 8);
}

uint16_t x86__mm_m16_r16_and(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_and(cpu, vaddr, x86_readR16(cpu, reg), 16);
    return (uint16_t)x86__mm_mX_immX_and(cpu, vaddr, x86_readR16(cpu, reg), 16);
}

uint32_t x86__mm_m32_r32_and(void *cpu, moffset32_t vaddr, uint8_t reg, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_and(cpu, vaddr, x86_readR32(cpu, reg), 32);
    return x86__mm_mX_immX_and(cpu, vaddr, x86_readR32(cpu, reg), 16);
}

uint8_t x86__mm_r8_m8_and(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_and(cpu, reg, x86_atomic_readM8(cpu, vaddr), 8);
    return (uint8_t)x86__mm_rX_immX_and(cpu, reg, x86_readM8(cpu, vaddr), 8);
}

uint16_t x86__mm_r16_m16_and(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_and(cpu, reg, x86_atomic_readM16(cpu, vaddr), 16);
    return (uint16_t)x86__mm_rX_immX_and(cpu, reg, x86_readM16(cpu, vaddr), 16);
}

uint32_t x86__mm_r32_m32_and(void *cpu, uint8_t reg, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_and(cpu, reg, x86_atomic_readM32(cpu, vaddr), 32);
    return x86__mm_rX_immX_and(cpu, reg, x86_readM32(cpu, vaddr), 32);
}

// CALL
static void x86__mm_abs_call(void *, moffset32_t);

static void x86__mm_abs_call(void *cpu, moffset32_t vaddr)
{
    tracer_push(x86_tracer(cpu), vaddr, x86_readR32(cpu, EIP), x86_readR32(cpu, ESP));

    x86__mm_r32_push(cpu, EIP);
    x86_update_eip_absolute(cpu, vaddr);
}

void x86__mm_rel16_call(void *cpu, int16_t moffset)
{
    x86__mm_abs_call(cpu, x86_readR32(cpu, EIP) + moffset);
}

void x86__mm_rel32_call(void *cpu, int32_t moffset)
{
    x86__mm_abs_call(cpu, x86_readR32(cpu, EIP) + moffset);
}

void x86__mm_r16_call(void *cpu, uint8_t reg)
{
    x86__mm_abs_call(cpu, x86_readR16(cpu, reg));
}

void x86__mm_r32_call(void *cpu, uint8_t reg)
{
    x86__mm_abs_call(cpu, x86_readR32(cpu, reg));
}

void x86__mm_m16_call(void *cpu, moffset32_t vaddr)
{
    x86__mm_abs_call(cpu, x86_readM16(cpu, vaddr));
}

void x86__mm_m32_call(void *cpu, moffset32_t vaddr)
{
    x86__mm_abs_call(cpu, x86_readM32(cpu, vaddr));
}

void x86__mm_far_ptr16_call(void *cpu, uint16_t segselector, uint16_t moffset)
{
    tracer_push(x86_tracer(cpu), moffset, x86_readR32(cpu, EIP), x86_readR32(cpu, ESP));

    x86__mm_imm32_push(cpu, sign16to32(x86_rdsreg(cpu, CS)));
    x86_writeR16(cpu, CS, segselector);
    x86__mm_r32_push(cpu, EIP);
    x86_update_eip_absolute(cpu, moffset);
}

void x86__mm_far_ptr32_call(void *cpu, uint16_t segselector, uint32_t moffset)
{
    tracer_push(x86_tracer(cpu), moffset, x86_readR32(cpu, EIP), x86_readR32(cpu, ESP));

    x86__mm_imm32_push(cpu, sign16to32(x86_rdsreg(cpu, CS)));
    x86_writeR16(cpu, CS, segselector);
    x86__mm_r32_push(cpu, EIP);
    x86_update_eip_absolute(cpu, moffset);
}


// MOV

void x86__mm_r8_r8_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_writeR8(cpu, dest, x86_readR8(cpu, src));
}

void x86__mm_r16_r16_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_writeR16(cpu, dest, x86_readR16(cpu, src));
}

void x86__mm_r32_r32_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_writeR32(cpu, dest, x86_readR32(cpu, src));
}

void x86__mm_m8_r8_mov(void *cpu, moffset32_t vaddr, uint8_t src)
{
    x86_writeM8(cpu, vaddr, x86_readR8(cpu, src));
}

void x86__mm_m16_r16_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_writeM16(cpu, dest, x86_readR16(cpu, src));
}

void x86__mm_m32_r32_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_writeM32(cpu, dest, x86_readR32(cpu, src));
}

void x86__mm_r8_m8_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_writeR8(cpu, dest, x86_readM8(cpu, src));
}

void x86__mm_r16_m16_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_writeR16(cpu, dest, x86_readM16(cpu, src));
}

void x86__mm_r32_m32_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_writeR32(cpu, dest, x86_readM32(cpu, src));
}

void x86__mm_r16_sreg_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_writeR16(cpu, dest, x86_rdsreg(cpu, src));
}

void x86__mm_m16_sreg_mov(void *cpu, moffset32_t dest, uint8_t src)
{
    x86_writeM16(cpu, dest, x86_rdsreg(cpu, src));
}

void x86__mm_sreg_r16_mov(void *cpu, uint8_t dest, uint8_t src)
{
    x86_wrsreg(cpu, dest, x86_readR16(cpu, src));
}

void x86__mm_sreg_m16_mov(void *cpu, uint8_t dest, moffset32_t src)
{
    x86_wrsreg(cpu, dest, x86_readM16(cpu, src));
}

void x86__mm_r8_imm8_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_writeR8(cpu, dest, imm);
}
void x86__mm_r16_imm16_mov(void *cpu, uint8_t dest, uint16_t imm)
{
    x86_writeR16(cpu, dest, imm);
}

void x86__mm_r32_imm32_mov(void *cpu, uint8_t dest, uint32_t imm)
{
    x86_writeR32(cpu, dest, imm);
}

void x86__mm_m8_imm8_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_writeM8(cpu, dest, imm);
}

void x86__mm_m16_imm16_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_writeM16(cpu, dest, imm);
}

void x86__mm_m32_imm32_mov(void *cpu, uint8_t dest, uint8_t imm)
{
    x86_writeM32(cpu, dest, imm);
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

    x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - imm);
    tracer_pop(x86_tracer(cpu));
}

void x86__mm_imm16_near_ret32(void *cpu, uint16_t imm)
{
    x86__mm_r32_pop(cpu, EIP);

    x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - imm);
    tracer_pop(x86_tracer(cpu));
}

void x86__mm_imm16_far_ret16(void *cpu, uint16_t imm)
{
    x86__mm_r16_pop(cpu, EIP);
    x86__mm_sreg_pop(cpu, CS);

    x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - imm);
    tracer_pop(x86_tracer(cpu));
}

void x86__mm_imm16_far_ret32(void *cpu, uint16_t imm)
{
    x86__mm_r32_pop(cpu, EIP);
    x86__mm_sreg_pop(cpu, CS);

    x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - imm);

    tracer_pop(x86_tracer(cpu));
}


// ADD

static uint32_t x86__mm_mX_immX_add(void *, moffset32_t, uint32_t, size_t);
static uint32_t x86__mm_mX_immX_locked_add(void *, moffset32_t, uint32_t, size_t);
static uint32_t x86__mm_rX_immX_add(void *, uint8_t, uint32_t, size_t);


static uint32_t x86__mm_mX_immX_add(void *cpu, moffset32_t vaddr, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readM8(cpu, vaddr);
        result = lsb(operand1) + lsb(imm);
        x86_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readM16(cpu, vaddr);
        result = low16(operand1) + low16(imm);
        x86_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readM32(cpu, vaddr);
        result = operand1 + imm;
        x86_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxcarry(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_add(void *cpu, moffset32_t vaddr, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_atomic_readM8(cpu, vaddr);
        result = lsb(operand1) + lsb(imm);

        x86_atomic_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_atomic_readM16(cpu, vaddr);
        result = low16(operand1) + low16(imm);
        x86_atomic_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_atomic_readM32(cpu, vaddr);
        result = operand1 + imm;
        x86_atomic_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxcarry(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

static uint32_t x86__mm_rX_immX_add(void *cpu, uint8_t dest, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readR8(cpu, dest);
        result = lsb(operand1) + lsb(imm);

        x86_writeR8(cpu, dest, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readR16(cpu, dest);
        result = low16(operand1) + low16(imm);
        x86_writeR16(cpu, dest, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readR32(cpu, dest);
        result = operand1 + imm;
        x86_writeR32(cpu, dest, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxcarry(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

uint8_t x86__mm_al_imm8_add(void *cpu, uint8_t imm)
{
    return x86__mm_r8_imm8_add(cpu, AL, imm);
}

uint16_t x86__mm_ax_imm16_add(void *cpu, uint16_t imm)
{
    return x86__mm_r16_imm16_add(cpu, AX, imm);
}

uint32_t x86__mm_eax_imm32_add(void *cpu, uint32_t imm)
{
    return x86__mm_r32_imm32_add(cpu, EAX, imm);
}

uint8_t x86__mm_m8_imm8_add(void *cpu, moffset32_t vaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_add(cpu, vaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_add(cpu, vaddr, imm, 8);
}

uint8_t x86__mm_r8_imm8_add(void *cpu, uint8_t dest, uint8_t imm)
{
    return (uint8_t)x86__mm_rX_immX_add(cpu, dest, imm, 8);
}

uint16_t x86__mm_m16_imm16_add(void *cpu, moffset32_t vaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_add(cpu, vaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_add(cpu, vaddr, imm, 16);
}

uint16_t x86__mm_r16_imm16_add(void *cpu, uint8_t dest, uint16_t imm)
{
    return (uint16_t)x86__mm_rX_immX_add(cpu, dest, imm, 16);
}

uint32_t x86__mm_m32_imm32_add(void *cpu, moffset32_t vaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_add(cpu, vaddr, imm, 32);
    return x86__mm_mX_immX_add(cpu, vaddr, imm, 32);
}

uint32_t x86__mm_r32_imm32_add(void *cpu, uint8_t dest, uint32_t imm)
{
    return x86__mm_rX_immX_add(cpu, dest, imm, 32);
}

uint8_t x86__mm_r8_r8_add(void *cpu, uint8_t dest, uint8_t src)
{
    return (uint8_t)x86__mm_rX_immX_add(cpu, dest, x86_readR8(cpu, src), 8);
}

uint8_t x86__mm_m8_r8_add(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_add(cpu, vaddr, x86_readR8(cpu, src), 8);
    return (uint8_t)x86__mm_mX_immX_add(cpu, vaddr, x86_readR8(cpu, src), 8);
}

uint8_t x86__mm_r8_m8_add(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_add(cpu, dest, x86_atomic_readM8(cpu, vaddr), 8);
    return (uint8_t)x86__mm_rX_immX_add(cpu, vaddr, x86_readM8(cpu, vaddr), 8);
}

uint16_t x86__mm_r16_r16_add(void *cpu, uint8_t dest, uint8_t src)
{
    return (u_int16_t)x86__mm_rX_immX_add(cpu, dest, x86_readR16(cpu, src), 16);
}

uint16_t x86__mm_m16_r16_add(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_add(cpu, vaddr, x86_readR16(cpu, src), 16);
    return (uint16_t)x86__mm_mX_immX_add(cpu, vaddr, x86_readR16(cpu, src), 16);
}

uint16_t x86__mm_r16_m16_add(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_add(cpu, dest, x86_atomic_readM16(cpu, vaddr), 16);
    return (uint16_t)x86__mm_rX_immX_add(cpu, vaddr, x86_readM16(cpu, vaddr), 16);
}

uint32_t x86__mm_r32_r32_add(void *cpu, uint8_t dest, uint8_t src)
{
    return x86__mm_rX_immX_add(cpu, dest, x86_readR32(cpu, src), 32);
}

uint32_t x86__mm_m32_r32_add(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_add(cpu, vaddr, x86_readR32(cpu, src), 32);
    return x86__mm_mX_immX_add(cpu, vaddr, x86_readR32(cpu, src), 32);
}

uint32_t x86__mm_r32_m32_add(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_add(cpu, dest, x86_atomic_readM32(cpu, vaddr), 32);
    return x86__mm_rX_immX_add(cpu, vaddr, x86_readM32(cpu, vaddr), 32);
}


// LEA


void x86__mm_r16_m_lea(void *cpu, uint8_t dest, moffset16_t vaddr)
{
    x86_writeR16(cpu, dest, vaddr);
}

void x86__mm_r32_m_lea(void *cpu, uint8_t dest, moffset32_t vaddr)
{
    x86_writeR32(cpu, dest, vaddr);
}


// SUB

static uint32_t x86__mm_mX_immX_sub(void *, moffset32_t, uint32_t, size_t);
static uint32_t x86__mm_mX_immX_locked_sub(void *, moffset32_t, uint32_t, size_t);
static uint32_t x86__mm_rX_immX_sub(void *, uint8_t, uint32_t, size_t);

static uint32_t x86__mm_mX_immX_sub(void *cpu, moffset32_t vaddr, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readM8(cpu, vaddr);
        result = lsb(operand1) - lsb(imm);
        x86_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readM16(cpu, vaddr);
        result = low16(operand1) - low16(imm);
        x86_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readM32(cpu, vaddr);
        result = operand1 - imm;
        x86_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxborrow(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

static uint32_t x86__mm_mX_immX_locked_sub(void *cpu, moffset32_t vaddr, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_atomic_readM8(cpu, vaddr);
        result = lsb(operand1) - lsb(imm);

        x86_atomic_writeM8(cpu, vaddr, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_atomic_readM16(cpu, vaddr);
        result = low16(operand1) - low16(imm);
        x86_atomic_writeM16(cpu, vaddr, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_atomic_readM32(cpu, vaddr);
        result = operand1 - imm;
        x86_atomic_writeM32(cpu, vaddr, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxborrow(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

static uint32_t x86__mm_rX_immX_sub(void *cpu, uint8_t dest, uint32_t imm, size_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readR8(cpu, dest);
        result = lsb(operand1) - lsb(imm);

        x86_writeR8(cpu, dest, lsb(result));

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readR16(cpu, dest);
        result = low16(operand1) - low16(imm);
        x86_writeR16(cpu, dest, low16(result));

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readR32(cpu, dest);
        result = operand1 - imm;
        x86_writeR32(cpu, dest, result);

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxborrow(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

uint8_t x86__mm_al_imm8_sub(void *cpu, uint8_t imm)
{
    return x86__mm_r8_imm8_sub(cpu, AL, imm);
}

uint16_t x86__mm_ax_imm16_sub(void *cpu, uint16_t imm)
{
    return x86__mm_r16_imm16_sub(cpu, AX, imm);
}

uint32_t x86__mm_eax_imm32_sub(void *cpu, uint32_t imm)
{
    return x86__mm_r32_imm32_sub(cpu, EAX, imm);
}

uint8_t x86__mm_m8_imm8_sub(void *cpu, moffset32_t vaddr, uint8_t imm, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_sub(cpu, vaddr, imm, 8);
    return (uint8_t)x86__mm_mX_immX_sub(cpu, vaddr, imm, 8);
}

uint8_t x86__mm_r8_imm8_sub(void *cpu, uint8_t dest, uint8_t imm)
{
    return (uint8_t)x86__mm_rX_immX_sub(cpu, dest, imm, 8);
}

uint16_t x86__mm_m16_imm16_sub(void *cpu, moffset32_t vaddr, uint16_t imm, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_sub(cpu, vaddr, imm, 16);
    return (uint16_t)x86__mm_mX_immX_sub(cpu, vaddr, imm, 16);
}

uint16_t x86__mm_r16_imm16_sub(void *cpu, uint8_t dest, uint16_t imm)
{
    return (uint16_t)x86__mm_rX_immX_sub(cpu, dest, imm, 16);
}

uint32_t x86__mm_m32_imm32_sub(void *cpu, moffset32_t vaddr, uint32_t imm, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_sub(cpu, vaddr, imm, 32);
    return x86__mm_mX_immX_sub(cpu, vaddr, imm, 32);
}

uint32_t x86__mm_r32_imm32_sub(void *cpu, uint8_t dest, uint32_t imm)
{
    return x86__mm_rX_immX_sub(cpu, dest, imm, 32);
}

uint8_t x86__mm_r8_r8_sub(void *cpu, uint8_t dest, uint8_t src)
{
    return (uint8_t)x86__mm_rX_immX_sub(cpu, dest, x86_readR8(cpu, src), 8);
}

uint8_t x86__mm_m8_r8_sub(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_mX_immX_locked_sub(cpu, vaddr, x86_readR8(cpu, src), 8);
    return (uint8_t)x86__mm_mX_immX_sub(cpu, vaddr, x86_readR8(cpu, src), 8);
}

uint8_t x86__mm_r8_m8_sub(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint8_t)x86__mm_rX_immX_sub(cpu, dest, x86_atomic_readM8(cpu, vaddr), 8);
    return (uint8_t)x86__mm_rX_immX_sub(cpu, vaddr, x86_readM8(cpu, vaddr), 8);
}

uint16_t x86__mm_r16_r16_sub(void *cpu, uint8_t dest, uint8_t src)
{
    return (u_int16_t)x86__mm_rX_immX_sub(cpu, dest, x86_readR16(cpu, src), 16);
}

uint16_t x86__mm_m16_r16_sub(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_mX_immX_locked_sub(cpu, vaddr, x86_readR16(cpu, src), 16);
    return (uint16_t)x86__mm_mX_immX_sub(cpu, vaddr, x86_readR16(cpu, src), 16);
}

uint16_t x86__mm_r16_m16_sub(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return (uint16_t)x86__mm_rX_immX_sub(cpu, dest, x86_atomic_readM16(cpu, vaddr), 16);
    return (uint16_t)x86__mm_rX_immX_sub(cpu, vaddr, x86_readM16(cpu, vaddr), 16);
}

uint32_t x86__mm_r32_r32_sub(void *cpu, uint8_t dest, uint8_t src)
{
    return x86__mm_rX_immX_sub(cpu, dest, x86_readR32(cpu, src), 32);
}

uint32_t x86__mm_m32_r32_sub(void *cpu, moffset32_t vaddr, uint8_t src, _Bool lock)
{
    if (lock)
        return x86__mm_mX_immX_locked_sub(cpu, vaddr, x86_readR32(cpu, src), 32);
    return x86__mm_mX_immX_sub(cpu, vaddr, x86_readR32(cpu, src), 32);
}

uint32_t x86__mm_r32_m32_sub(void *cpu, uint8_t dest, moffset32_t vaddr, _Bool lock)
{
    if (lock)
        return x86__mm_rX_immX_sub(cpu, dest, x86_atomic_readM32(cpu, vaddr), 32);
    return x86__mm_rX_immX_sub(cpu, vaddr, x86_readM32(cpu, vaddr), 32);
}


// TEST

static void x86__mm_rX_immX_test(void *, uint8_t, uint32_t, int);
static void x86__mm_mX_immX_test(void *, moffset32_t, uint32_t, int);

static void x86__mm_rX_immX_test(void *cpu, uint8_t reg, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readR8(cpu, reg) & sign8to32(lsb(imm));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readR16(cpu, reg) & sign16to32(low16(imm));

        if (signbit16(result))
            x86_setflag(cpu, SF);

    } else {
        result = x86_readR32(cpu, reg) & imm;

        if (signbit32(result))
            x86_setflag(cpu, SF);
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);
}

static void x86__mm_mX_immX_test(void *cpu, moffset32_t vaddr, uint32_t imm, int size)
{
    uint32_t result;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        result = x86_readM8(cpu, vaddr) & sign8to32(lsb(imm));

        if (signbit8(result))
            x86_setflag(cpu, SF);

    } else if (size == 16) {
        result = x86_readM16(cpu, vaddr) & sign16to32(low16(imm));

        if (signbit16(result))
            x86_setflag(cpu, SF);

    } else {
        result = x86_readM32(cpu, vaddr) & imm;

        if (signbit32(result))
            x86_setflag(cpu, SF);

    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);
}

void x86__mm_al_imm8_test(void *cpu, uint8_t imm)
{

    x86__mm_r8_imm8_test(cpu, AL, imm);
}

void x86__mm_ax_imm16_test(void *cpu, uint16_t imm)
{
    x86__mm_r16_imm16_test(cpu, AX, imm);
}

void x86__mm_eax_imm32_test(void *cpu, uint32_t imm)
{
    x86__mm_r32_imm32_test(cpu, EAX, imm);
}

void x86__mm_r8_imm8_test(void *cpu, uint8_t reg, uint8_t imm)
{
    x86__mm_rX_immX_test(cpu, reg, imm, 8);
}

void x86__mm_r16_imm16_test(void *cpu, uint8_t reg, uint16_t imm)
{
    x86__mm_rX_immX_test(cpu, reg, imm, 16);
}

void x86__mm_r32_imm32_test(void *cpu, uint8_t reg, uint32_t imm)
{
    x86__mm_rX_immX_test(cpu, reg, imm, 32);
}

void x86__mm_m8_imm8_test(void *cpu, moffset32_t vaddr, uint8_t imm)
{
    x86__mm_mX_immX_test(cpu, vaddr, imm, 8);
}

void x86__mm_m16_imm16_test(void *cpu, moffset32_t vaddr, uint16_t imm)
{
    x86__mm_mX_immX_test(cpu, vaddr, imm, 16);
}

void x86__mm_m32_imm32_test(void *cpu, moffset32_t vaddr, uint32_t imm)
{
    x86__mm_mX_immX_test(cpu, vaddr, imm, 32);
}

void x86__mm_r8_r8_test(void *cpu, uint8_t dest, uint8_t reg2)
{
    x86__mm_rX_immX_test(cpu, dest, x86_readR8(cpu, reg2), 8);
}

void x86__mm_r16_r16_test(void *cpu, uint8_t dest, uint8_t reg2)
{
    x86__mm_rX_immX_test(cpu, dest, x86_readR16(cpu, reg2), 16);
}

void x86__mm_r32_r32_test(void *cpu, uint8_t dest, uint8_t reg2)
{
    x86__mm_rX_immX_test(cpu, dest, x86_readR32(cpu, reg2), 32);
}

void x86__mm_m8_r8_test(void *cpu, moffset32_t vaddr, uint8_t reg)
{
    x86__mm_mX_immX_test(cpu, vaddr, x86_readR8(cpu, reg), 8);
}

void x86__mm_m16_r16_test(void *cpu, moffset32_t vaddr, uint8_t reg)
{
    x86__mm_mX_immX_test(cpu, vaddr, x86_readR16(cpu, reg), 16);
}

void x86__mm_m32_r32_test(void *cpu, moffset32_t vaddr, uint8_t reg)
{
    x86__mm_mX_immX_test(cpu, vaddr, x86_readR32(cpu, reg), 16);
}


// CMP

static _Bool x86__mm_rX_immX_cmp(void *, uint8_t, uint32_t, uint8_t);
static _Bool x86__mm_mX_immX_cmp(void *, moffset32_t, uint32_t, uint8_t);

static _Bool x86__mm_rX_immX_cmp(void *cpu, uint8_t src, uint32_t imm, uint8_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readR8(cpu, src);
        result = lsb(operand1) - lsb(imm);

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readR16(cpu, src);
        result = low16(operand1) - low16(imm);

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readR32(cpu, src);
        result = operand1 - imm;

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxborrow(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

static _Bool x86__mm_mX_immX_cmp(void *cpu, moffset32_t src, uint32_t imm, uint8_t size)
{
    uint32_t result = 0;
    uint32_t operand1;

    x86_clearflag(cpu, OF);
    x86_clearflag(cpu, SF);
    x86_clearflag(cpu, ZF);
    x86_clearflag(cpu, AF);
    x86_clearflag(cpu, CF);
    x86_clearflag(cpu, PF);

    if (size == 8) {
        operand1 = x86_readM8(cpu, src);
        result = lsb(operand1) - lsb(imm);

        if (signbit8(result))
            x86_setflag(cpu, SF);

        if (overflow8(lsb(operand1), lsb(imm), lsb(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else if (size == 16) {
        operand1 = x86_readM16(cpu, src);
        result = low16(operand1) - low16(imm);

        if (signbit16(result))
            x86_setflag(cpu, SF);

        if (overflow16(low16(operand1), low16(imm), low16(result))) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }

    } else {
        operand1 = x86_readM32(cpu, src);
        result = operand1 - imm;

        if (signbit32(result))
            x86_setflag(cpu, SF);

        if (overflow32(operand1, imm, result)) {
            x86_setflag(cpu, OF);
            x86_setflag(cpu, CF);
        }
    }

    if (!result)
        x86_setflag(cpu, ZF);

    if (parity_even(result))
        x86_setflag(cpu, PF);

    if (auxborrow(operand1, imm))
        x86_setflag(cpu, AF);

    return result;
}

_Bool x86__mm_al_imm8_cmp(void *cpu, uint8_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, AL, imm, 8);
}

_Bool x86__mm_ax_imm16_cmp(void *cpu, uint16_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, AX, imm, 16);
}

_Bool x86__mm_eax_imm32_cmp(void *cpu, uint32_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, EAX, imm, 32);
}

_Bool x86__mm_r8_imm8_cmp(void *cpu, uint8_t src, uint8_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, src, imm, 8);
}

_Bool x86__mm_r16_imm16_cmp(void *cpu, uint8_t src, uint16_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, src, imm, 16);
}

_Bool x86__mm_r32_imm32_cmp(void *cpu, uint8_t src, uint32_t imm)
{
    return x86__mm_rX_immX_cmp(cpu, src, imm, 32);
}

_Bool x86__mm_m8_imm8_cmp(void *cpu, moffset32_t src, uint8_t imm)
{
    return x86__mm_mX_immX_cmp(cpu, src, imm, 8);
}

_Bool x86__mm_m16_imm16_cmp(void *cpu, moffset32_t src, uint16_t imm)
{
    return x86__mm_mX_immX_cmp(cpu, src, imm, 16);
}

_Bool x86__mm_m32_imm32_cmp(void *cpu, moffset32_t src, uint32_t imm)
{
    return x86__mm_mX_immX_cmp(cpu, src, imm, 32);
}

_Bool x86__mm_r8_r8_cmp(void *cpu, uint8_t src1, uint8_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readR8(cpu, src2), 8);
}

_Bool x86__mm_m8_r8_cmp(void *cpu, moffset32_t src1, uint8_t src2)
{
    return x86__mm_mX_immX_cmp(cpu, src1, x86_readR8(cpu, src2), 8);
}

_Bool x86__mm_r16_r16_cmp(void *cpu, uint8_t src1, uint8_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readR16(cpu, src2), 16);
}

_Bool x86__mm_m16_r16_cmp(void *cpu, moffset32_t src1, uint8_t src2)
{
    return x86__mm_mX_immX_cmp(cpu, src1, x86_readR16(cpu, src2), 16);
}

_Bool x86__mm_r32_r32_cmp(void *cpu, uint8_t src1, uint8_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readR32(cpu, src2), 32);
}

_Bool x86__mm_m32_r32_cmp(void *cpu, moffset32_t src1, uint8_t src2)
{
    return x86__mm_mX_immX_cmp(cpu, src1, x86_readR32(cpu, src2), 32);
}

_Bool x86__mm_r8_m8_cmp(void *cpu, uint8_t src1, moffset32_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readM8(cpu, src2), 8);
}

_Bool x86__mm_r16_m16_cmp(void *cpu, uint8_t src1, moffset32_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readM16(cpu, src2), 16);
}

_Bool x86__mm_r32_m32_cmp(void *cpu, uint8_t src1, moffset32_t src2)
{
    return x86__mm_rX_immX_cmp(cpu, src1, x86_readM32(cpu, src2), 32);
}


// MOVZX

void x86__mm_r16_r8_movzx(void *cpu, uint8_t dest, uint8_t src)
{
    x86__mm_r16_imm16_mov(cpu, dest, zeroxtnd8to16(x86_readR8(cpu, src)));
}

void x86__mm_r32_r8_movzx(void *cpu, uint8_t dest, uint8_t src)
{
    x86__mm_r32_imm32_mov(cpu, dest, zeroxtnd8(x86_readR8(cpu, src)));
}

void x86__mm_r16_m8_movzx(void *cpu, uint8_t dest, moffset32_t src)
{
    x86__mm_r16_imm16_mov(cpu, dest, zeroxtnd8to16(x86_readM8(cpu, src)));
}

void x86__mm_r32_m8_movzx(void *cpu, uint8_t dest, moffset32_t src)
{
    x86__mm_r32_imm32_mov(cpu, dest, zeroxtnd8(x86_readM8(cpu, src)));
}

void x86__mm_r32_r16_movzx(void *cpu, uint8_t dest, uint8_t src)
{
    x86__mm_r32_imm32_mov(cpu, dest, zeroxtnd16(x86_readR16(cpu, src)));
}

void x86__mm_r32_m16_movzx(void *cpu, uint8_t dest, moffset32_t src)
{
    x86__mm_r32_imm32_mov(cpu, dest, zeroxtnd16(x86_readM16(cpu, src)));
}
