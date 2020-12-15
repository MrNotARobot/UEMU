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
 *  an emulated x86 cpu.
 */

#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "instructions.h"   // for the initialization function
#include "../system.h"
#include "../memory.h"

static void fetch(x86CPU *, struct instruction *);

//
// initialization
//
void c_x86_startcpu(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    x86_init_opcode_table();
    b_mmu_init(&cpu->mmu);

    cpu->EAX = 0;
    cpu->ECX = 0;
    cpu->EDX = 0;
    cpu->EBX = 0;
    cpu->ESI = 0;
    cpu->EDI = 0;
    cpu->ESP = 0;
    cpu->EBP = 0;
    cpu->EIP = 0;

    cpu->eflags.f_AC = 0;
    cpu->eflags.f_VM = 0;
    cpu->eflags.f_RF = 0;
    cpu->eflags.f_NT = 0;
    cpu->eflags.f_IOPL = 0;
    cpu->eflags.f_OF = 0;
    cpu->eflags.f_DF = 0;
    cpu->eflags.f_IF = 0;
    cpu->eflags.f_TF = 0;
    cpu->eflags.f_SF = 0;
    cpu->eflags.f_ZF = 0;
    cpu->eflags.f_AF = 0;
    cpu->eflags.f_PF = 0;
    cpu->eflags.reserved1 = 1;
    cpu->eflags.f_CF = 0;

    cpu->reg_table[EAX] = &cpu->EAX;
    cpu->reg_table[ECX] = &cpu->ECX;
    cpu->reg_table[EDX] = &cpu->EDX;
    cpu->reg_table[EBX] = &cpu->EBX;
    cpu->reg_table[ESP] = &cpu->ESP;
    cpu->reg_table[EBP] = &cpu->EBP;
    cpu->reg_table[ESI] = &cpu->ESI;
    cpu->reg_table[EDI] = &cpu->EDI;
    cpu->eflags_ptr = &cpu->eflags;
}


void c_x86_stopcpu(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    x86_free_opcode_table();

    if (cpu->executable.g_name)   // if we have an executable, then free it
        g_elf_unload(&cpu->executable);
    b_mmu_release_recs(&cpu->mmu);

    xfree(cpu);
}

void c_x86_raise_exception(x86CPU *cpu, int exct)
{
    ASSERT(cpu != NULL);

    switch (exct) {
        case INT_UD:
            c_x86_stopcpu(cpu);
            s_error(1, "Invalid Instruction at 0x%08x", cpu->EIP);
        default:
            break;
    }
}


int modrm2reg(uint8_t modrm)
{
    int reg = 0;
    switch (modrm) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06:
        case 0x07: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
        case 0x46: case 0x47: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84:
        case 0x85: case 0x86: case 0x87: case 0xC0: case 0xC1: case 0xC2: case 0xC3:
        case 0xC4: case 0xC5: case 0xC6: case 0xC7:
            reg = EAX; break;
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E:
        case 0x0F: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D:
        case 0x4E: case 0x4F: case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C:
        case 0x8D: case 0x8E: case 0x8F: case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
            reg = ECX; break;
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16:
        case 0x17: case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
        case 0x56: case 0x57: case 0x90: case 0x91: case 0x92: case 0x93: case 0x94:
        case 0x95: case 0x96: case 0x97: case 0xD0: case 0xD1: case 0xD2: case 0xD3:
        case 0xD4: case 0xD5: case 0xD6: case 0xD7:
            reg = EDX; break;
        case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
        case 0x1F: case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D:
        case 0x5E: case 0x5F: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C:
        case 0x9D: case 0x9E: case 0x9F: case 0xD8: case 0xD9: case 0xDA: case 0xDB:
        case 0xDC: case 0xDD: case 0xDE: case 0xDF:
            reg = EBX; break;
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
        case 0x27: case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:
        case 0x66: case 0x67: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4:
        case 0xA5: case 0xA6: case 0xA7: case 0xE0: case 0xE1: case 0xE2: case 0xE3:
        case 0xE4: case 0xE5: case 0xE6: case 0xE7:
            reg = ESP; break;
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E:
        case 0x2F: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D:
        case 0x6E: case 0x6F: case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC:
        case 0xAD: case 0xAE: case 0xAF: case 0xE8: case 0xE9: case 0xEA: case 0xEB:
        case 0xEC: case 0xED: case 0xEE: case 0xEF:
            reg = EBP; break;
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36:
        case 0x37: case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
        case 0x76: case 0x77: case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4:
        case 0xB5: case 0xB6: case 0xB7: case 0xF0: case 0xF1: case 0xF2: case 0xF3:
        case 0xF4: case 0xF5: case 0xF6: case 0xF7:
            reg = ESI; break;
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E:
        case 0x3F: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D:
        case 0x7E: case 0x7F: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC:
        case 0xBD: case 0xBE: case 0xBF: case 0xF8: case 0xF9: case 0xFA: case 0xFB:
        case 0xFC: case 0xFD: case 0xFE: case 0xFF:
            reg = EDI; break;
    }

    return reg;
}

