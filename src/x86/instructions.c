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
 *      x86 instructions as is described in the intel manual vol. 2.
 */

#include "../system.h"

#include "instructions.h"

#include "cpu.h"
#include "x86-utils.h"
#include "disassembler.h"
#include "general-purpose.h"


void x86_aaa(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aad(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_add(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, low16(data.moffset));

    switch (data.opc) {
        case 0x04:  // ADD AL, imm8
            x86__mm_al_imm8_add(cpu, lsb(data.imm1));
            break;
        case 0x05:  // ADD eAX, imm32  ADD AX, imm16
            if (data.oprsz_pfx)
                x86__mm_ax_imm16_add(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_add(cpu, data.imm1);
            break;
        case 0x82:
        case 0x80:  // ADD rm8, imm8
            if (vaddr)
                x86__mm_m8_imm8_add(cpu, vaddr, lsb(data.imm1), data.lock);
            else
                x86__mm_r8_imm8_add(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0x81:  // ADD rm32, imm32  ADD rm16, imm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_add(cpu, vaddr, low16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_add(cpu, vaddr, data.imm1, data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_add(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                else
                    x86__mm_r32_imm32_add(cpu, effctvregister(data.modrm, 32), data.imm1);
            }
            break;
        case 0x83:  // ADD rm32, imm8   ADD rm16, imm8
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_add(cpu, vaddr, zeroxtnd8to16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_add(cpu, vaddr, zeroxtnd8(data.imm1), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_add(cpu, effctvregister(data.modrm, 16), zeroxtnd8to16(data.imm1));
                else
                    x86__mm_r32_imm32_add(cpu, effctvregister(data.modrm, 32), zeroxtnd8(data.imm1));
            }
            break;

        case 0x00:  // ADD rm8, r8
            if (vaddr)
                x86__mm_m8_imm8_add(cpu, vaddr, reg(data.modrm), data.lock);
            else
                x86__mm_r8_imm8_add(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x01:  // ADD rm32, r32    ADD rm16, r16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_add(cpu, vaddr, reg(data.modrm), data.lock);
                else
                    x86__mm_m32_r32_add(cpu, vaddr, reg(data.modrm), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_add(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_add(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;
        case 0x02:  // ADD r8, rm8
            if (vaddr)
                x86__mm_r8_m8_add(cpu, reg(data.modrm), vaddr, data.lock);
            else
                x86__mm_r8_r8_add(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            break;
        case 0x03:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m16_add(cpu, reg(data.modrm), vaddr, data.lock);
                else
                    x86__mm_r32_m32_add(cpu, reg(data.modrm), vaddr, data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_add(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                else
                    x86__mm_r32_r32_add(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
            break;
    }
}


void x86_aam(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aas(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_adc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_adcx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addsubpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_addsubps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_adox(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aesdec(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aesdeclast(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aesenc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aesenclast(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aesimc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_aeskeygenassist(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_and(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x24:  // AND AL, imm8
            x86__mm_al_imm8_and(cpu, lsb(data.imm1));
            break;
        case 0x25: // AND EAX, imm32   AX, imm16
            if (data.oprsz_pfx)
                x86__mm_ax_imm16_and(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_and(cpu, data.imm1);
            break;
        case 0x80:  // AND r/m8, imm8
            if (vaddr)
                x86__mm_m8_imm8_and(cpu, vaddr, lsb(data.imm1), data.lock);
            else
                x86__mm_r8_imm8_and(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0x81: // AND r/m32, imm32      AND r/m16, imm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_and(cpu, vaddr, low16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_and(cpu, vaddr, data.imm1, data.lock);
            } else {
                if (data.oprsz_pfx) {
                    x86__mm_r16_imm16_and(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                } else {
                    x86__mm_r32_imm32_and(cpu, effctvregister(data.modrm, 32), data.imm1);
                }
            }
            break;
        case 0x83:  // AND r/m32, imm8  AND r/m16, imm8
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_and(cpu, vaddr, sign8to16(lsb(data.imm1)), data.lock);
                else
                    x86__mm_m32_imm32_and(cpu, vaddr, sign8to32(lsb(data.imm1)), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_and(cpu, effctvregister(data.modrm, 16), sign8to16(lsb(data.imm1)));
                else
                    x86__mm_r32_imm32_and(cpu, effctvregister(data.modrm, 32), sign8to32(lsb(data.imm1)));
            }
            break;
        case 0x20:  // AND r/m8, r8
            if (vaddr)
                x86__mm_m8_r8_and(cpu, vaddr, reg(data.modrm), data.lock);
            else
                x86__mm_r8_r8_and(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x21:  // AND r/m32, r32   AND r/m16, r16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_and(cpu, vaddr, reg(data.modrm), data.lock);
                else
                    x86__mm_m32_r32_and(cpu, vaddr, reg(data.modrm), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_and(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_and(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;
        case 0x22:  // AND r8, r/m8
            if (vaddr)
                x86__mm_r8_m8_and(cpu, reg(data.modrm), vaddr, data.lock);
            else
                x86__mm_r8_r8_and(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            break;
        case 0x23: // AND r32, r/m32    AND r16, r/m16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m16_and(cpu, reg(data.modrm), vaddr, data.lock);
                else
                    x86__mm_r32_m32_and(cpu, reg(data.modrm), vaddr, data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_and(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                else
                    x86__mm_r32_r32_and(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
            break;

    }
}


void x86_andpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_andps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_andnpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_andnps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_arpl(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_blendpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_blendps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_blendvpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_blendvps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndcl(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndcu(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndcn(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndldx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndmk(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndmov(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bndstx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bound(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bsf(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bsr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bswap(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_btc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_btr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_bts(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_call(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);


    switch (data.opc) {
        case 0xE8:  // CALL rel32   CALL rel16
            if (data.oprsz_pfx)
                x86__mm_rel16_call(cpu, zeroxtnd16(low16(data.imm1)));
            else
                x86__mm_rel32_call(cpu, data.imm1);
            break;
        case 0xFF:
            if (data.ext == 2) {    // FF /2 CALL r/m32 CALL r/m16
                if (vaddr) {
                    if (data.oprsz_pfx)
                        x86__mm_m16_call(cpu, vaddr);
                    else
                        x86__mm_m32_call(cpu, vaddr);
                } else {
                    if (data.oprsz_pfx)
                        x86__mm_r16_call(cpu, effctvregister(data.modrm, 16));
                    else
                        x86__mm_r32_call(cpu, effctvregister(data.modrm, 32));
                }
            } else {    // FF /3 CALL m16:32  CALL m16:16
                if (data.oprsz_pfx)
                    x86__mm_far_ptr16_call(cpu, x86_readM16(cpu, vaddr + 2), x86_readM16(cpu, vaddr));
                else
                    x86__mm_far_ptr32_call(cpu, x86_readM16(cpu, vaddr + 4), x86_readM32(cpu, vaddr));
            }
            break;
        case 0x9A:
            if (data.oprsz_pfx)
                x86__mm_far_ptr16_call(cpu, high16(data.imm1), low16(data.imm1));
            else
                x86__mm_far_ptr32_call(cpu, low16(data.imm1), data.imm2);
            break;
    }

}


void x86_cbw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_clac(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_clc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_clflush(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cli(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_clts(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_cmovcc(void *cpu, struct exec_data data)
{
    _Bool condition_is_true = 0;
    moffset32_t vaddr;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x47:  // CMOVA
            condition_is_true = x86_flag_off(cpu, CF) && x86_flag_off(cpu, ZF);
            break;
        case 0x43:  // CMOVAE
            condition_is_true = x86_flag_off(cpu, CF);
            break;
        case 0x42:  // CMOVB
            condition_is_true = x86_flag_on(cpu, CF);
            break;
        case 0x46:  // CMOVBE
            condition_is_true = x86_flag_on(cpu, CF) && x86_flag_on(cpu, ZF);
            break;
        case 0x44:  // CMOVE
            condition_is_true = x86_flag_on(cpu, ZF);
            break;
        case 0x4F:  // CMOVG
            condition_is_true = x86_flag_off(cpu, ZF) && ((x86_flag_on(cpu, SF) && x86_flag_on(cpu, OF)) || (x86_flag_off(cpu, SF) && x86_flag_off(cpu, OF)));
            break;
        case 0x4D:  // CMOVGE
            condition_is_true = (x86_flag_on(cpu, SF) && x86_flag_on(cpu, OF)) || (x86_flag_off(cpu, SF) && x86_flag_off(cpu, OF));
            break;
        case 0x4C:  // CMOVL
            condition_is_true = (x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF)) || (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF));
            break;
        case 0x4E:  // CMOVLE
            condition_is_true = x86_flag_on(cpu, ZF) && ((x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF)) || (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF)));
            break;
        case 0x45:  // CMOVNE
            condition_is_true = x86_flag_off(cpu, ZF);
            break;
        case 0x41:  // CMOVNO
            condition_is_true = x86_flag_off(cpu, OF);
            break;
        case 0x4B:  // CMOVNP
            condition_is_true = x86_flag_off(cpu, PF);
            break;
        case 0x49:  // CMOVNS
            condition_is_true = x86_flag_off(cpu, SF);
            break;
        case 0x40:  // CMOVO
            condition_is_true = x86_flag_on(cpu, OF);
            break;
        case 0x4A:  // CMOVP
            condition_is_true = x86_flag_on(cpu, PF);
            break;
        case 0x48:  // CMOVS
            condition_is_true = x86_flag_on(cpu, SF);
            break;
    }

    if (condition_is_true) {
        if (vaddr) {
            if (data.oprsz_pfx) {
                if (x86_flag_off(cpu, SF))
                    x86__mm_r16_m16_mov(cpu, reg(data.modrm), vaddr);
            } else {
                if (x86_flag_off(cpu, SF))
                    x86__mm_r32_m32_mov(cpu, reg(data.modrm), vaddr);
            }
        } else {
            if (data.oprsz_pfx) {
                if (x86_flag_off(cpu, SF))
                    x86__mm_r16_r16_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
            } else {
                if (x86_flag_off(cpu, SF))
                    x86__mm_r32_r32_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
        }
    }
}


void x86_mm_cmp(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x3C:
            x86__mm_al_imm8_cmp(cpu, lsb(data.imm1));
            break;
        case 0x3D:
            if (data.oprsz_pfx)
                x86__mm_ax_imm16_cmp(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_cmp(cpu, data.imm1);
            break;
        case 0x82:
        case 0x80:
            if (vaddr)
                x86__mm_m8_imm8_cmp(cpu, vaddr, lsb(data.imm1));
            else
                x86__mm_r8_imm8_cmp(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0x81:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_cmp(cpu, vaddr, low16(data.imm1));
                else
                    x86__mm_m32_imm32_cmp(cpu, vaddr, data.imm1);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_cmp(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                else
                    x86__mm_r32_imm32_cmp(cpu, effctvregister(data.modrm, 32), data.imm1);
            }
            break;
        case 0x83:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_cmp(cpu, vaddr, sign8to32(lsb(data.imm1)));
                else
                    x86__mm_m32_imm32_cmp(cpu, vaddr, sign8to32(lsb(data.imm1)));
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_cmp(cpu, effctvregister(data.modrm, 16), sign8to32(lsb(data.imm1)));
                else
                    x86__mm_r32_imm32_cmp(cpu, effctvregister(data.modrm, 32), sign8to32(lsb(data.imm1)));
            }
            break;
        case 0x38:
            if (vaddr)
                x86__mm_m8_r8_cmp(cpu, vaddr, reg(data.modrm));
            else
                x86__mm_r8_r8_cmp(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x39:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_cmp(cpu, vaddr, reg(data.modrm));
                else
                    x86__mm_m32_r32_cmp(cpu, vaddr, reg(data.modrm));
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_cmp(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_cmp(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;
        case 0x3A:
            if (vaddr)
                x86__mm_r8_m8_cmp(cpu, reg(data.modrm), vaddr);
            else
                x86__mm_r8_r8_cmp(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            break;
        case 0x3B:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m16_cmp(cpu, reg(data.modrm), vaddr);
                else
                    x86__mm_r32_m32_cmp(cpu, reg(data.modrm), vaddr);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_cmp(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                else                                        
                    x86__mm_r32_r32_cmp(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
            break;
    }
}


void x86_cmppd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmpps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmpsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmpss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmpxchg(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cmpxchg8b(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_comisd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_comiss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cpuid(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    // NOTE: don't forget the ID flag in the eflags register to notify that we support cpuid
    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_crc32(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtdq2pd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtdq2ps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtpd2qd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtpd2pi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtpd2ps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtpi2pd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtpi2ps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtps2dq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtps2pd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtps2pi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtsd2si(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtsd2ss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtsi2sd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtsi2ss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtss2sd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvtss2si(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttpd2dq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttpd2pi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttps2dq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttps2pi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttsd2si(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cvttss2si(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_cwq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_daa(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_das(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_dec(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_div(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_divpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_divps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_divsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_divss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_dppd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_dpps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_emms(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}

void x86_mm_endbr32(void *cpu, struct exec_data data)
{
    x86_mm_nop(cpu, data);
}


void x86_enter(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_extractps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_f2xm1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fabs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fadd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_faddp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fiadd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fbld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fbstp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fchs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnclex(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcmovcc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcom(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcomp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcompp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcomi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcomip(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fucomi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fucomip(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fcos(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdecstp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdiv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdivp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fidiv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdivr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdivrp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fdivir(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ffree(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ficom(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ficomp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fild(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fincstp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fclex(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_finit(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fininit(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fist(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fistp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fisttp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fld1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldl2t(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldl2e(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldpi(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldlg2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldln2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldz(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldcw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fldenv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fmul(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fmulp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fimul(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnop(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fpatan(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fprem(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fprem1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fptan(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_frndint(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_frstor(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsave(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnsave(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fscale(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsin(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsincos(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsqrt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fst(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fstp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnstcw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fstenv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnstenv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fstsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fnstsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsub(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsubp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fisub(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsubr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fsubrp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fisubr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ftst(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fucom(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fucomp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fucompp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fxam(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fxch(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fxrstor(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fxsave(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fxtract(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fyl2x(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_fyl2xp1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_haddpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_haddps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_hlt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_hsubpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_hsubps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_idiv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_imul(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_in(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_inc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ins(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_insertps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_int3(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_int(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_int0(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_int1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_invd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_invlpg(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_iret(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_jcc(void *cpu, struct exec_data data)
{
    _Bool condition_is_true = 0;
    reg32_t jump_target = x86_findbranchtarget_relative(cpu, x86_readR32(cpu, EIP) - data.bytes, data);
    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    switch (data.opc) {
        case 0x87:  // JA rel32     rel16
        case 0x77:  // JA rel8
            condition_is_true = x86_flag_off(cpu, CF) && x86_flag_off(cpu, ZF);
            break;
        case 0x83:  // JAE rel32    rel16
        case 0x73:  // JAE rel8
            condition_is_true = x86_flag_off(cpu, CF);
            break;
        case 0x82:  // JB rel32     rel16
        case 0x72:  // JB rel8
            condition_is_true = x86_flag_on(cpu, CF);
            break;
        case 0x86:  // JBE rel32    rel16
        case 0x76:  // JBE rel8
            condition_is_true = x86_flag_on(cpu, CF) || x86_flag_on(cpu, ZF);
            break;
        case 0xE3:  // JCXZ rel8
            if (data.adrsz_pfx)
                condition_is_true = x86_readR16(cpu, CX) == 0;
            else
                condition_is_true = x86_readR32(cpu, ECX) == 0;
            break;
        case 0x84:  // JE rel32     rel16
        case 0x74:  // JE rel8
            condition_is_true = x86_flag_on(cpu, ZF);
            break;
        case 0x8F:  // JG rel32     rel16
        case 0x7F:  // JG rel7
            condition_is_true = x86_flag_off(cpu, ZF) && x86_flag_off(cpu, SF);
            break;
        case 0x8D:  // JGE rel32    rel16
        case 0x7D:  // JGE rel8
            condition_is_true = x86_flag_off(cpu, SF);
            break;
        case 0x8C:  // JL rel32     rel16
        case 0x7C:  // JL rel8
            condition_is_true = (x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF)) ||
                                (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF));
            break;
        case 0x8E:  // JLE rel32     rel16
        case 0x7E:  // JLE rel8
            condition_is_true = x86_flag_on(cpu, ZF) ||
                                (x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF)) ||
                                (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF));
            break;
        case 0x85:  // JNE rel32     rel16
        case 0x75:  // JNE rel8
            condition_is_true = x86_flag_off(cpu, ZF);
            break;
        case 0x81:  // JNO rel32     rel16
        case 0x71:  // JNO rel8
            condition_is_true = x86_flag_off(cpu, OF);
            break;
        case 0x8B:  // JNP rel32     rel16
        case 0x7B:  // JNP rel8
            condition_is_true = x86_flag_off(cpu, PF);
            break;
        case 0x89:  // JNS rel32     rel16
        case 0x79:  // JNS rel8
            condition_is_true = x86_flag_off(cpu, SF);
            break;
        case 0x80:  // JO rel32     rel16
        case 0x70:  // JO rel8
            condition_is_true = x86_flag_on(cpu, OF);
            break;
        case 0x8A:  // JP rel32     rel16
        case 0x7A:  // JPE rel8
            condition_is_true = x86_flag_on(cpu, PF);
            break;
        case 0x88:  // JS rel32     rel16
        case 0x78:  // JS rel8
            condition_is_true = x86_flag_on(cpu, SF);
            break;
    }

    if (condition_is_true)
        x86_update_eip_absolute(cpu, jump_target);
}


void x86_mm_jmp(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0xEB:
            x86__mm_short_rel8_jmp(cpu, lsb(data.imm1));
            break;
        case 0xE9:
            if (data.oprsz_pfx)
                x86__mm_near_rel16_jmp(cpu, low16(data.imm1));
            else
                x86__mm_near_rel32_jmp(cpu, data.imm1);
            break;
        case 0xEA:
            if (data.oprsz_pfx)
                x86__mm_far_absl_ptr16_jmp(cpu, data.segovr, low16(data.imm1));
            else
                x86__mm_far_absl_ptr32_jmp(cpu, data.segovr, data.imm2);
            break;
        case 0xFF:
            if (data.ext == 4) {
                if (vaddr) {
                    if (data.oprsz_pfx)
                        x86__mm_near_absl_m16_jmp(cpu, vaddr);
                    else
                        x86__mm_near_absl_m32_jmp(cpu, vaddr);
                } else {
                    if (data.oprsz_pfx)
                        x86__mm_near_absl_r16_jmp(cpu, effctvregister(data.modrm, 16));
                    else
                        x86__mm_near_absl_r32_jmp(cpu, effctvregister(data.modrm, 32));
                }
            } else {
                if (data.oprsz_pfx)
                    x86__mm_far_absl_ptr16_jmp(cpu, data.segovr, low16(data.imm1));
                else
                    x86__mm_far_absl_ptr32_jmp(cpu, data.segovr, data.imm2);
            }
            break;
    }
}


void x86_lahf(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lar(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lddqu(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ldmxcsr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lds(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_les(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lfs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lgs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_lea(void *cpu, struct exec_data data)
{
    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx && data.oprsz_pfx) {
        x86__mm_r16_m_lea(cpu, reg(data.modrm), x86_effectiveaddress16(cpu, data.modrm, data.moffset));
    } else if (data.oprsz_pfx) {
        x86__mm_r16_m_lea(cpu, reg(data.modrm), moffset16(x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset)));
    } else if (data.adrsz_pfx) {
        x86__mm_r32_m_lea(cpu, reg(data.modrm), zeroxtnd16(x86_effectiveaddress16(cpu, data.modrm, data.moffset)));
    } else {
        x86__mm_r32_m_lea(cpu, reg(data.modrm), x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset));
    }
}


void x86_leave(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lfence(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lgdt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lidt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lldt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lmsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lods(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_loopcc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lsl(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ltr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_lzcnt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maskmovdqu(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maskmovq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maxpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maxps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maxsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_maxss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mfence(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_minpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_minps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_minsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_minss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_monitor(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_mov(void *cpu, struct exec_data data)
{
    moffset32_t vaddr = 0;
    int reg_dest = 0;
    _Bool r8imm8 = 0, rXimmX = 0;
    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x88:  // MOV r/m8, r8
            if (vaddr)
                x86__mm_m8_r8_mov(cpu, vaddr, reg(data.modrm));
            else
                x86__mm_m8_r8_mov(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x89: // MOV r/m32,r32     MOV r/m16,r16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_mov(cpu, vaddr, reg(data.modrm));
                else
                    x86__mm_m32_r32_mov(cpu, vaddr, reg(data.modrm));
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_mov(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_mov(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;
        case 0x8A:  //MOV r8,r/m8
            if (vaddr)
                x86__mm_r8_m8_mov(cpu, reg(data.modrm), vaddr);
            else
                x86__mm_r8_r8_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            break;
        case 0x8B: // MOV r32,r/m32     MOV r16,r/m16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m16_mov(cpu, reg(data.modrm), vaddr);
                else
                    x86__mm_r32_m32_mov(cpu, reg(data.modrm), vaddr);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                else
                    x86__mm_r32_r32_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
            break;
        case 0x8C: // MOV r/m16, Sreg
            if (vaddr)
                x86__mm_m16_sreg_mov(cpu, vaddr, reg(data.modrm));
            else
                x86__mm_r16_sreg_mov(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
            break;
        case 0x8E: // MOV Sreg,r/m16
            if (vaddr)
                x86__mm_sreg_m16_mov(cpu, reg(data.modrm), vaddr);
            else
                x86__mm_sreg_r16_mov(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
            break;
        case 0xA0: // MOV AL,moffs8
            x86__mm_r8_imm8_mov(cpu, AL, x86_rdsreg(cpu, data.segovr) + lsb(data.imm1));
            break;
        case 0xA1: //MOV EAX,moffs32    MOV AX,moffs16
            if (data.oprsz_pfx)
                x86__mm_r16_imm16_mov(cpu, AX, x86_rdsreg(cpu, data.segovr) + low16(data.imm1));
            else
                x86__mm_r32_imm32_mov(cpu, EAX, x86_rdsreg(cpu, data.segovr) + data.imm1);
            break;
        case 0xA2:  // MOV moffs8,AL
            x86__mm_m8_r8_mov(cpu, x86_rdsreg(cpu, data.segovr) + lsb(data.imm1), AL);
            break;
        case 0xA3:  // MOV moffs32,EAX  MOV moffs16,AX
            if (data.oprsz_pfx)
                x86__mm_m16_r16_mov(cpu, x86_rdsreg(cpu, data.segovr) + low16(data.imm1), AX);
            else
                x86__mm_m32_r32_mov(cpu, x86_rdsreg(cpu, data.segovr) + data.imm1, EAX);
            break;
            //MOV r16/32, imm16/32
        case 0xB0: reg_dest = EAX; rXimmX = 1; break;
        case 0xB1: reg_dest = ECX; rXimmX = 1; break;
        case 0xB2: reg_dest = EDX; rXimmX = 1; break;
        case 0xB3: reg_dest = EBX; rXimmX = 1; break;
        case 0xB4: reg_dest = ESP; rXimmX = 1; break;
        case 0xB5: reg_dest = EBP; rXimmX = 1; break;
        case 0xB6: reg_dest = ESI; rXimmX = 1; break;
        case 0xB7: reg_dest = EDI; rXimmX = 1; break;

        // MOV r8, imm8
        case 0xB8: reg_dest = EAX; r8imm8 = 1; break;
        case 0xB9: reg_dest = ECX; r8imm8 = 1; break;
        case 0xBA: reg_dest = EDX; r8imm8 = 1; break;
        case 0xBB: reg_dest = EBX; r8imm8 = 1; break;
        case 0xBC: reg_dest = ESP; r8imm8 = 1; break;
        case 0xBD: reg_dest = EBP; r8imm8 = 1; break;
        case 0xBE: reg_dest = ESI; r8imm8 = 1; break;
        case 0xBF: reg_dest = EDI; r8imm8 = 1; break;

        case 0xC6:  // MOV r/m8, imm8
            if (vaddr)
                x86__mm_m8_imm8_mov(cpu, vaddr, lsb(data.imm1));
            else
                x86__mm_r8_imm8_mov(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0xC7: // MOV r/m32, imm32  MOV r/m16, imm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_mov(cpu, vaddr, low16(data.imm1));
                else
                    x86__mm_m32_imm32_mov(cpu, vaddr, data.imm1);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_mov(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                else
                    x86__mm_r32_imm32_mov(cpu, effctvregister(data.modrm, 32), data.imm1);
            }
            break;
    }

    if (r8imm8) {
        x86__mm_r8_imm8_mov(cpu, reg_dest, lsb(data.imm1));
        return;
    }

    if (rXimmX) {
        if (data.oprsz_pfx)
            x86__mm_r16_imm16_mov(cpu, reg_dest, low16(data.imm1));
        else
            x86__mm_r32_imm32_mov(cpu, reg_dest, data.imm1);
    }
}


void x86_movapd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movaps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movbe(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movddup(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movdqa(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movdqu(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movdq2q(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movhlps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movhpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movhps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movlpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movlps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movmskpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movmskps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movntdqa(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movntdq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movnti(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movntpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movntps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movntq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movq2dq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movshdup(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movsldup(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movsx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movupd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_movups(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_movzx(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0xB6:
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m8_movzx(cpu, reg(data.modrm), vaddr);
                else
                    x86__mm_r32_m8_movzx(cpu, reg(data.modrm), vaddr);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r8_movzx(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
                else
                    x86__mm_r32_r8_movzx(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            }
            break;
        case 0xB7:
            if (vaddr)
                x86__mm_r32_m16_movzx(cpu, reg(data.modrm), vaddr);
            else
                x86__mm_r32_r16_movzx(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
            break;
    }
}


void x86_mpsadbw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mul(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mulpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mulps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mulsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mulss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mwait(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_neg(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_nop(void *cpu, struct exec_data data)
{
    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");
}


void x86_not(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_or(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_orpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_orps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_out(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_outs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pabsb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pabsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pabsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_packsswb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_packssdw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_packusdw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_packuswb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddsb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddusb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_paddusw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_palignr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pand(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pandn(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pause(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pavgb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pavgw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pblendvb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pblendw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pclmulqdq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpeqb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpeqw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpeqd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpeqq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpestri(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpestrm(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpgtb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpgtw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpgtd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpgtq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpistri(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pcmpistrm(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pextrb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pextrd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pextrw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phaddw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phaddd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phaddsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phminposuw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phsubw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phsubd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_phsubsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pinsrb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pinsrd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pinsrw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaddubsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaddwd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxsb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxub(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxuw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmaxud(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminsb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminub(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminuw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pminud(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmovmskb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmovsx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmovzx(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmuldq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmulhrsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmulhuw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmulhw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmulld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmullw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pmuludq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_pop(void *cpu, struct exec_data data)
{
    moffset32_t vaddr = 0;
    int reg_dest = 0;
    _Bool is_sreg = 0;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    switch (data.opc) {
        case 0x8F:
            if (data.adrsz_pfx)
                vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
            else
                vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

            if (!vaddr)
                reg_dest = effctvregister(data.modrm, 32);
            break;
        case 0x58: reg_dest = EAX; break;
        case 0x59: reg_dest = ECX; break;
        case 0x5A: reg_dest = EDX; break;
        case 0x5B: reg_dest = EBX; break;
        case 0x5C: reg_dest = ESP; break;
        case 0x5D: reg_dest = EBP; break;
        case 0x5E: reg_dest = ESI; break;
        case 0x5F: reg_dest = EDI; break;
        case 0x1F: reg_dest = DS; is_sreg = 1; break;
        case 0x07: reg_dest = ES; is_sreg = 1; break;
        case 0x17: reg_dest = SS; is_sreg = 1; break;
        case 0xA1: reg_dest = FS; is_sreg = 1; break;
        case 0xA9: reg_dest = GS; is_sreg = 1; break;
    }

    if (vaddr) {
        if (data.oprsz_pfx)
            x86__mm_m16_pop(cpu, vaddr);
        else
            x86__mm_m32_pop(cpu, vaddr);
    } else if (is_sreg) {
            x86__mm_sreg_pop(cpu, reg_dest);
    } else {
        if (data.oprsz_pfx)
            x86__mm_r16_pop(cpu, reg_dest);
        else
            x86__mm_r32_pop(cpu, reg_dest);
    }
}


void x86_popa(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_popcnt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_popf(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_por(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_prefetcht0(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_prefetcht1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_prefetcht2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_prefetchnta(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_prefetchw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psadbw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pshufb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pshufd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pshufhw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pshuflw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pshufw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psignb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psignw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psignd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psllw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pslld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psllq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubsb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubusb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_psubusw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ptest(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpckhbw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpckhwd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpckhdq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpckhqdq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpcklbw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpcklwd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpckldq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_punpcklqdq(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_push(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;
    uint8_t reg = 0;
    uint8_t sreg = 0;
    _Bool  is_sreg = 0, is_reg = 0;

    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");


    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0xFF: // PUSH r/m32    PUSH r/m16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_push(cpu, vaddr);
                else
                    x86__mm_m32_push(cpu, vaddr);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_push(cpu, effctvregister(data.modrm, 16));
                else
                    x86__mm_r32_push(cpu, effctvregister(data.modrm, 32));
            }
            break;

        // PUSH r32     PUSH r16
        case 0x50: reg = EAX; is_reg = 1; break;
        case 0x51: reg = ECX; is_reg = 1; break;
        case 0x52: reg = EDX; is_reg = 1; break;
        case 0x53: reg = EBX; is_reg = 1; break;
        case 0x54: reg = ESP; is_reg = 1; break;
        case 0x55: reg = EBP; is_reg = 1; break;
        case 0x56: reg = ESI; is_reg = 1; break;
        case 0x57: reg = EDI; is_reg = 1; break;
        // PUSH imm8
        case 0x6A:
            x86__mm_imm32_push(cpu, zeroxtnd8(data.imm1));
            break;
        case 0x68:
            if (data.oprsz_pfx)
                x86__mm_imm16_push(cpu, low16(data.imm1));
            else
                x86__mm_imm32_push(cpu, data.imm1);
            break;

        case 0x0E: sreg = CS; is_sreg = 1; break;
        case 0x16: sreg = SS; is_sreg = 1; break;
        case 0x1E: sreg = DS; is_sreg = 1; break;
        case 0x06: sreg = ES; is_sreg = 1; break;
        case 0xA0: sreg = FS; is_sreg = 1; break;
        case 0xA8: sreg = GS; is_sreg = 1; break;
    }

    if (is_sreg) {
        x86__mm_sreg_push(cpu, sreg);
    } else if (is_reg) {
        if (data.oprsz_pfx)
            x86__mm_r16_push(cpu, reg);
        else
            x86__mm_r32_push(cpu, reg);
    }
}


void x86_pusha(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pushf(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_pxor(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rcl(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rcr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rol(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ror(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rcpps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rcpss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rdmsr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rdpkru(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rdpmc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rdtsc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rdtscp(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_ret(void *cpu, struct exec_data data)
{
    switch (data.opc) {
        case 0xC2:
            if (data.oprsz_pfx)
                x86__mm_imm16_near_ret16(cpu, low16(data.imm1));
            else
                x86__mm_imm16_near_ret32(cpu, low16(data.imm1));
            break;
        case 0xC3:
            if (data.oprsz_pfx)
                x86__mm_near_ret16(cpu);
            else
                x86__mm_near_ret32(cpu);
            break;
        case 0xCA:
            if (data.oprsz_pfx)
                x86__mm_imm16_far_ret16(cpu, low16(data.imm1));
            else
                x86__mm_imm16_far_ret32(cpu, low16(data.imm1));
            break;
        case 0xCB:
            if (data.oprsz_pfx)
                x86__mm_far_ret16(cpu);
            else
                x86__mm_far_ret32(cpu);
            break;

    }
}


void x86_roundpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_roundps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_roundsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_roundss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rsm(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rsqrtps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_rsqrtss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sahf(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sal(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sar(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_shl(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_shr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sbb(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_scas(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_setcc(void *cpu, struct exec_data data)
{
    _Bool  condition_is_true = 0;
    moffset32_t vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x97:  // SETA
            condition_is_true = x86_flag_off(cpu, CF) && x86_flag_off(cpu, ZF);
            break;
        case 0x93:  // SETAE
            condition_is_true = x86_flag_off(cpu, CF);
            break;
        case 0x92:  // SETB
            condition_is_true = x86_flag_on(cpu, CF);
            break;
        case 0x96:  // SETBE
            condition_is_true = x86_flag_on(cpu, CF) || x86_flag_on(cpu, ZF);
            break;
        case 0x94:  // SETE
            condition_is_true = x86_flag_on(cpu, ZF);
            break;
        case 0x9F:  // SETG
            condition_is_true = x86_flag_off(cpu, ZF) && x86_flag_off(cpu, SF);
            break;
        case 0x9D:  // SETGE
            condition_is_true = x86_flag_off(cpu, SF);
            break;
        case 0x9C:  // SETL
            condition_is_true = (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF)) ||
                                (x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF));
            break;
        case 0x9E:  // SETLE
            condition_is_true = x86_flag_on(cpu, ZF) ||
                                (x86_flag_off(cpu, SF) && x86_flag_on(cpu, OF)) ||
                                (x86_flag_on(cpu, SF) && x86_flag_off(cpu, OF));
            break;
        case 0x95:  // SETNE
            condition_is_true = x86_flag_off(cpu, ZF);
            break;
        case 0x91:  // SETNO
            condition_is_true = x86_flag_off(cpu, OF);
            break;
        case 0x9B:  // SETNP
            condition_is_true = x86_flag_off(cpu, PF);
            break;
        case 0x99:  // SETNS
            condition_is_true = x86_flag_off(cpu, SF);
            break;
        case 0x90:  // SETO
            condition_is_true = x86_flag_on(cpu, ZF);
            break;
        case 0x9A:  // SETP
            condition_is_true = x86_flag_on(cpu, PF);
            break;
        case 0x98:  // SETS
            condition_is_true = x86_flag_on(cpu, SF);
            break;
    }

    if (condition_is_true) {
        if (vaddr)
            x86_writeM8(cpu, vaddr, 1);
        else
            x86_writeR8(cpu, effctvregister(data.modrm, 8), 1);
    }
}


void x86_sfence(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sgdt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha1rnds4(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha1nexte(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha1msg1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha1msg2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha256rnds2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha256msg1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sha256msg2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_shld(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_shrd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_shufpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sidt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sldt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_smsw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sqrtpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sqrtps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sqrtsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sqrtss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_stac(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_stc(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_std(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sti(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_stos(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_str(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_sub(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, low16(data.moffset));

    switch (data.opc) {
        case 0x2C:  // SUB AL, imm8
            x86__mm_al_imm8_sub(cpu, lsb(data.imm1));
            break;
        case 0x2D:  // SUB eAX, imm32  SUB AX, imm16
            if (data.oprsz_pfx)
                x86__mm_ax_imm16_sub(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_sub(cpu, data.imm1);
            break;
        case 0x82:
        case 0x80:  // SUB rm8, imm8
            if (vaddr)
                x86__mm_m8_imm8_sub(cpu, vaddr, lsb(data.imm1), data.lock);
            else
                x86__mm_r8_imm8_sub(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0x81:  // SUB rm32, imm32  SUB rm16, imm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_sub(cpu, vaddr, low16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_sub(cpu, vaddr, data.imm1, data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_sub(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                else
                    x86__mm_r32_imm32_sub(cpu, effctvregister(data.modrm, 32), data.imm1);
            }
            break;
        case 0x83:  // SUB rm32, imm8   SUB rm16, imm8
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_sub(cpu, vaddr, sign8to16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_sub(cpu, vaddr, sign8to32(data.imm1), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_sub(cpu, effctvregister(data.modrm, 16), sign8to16(data.imm1));
                else
                    x86__mm_r32_imm32_sub(cpu, effctvregister(data.modrm, 32), sign8to32(data.imm1));
            }
            break;

        case 0x28:  // SUB rm8, r8
            if (vaddr)
                x86__mm_m8_imm8_sub(cpu, vaddr, reg(data.modrm), data.lock);
            else
                x86__mm_r8_imm8_sub(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x29:  // SUB rm32, r32    SUB rm16, r16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_sub(cpu, vaddr, reg(data.modrm), data.lock);
                else
                    x86__mm_m32_r32_sub(cpu, vaddr, reg(data.modrm), data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_sub(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_sub(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;
        case 0x2A:  // SUB r8, rm8
            if (vaddr)
                x86__mm_r8_m8_sub(cpu, reg(data.modrm), vaddr, data.lock);
            else
                x86__mm_r8_r8_sub(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            break;
        case 0x2B: // SUB r32, rm32  SUB r16, rm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_m16_sub(cpu, reg(data.modrm), vaddr, data.lock);
                else
                    x86__mm_r32_m32_sub(cpu, reg(data.modrm), vaddr, data.lock);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_sub(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                else
                    x86__mm_r32_r32_sub(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
            }
            break;
    }
}


void x86_subpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_subps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_subsd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_subss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_swapgs(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_syscall(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sysenter(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sysexit(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_sysret(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_mm_test(void *cpu, struct exec_data data)
{
    if (data.lock)
        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

    moffset32_t vaddr;

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0xA8:  // TEST AL, imm8
            x86__mm_al_imm8_test(cpu, lsb(data.imm1));
            break;
        case 0xA9: // TEST EAX, imm32   AX, imm16
            if (data.oprsz_pfx)
                x86__mm_ax_imm16_test(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_test(cpu, data.imm1);
            break;
        case 0xF6:  // TEST r/m8, imm8
            if (vaddr)
                x86__mm_m8_imm8_test(cpu, vaddr, lsb(data.imm1));
            else
                x86__mm_r8_imm8_test(cpu, effctvregister(data.modrm, 8), lsb(data.imm1));
            break;
        case 0xF7: // TEST r/m32, imm32      TEST r/m16, imm16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_test(cpu, vaddr, low16(data.imm1));
                else
                    x86__mm_m32_imm32_test(cpu, vaddr, data.imm1);
            } else {
                if (data.oprsz_pfx) {
                    x86__mm_r16_imm16_test(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                } else {
                    x86__mm_r32_imm32_test(cpu, effctvregister(data.modrm, 32), data.imm1);
                }
            }
            break;
        case 0x84:  // TEST r/m8, r8
            if (vaddr)
                x86__mm_m8_r8_test(cpu, vaddr, reg(data.modrm));
            else
                x86__mm_r8_r8_test(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            break;
        case 0x85:  // TEST r/m32, r32   TEST r/m16, r16
            if (vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_m16_r16_test(cpu, vaddr, reg(data.modrm));
                else
                    x86__mm_m32_r32_test(cpu, vaddr, reg(data.modrm));
            } else {
                if (data.oprsz_pfx)
                    x86__mm_r16_r16_test(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                else
                    x86__mm_r32_r32_test(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
            }
            break;

    }
}


void x86_tzcnt(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ucomisd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ucomiss(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ud0(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ud1(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_ud2(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_unpckhpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_unpckhps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_unpcklpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_unpcklps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_verr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_verw(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_wait(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_wbinvd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_wrmsr(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_wrpkru(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xabort(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xadd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xbegin(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xchg(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xend(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xgetbv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xlat(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}

void x86_mm_xor(void *cpu, struct exec_data data)
{
    moffset32_t vaddr;

    if (data.adrsz_pfx)
        vaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        vaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x30:  // r/m8, r8
            if (!vaddr) {
                if (data.lock)
                    x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");
                x86__mm_r8_r8_xor(cpu, effctvregister(data.modrm, 8), reg(data.modrm));
            } else {
                x86__mm_m8_r8_xor(cpu, vaddr, reg(data.modrm), data.lock);
            }
            break;
        case 0x31:  // r/m32, r32   r/m16, r16
            if (data.oprsz_pfx) {
                if (!vaddr) {
                    if (data.lock)
                        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

                    x86__mm_r16_r16_xor(cpu, effctvregister(data.modrm, 16), reg(data.modrm));
                } else {
                    x86__mm_m16_r16_xor(cpu, vaddr, reg(data.modrm), data.lock);
                }
            } else {
                if (!vaddr) {
                    if (data.lock)
                        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

                    x86__mm_r32_r32_xor(cpu, effctvregister(data.modrm, 32), reg(data.modrm));
                } else {
                    x86__mm_m32_r32_xor(cpu, vaddr, reg(data.modrm), data.lock);
                }
            }

            break;
        case 0x32: // r8, r/m8
            if (!vaddr) {
                if (data.lock)
                    x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

                x86__mm_r8_r8_xor(cpu, reg(data.modrm), effctvregister(data.modrm, 8));
            } else {
                x86__mm_r8_m8_xor(cpu, reg(data.modrm), vaddr, data.lock);
            }
            break;
        case 0x33: // r32, r/m32    r16, r/m16

            if (data.oprsz_pfx) {
                if (!vaddr) {
                    if (data.lock)
                        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

                    x86__mm_r16_r16_xor(cpu, reg(data.modrm), effctvregister(data.modrm, 16));
                } else {
                    x86__mm_r16_m16_xor(cpu, reg(data.modrm), vaddr, data.lock);
                }
            } else {
                if (!vaddr) {
                    if (data.lock)
                        x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

                    x86__mm_r32_r32_xor(cpu, reg(data.modrm), effctvregister(data.modrm, 32));
                } else {
                    x86__mm_r32_m32_xor(cpu, reg(data.modrm), vaddr, data.lock);
                }
            }

            break;
        case 0x34: // AL, imm8
            if (data.lock)
                x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

            x86__mm_al_imm8_xor(cpu, lsb(data.imm1));
            break;
        case 0x35: // eAX, imm32    AX, imm16
            if (data.lock)
                x86_raise_exception_d(cpu, INT_UD, tracer_get(x86_tracer(cpu), TRACE_VAR_EIP), "Invalid LOCK prefix");

            if (data.oprsz_pfx)
                x86__mm_ax_imm16_xor(cpu, low16(data.imm1));
            else
                x86__mm_eax_imm32_xor(cpu, data.imm1);
            break;
        case 0x80: // r/m8, imm8
        case 0x82:
            if (!vaddr)
                x86__mm_r8_imm8_xor(cpu, effctvregister(data.modrm, 8), data.imm1);
            else
                x86__mm_m8_imm8_xor(cpu, vaddr, data.imm1, data.lock);
            break;
        case 0x81: // r/m32, imm32  r/m16, imm16
            if (!vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_xor(cpu, effctvregister(data.modrm, 16), low16(data.imm1));
                else
                    x86__mm_r32_imm32_xor(cpu, effctvregister(data.modrm, 32), data.imm1);
            } else {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_xor(cpu, vaddr, low16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_xor(cpu, vaddr, data.imm1, data.lock);
            }
            break;
        case 0x83: // r/m32, imm8   r/m16, imm8
            if (!vaddr) {
                if (data.oprsz_pfx)
                    x86__mm_r16_imm16_xor(cpu, effctvregister(data.modrm, 16), zeroxtnd8to16(data.imm1));
                else
                    x86__mm_r32_imm32_xor(cpu, effctvregister(data.modrm, 32), zeroxtnd8(data.imm1));
            } else {
                if (data.oprsz_pfx)
                    x86__mm_m16_imm16_xor(cpu, vaddr, zeroxtnd8to16(data.imm1), data.lock);
                else
                    x86__mm_m32_imm32_xor(cpu, vaddr, zeroxtnd8(data.imm1), data.lock);
            }
            break;
    }
}


void x86_xorpd(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xorps(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xsetbv(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


void x86_xtest(void *cpu, struct exec_data data)
{
    (void)cpu, (void)data;

    
    x86_stopcpu(cpu);
    s_error(1, "emulator: Instruction %s not implemented", __FUNCTION__);
}