int reg8islb(int reg)
{
    _Bool is_lb = 0;
    switch (reg) {
        case AL: case CL: case DL: case BL:
            is_lb = 1; break;
        default:
            break;
    }

    return is_lb;
}

int reg8to32(int reg)
{
    int reg32 = 0;

    switch (reg) {
        case AL: case AH: reg32 = EAX; break;
        case CL: case CH: reg32 = ECX; break;
        case DL: case DH: reg32 = EDX; break;
        case BL: case BH: reg32 = EBX; break;
        default: break;
    }

    return reg32;
}


addr_t c_x86_effctvaddr16(x86CPU *cpu, uint8_t modrm, uint32_t imm)
{
    ASSERT(cpu != NULL);
    uint8_t mod = modrm & MODRM_MOD_BIT_MASK >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;
    addr_t effctvaddr = 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: effctvaddr = C_x86_rdreg16(cpu, EBX) + C_x86_rdreg16(cpu, ESI); break;
            case 0b001: effctvaddr = C_x86_rdreg16(cpu, EBX) + C_x86_rdreg16(cpu, EDI); break;
            case 0b010: effctvaddr = C_x86_rdreg16(cpu, EBP) + C_x86_rdreg16(cpu, ESI); break;
            case 0b011: effctvaddr = C_x86_rdreg16(cpu, EBP) + C_x86_rdreg16(cpu, EDI); break;
            case 0b100: effctvaddr = C_x86_rdreg16(cpu, ESI); break;
            case 0b101: effctvaddr = C_x86_rdreg16(cpu, EDI); break;
            case 0b110: effctvaddr = imm; break;
            case 0b111: effctvaddr = C_x86_rdreg16(cpu, EBX); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: effctvaddr = C_x86_rdreg16(cpu, EBX) + imm; break;
            case 0b001: effctvaddr = C_x86_rdreg16(cpu, EBX) + imm; break;
            case 0b010: effctvaddr = C_x86_rdreg16(cpu, EBP) + imm; break;
            case 0b011: effctvaddr = C_x86_rdreg16(cpu, EBP) + imm; break;
            case 0b100: effctvaddr = C_x86_rdreg16(cpu, ESI) + imm; break;
            case 0b101: effctvaddr = C_x86_rdreg16(cpu, EDI) + imm; break;
            case 0b110: effctvaddr = C_x86_rdreg16(cpu, EBP) + imm; break;
            case 0b111: effctvaddr = C_x86_rdreg16(cpu, EBX) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    return effctvaddr;
}


addr_t c_x86_effctvaddr32(x86CPU *cpu, uint8_t modrm, uint32_t imm)
{
    ASSERT(cpu != NULL);
    uint8_t mod = modrm & MODRM_MOD_BIT_MASK >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;
    addr_t effctvaddr = 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: effctvaddr = C_x86_rdreg32(cpu, EAX); break;
            case 0b001: effctvaddr = C_x86_rdreg32(cpu, ECX); break;
            case 0b010: effctvaddr = C_x86_rdreg32(cpu, EDX); break;
            case 0b011: effctvaddr = C_x86_rdreg32(cpu, EBX); break;
            case 0b100:
                // TODO
                s_error(1, "TODO: calculate effective address with SIB");
                break;
            case 0b101: effctvaddr = cpu->EIP + imm; break;
            case 0b110: effctvaddr = C_x86_rdreg32(cpu, ESI); break;
            case 0b111: effctvaddr = C_x86_rdreg32(cpu, EDI); break;
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: effctvaddr = C_x86_rdreg32(cpu, EAX) + imm; break;
            case 0b001: effctvaddr = C_x86_rdreg32(cpu, ECX) + imm; break;
            case 0b010: effctvaddr = C_x86_rdreg32(cpu, EDX) + imm; break;
            case 0b011: effctvaddr = C_x86_rdreg32(cpu, EBX) + imm; break;
            case 0b100:
                // TODO
                s_error(1, "TODO: calculate effective address with SIB");
                break;
            case 0b101: effctvaddr = C_x86_rdreg32(cpu, EBP) + imm; break;
            case 0b110: effctvaddr = C_x86_rdreg32(cpu, ESI) + imm; break;
            case 0b111: effctvaddr = C_x86_rdreg32(cpu, EDI) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    return effctvaddr;
}

#define update_eip8(cpu) (cpu)->EIP+=1
#define update_eip16(cpu) (cpu)->EIP+=2
#define update_eip32(cpu) (cpu)->EIP+=4
#define set_fetch_err(instr, byte) \
    do {        \
        (instr)->fail_to_fetch = 1;     \
        (instr)->fail_byte = byte;  \
    } while (0)

// this will fetch the instruction and update the EIP accordingly
static void fetch(x86CPU *cpu, struct instruction *instr)
{
    uint8_t byte, byte2;
    int encoding;
    struct opcode *table = x86_opcode_table;
    struct opcode op;
    struct exec_data data;

    data.ext = 0;
    data.sec = 0;
    data.opc = 0;
    data.pfx = 0;
    data.modrm = 0;
    data.sib = 0;
    data.imm1 = 0;
    data.imm2 = 0;
    data.oprsz_pfx = 0;
    data.adrsz_pfx = 0;

    instr->fail_to_fetch = 0;
    instr->fail_byte = 0;

    byte = b_mmu_read8(&cpu->mmu, cpu->EIP);
    update_eip8(cpu);

    // handles prefixes
    if (x86_byteispfx(byte)) {
        data.pfx = byte;

        byte = b_mmu_read8(&cpu->mmu, cpu->EIP);
        update_eip8(cpu);

        if (byte == PFX_OPRSZ)
            data.oprsz_pfx = 1;
        else if (byte == PFX_ADDRSZ)
            data.adrsz_pfx = 1;

    }

    // 'two-byte' instructions
    if (byte == 0x0F) {
        table = x86_opcode_0f_table;

        byte = b_mmu_read8(&cpu->mmu, cpu->EIP);
        update_eip8(cpu);

    }

    op = table[byte];

    if (op.o_is_prefix) {
        set_fetch_err(instr, byte);
        return;
    }
    data.opc = byte;

    // handle instructions with secondary bytes (see AAM/AAD)
    if (op.o_sec_table) {
        byte2 = b_mmu_read8(&cpu->mmu, cpu->EIP);

        for (size_t i = 0; i < op.o_sec_tablesz; i++) {
            if (op.o_sec_table[i].o_opcode == byte2) {

                data.sec = byte2;
                op = op.o_sec_table[i];
                update_eip8(cpu);
            }
        }
    }

    // handle Mod/RM and opcode extensions
    if (op.o_use_rm) {
        data.modrm = b_mmu_read8(&cpu->mmu, cpu->EIP);
        update_eip8(cpu);

        switch (data.modrm & MODRM_MOD_BIT_MASK) {
            case 0: // MOD 00 RM 100
            case 1: // MOD 01 RM 100
            case 2: // MOD 10 RM 100
                if ((data.modrm & MODRM_RM_BIT_MASK) == 4) {
                    data.sib = b_mmu_read8(&cpu->mmu, cpu->EIP);
                    update_eip8(cpu);
                }
                break;
            default:
                break;
        }
    }

    // handle instructions with type <prefix> <primary> [secondary]
    if (op.o_prefix && data.pfx) {
        if (table == x86_opcode_table) {
            if (data.pfx == op.o_prefix->o_opcode)
                op = *op.o_prefix;
        } else if (table == x86_opcode_0f_table) {
            if (data.pfx == op.o_prefix[data.pfx & TABLE_0F_PREFIX_MASK].o_opcode)
                op = op.o_prefix[data.pfx & TABLE_0F_PREFIX_MASK];
        }

        if (op.o_sec_table) {
            byte2 = b_mmu_read8(&cpu->mmu, cpu->EIP);

            for (size_t i = 0; i < op.o_sec_tablesz; i++) {
                if (op.o_sec_table[i].o_opcode == byte2) {

                    data.sec = byte2;
                    op = op.o_sec_table[i];
                    update_eip8(cpu);
                }
            }
        }
    }

    if (op.o_extensions) {
        op = op.o_extensions[data.modrm & MODRM_REG_BIT_MASK];
        data.ext = data.modrm & MODRM_REG_BIT_MASK;
    }

    // select a encoding according to the operand-size
    if (data.oprsz_pfx)
        encoding = op.o_encoding16bit;
    else
        encoding = op.o_encoding;

    switch (encoding) {
        case imm8:
        case imm8_AL:
        case imm8_AX:
        case imm8_eAX:
        case AL_imm8:
        case AX_imm8:
        case eAX_imm8:
        case mm_imm8:
        case mm_r32m16_imm8:
        case mm1_imm8:
        case mm1_mm2m64_imm8:
        case rm8_imm8:
        case rm8_xmm2_imm8:
        case rm16_imm8:
        case rm16_r16_imm8:
        case rm16_xmm1_imm8:
        case rm32_r32_imm8:
        case rm32_imm8:
        case rm32_xmm1_imm8:
        case rm32_xmm2_imm8:
        case r16_rm16_imm8:
        case r32_rm32_imm8:
        case r32_mm_imm8:
        case r32_xmm_imm8:
        case rela8:
        case xmm_r32m16_imm8:
        case xmm1_imm8:
        case xmm1_r32m8_imm8:
        case xmm1_xmm2m32_imm8:
        case xmm1_xmm2m64_imm8:
        case xmm1_xmm2m128_imm8:
            data.imm1 = b_mmu_read8(&cpu->mmu, cpu->EIP);
            update_eip8(cpu);
            break;
        case imm16:
        case imm16_AX:
        case AX_imm16:
        case rm16_imm16:
        case r16_rm16_imm16:
        case rela16:
            data.imm1 = b_mmu_read16(&cpu->mmu, cpu->EIP);
            update_eip16(cpu);
            break;
        case eAX_imm32:
        case rela32:
            data.imm1 = b_mmu_read32(&cpu->mmu, cpu->EIP);
            update_eip32(cpu);
            break;
        case rela16_16:
        case ptr16_16:
            data.imm1 = b_mmu_read16(&cpu->mmu, cpu->EIP);
            update_eip16(cpu);
            data.imm2 = b_mmu_read16(&cpu->mmu, cpu->EIP);
            update_eip16(cpu);
            break;
        case rela16_32:
        case ptr16_32:
            data.imm1 = b_mmu_read16(&cpu->mmu, cpu->EIP);
            update_eip16(cpu);
            data.imm2 = b_mmu_read32(&cpu->mmu, cpu->EIP);
            update_eip32(cpu);
            break;
        case imm16_imm8:
            data.imm1 = b_mmu_read16(&cpu->mmu, cpu->EIP);
            update_eip16(cpu);
            data.imm2 = b_mmu_read8(&cpu->mmu, cpu->EIP);
            update_eip8(cpu);
            break;
        case imm32:
        case imm32_eAX:
        case rm32_imm32:
        case r32_rm32_imm32:
            data.imm1 = b_mmu_read32(&cpu->mmu, cpu->EIP);
            update_eip32(cpu);
            break;
        case AX_r16:
        case bnd_rm32:
        case bnd_sib:
        case bnd1m64_bnd2:
        case bnd1_bnd2m64:
        case eAX_r32:
        case m8:
        case m16:
        case m16_16:
        case m16_32:
        case m32:
        case m32_r32:
        case m64:
        case m64_mm:
        case m64_xmm1:
        case m128_xmm1:
        case mm_m64:
        case mm_rm32:
        case mm_xmm:
        case mm_xmm1m64:
        case mm_xmm1m128:
        case mm1_mm2:
        case mm1_mm2m32:
        case mm1_mm2m64:
        case rm8:
        case rm8_1:
        case rm8_CL:
        case rm8_r8:
        case rm16:
        case rm16_1:
        case rm16_CL:
        case rm16_r16:
        case rm16_r16_CL:
        case rm16_sreg:
        case rm32:
        case rm32_1:
        case rm32_CL:
        case rm32_mm:
        case rm32_r32:
        case rm32_r32_CL:
        case rm32_xmm:
        case r8_rm8:
        case r16:
        case r16_m16:
        case r16_m16_16:
        case r16_m32_16:
        case r16_rm8:
        case r16_rm16:
        case r16_r16m16:
        case r32:
        case r32m16:
        case r32_mm:
        case r32_m32:
        case r32_m16_32:
        case r32_m32_16:
        case r32_rm32:
        case r32_rm8:
        case r32_rm16:
        case r32_r32m16:
        case r32_sib:
        case r32_xmm:
        case r32_xmm1m32:
        case r32_xmm1m64:
        case sib_bnd:
        case sreg_rm16:
        case OP:
        case xmm_mm:
        case xmm_rm32:
        case xmm1_mm:
        case xmm1_m32:
        case xmm1_m64:
        case xmm1_m128:
        case xmm1_mm1m64:
        case xmm1_rm32:
        case xmm1_r32m32:
        case xmm1_xmm2:
        case xmm1_xmm2m16:
        case xmm1_xmm2m32:
        case xmm1_xmm2m64:
        case xmm1_xmm2m128:
        case xmm1m64_xmm2:
        case xmm2m32_xmm1:
        case xmm2m64_xmm1:
        case xmm2m128_xmm1:
            break;
        case no_encoding:
        default:
            set_fetch_err(instr, op.o_opcode);
            return;
    }

    instr->name = op.o_name;
    instr->handler = op.o_handler;
    instr->data = data;
}


/*
 * this is the main part.
 *
 * NOTE: executable should be malloc'ed buffer as this function does not return to the caller.
 */
void c_x86_cpu_exec(char *executable)
{
    x86CPU *cpu;
    struct instruction instr;

    int stack_flags = 0;

    if (!executable)
        s_error(1, "invalid reference at c_x86_cpu_exec");

    cpu = xcalloc(1, sizeof(*cpu));
    c_x86_startcpu(cpu);

    // map the executable segments to memory.
    // no shared library is loaded just yet.
    g_elf_load(&cpu->executable, executable);
    if (G_elf_error(&cpu->executable)) {
        c_x86_stopcpu(cpu);
        s_error(1, "%s", G_elf_errstr(&cpu->executable));
    }

    // we don't need it anymore.
    xfree(executable);

    // actually map the segments
    b_mmu_mmap_loadable(&cpu->executable, &cpu->mmu);
    if (B_mmu_error(&cpu->mmu)) {
        c_x86_stopcpu(cpu);
        s_error(1, "%s", B_mmu_errstr(&cpu->mmu));
    }

    // create a stack
    if (G_elf_execstack(&cpu->executable))
        stack_flags |= B_STACKEXEC;

    cpu->ESP = cpu->EBP = b_mmu_create_stack(&cpu->mmu, stack_flags);

    cpu->EIP = cpu->executable.g_entryp;

    while (1) {
        fetch(cpu, &instr);

        if (instr.fail_to_fetch)
            c_x86_raise_exception(cpu, INT_UD);
        else
            instr.handler(cpu, instr.data);
        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    c_x86_stopcpu(cpu);
    exit(0);
}
