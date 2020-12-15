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

    cpu->r_EAX = 0;
    cpu->r_ECX = 0;
    cpu->r_EDX = 0;
    cpu->r_EBX = 0;
    cpu->r_ESI = 0;
    cpu->r_EDI = 0;
    cpu->r_ESP = 0;
    cpu->r_EBP = 0;
    cpu->r_EIP = 0;

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

#define update_eip8(cpu) (cpu)->r_EIP+=1
#define update_eip16(cpu) (cpu)->r_EIP+=2
#define update_eip32(cpu) (cpu)->r_EIP+=4
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

    byte = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
    update_eip8(cpu);

    // handles prefixes
    if (x86_byteispfx(byte)) {
        data.pfx = byte;

        byte = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
        update_eip8(cpu);

        if (byte == PFX_OPRSZ)
            data.oprsz_pfx = 1;
        else if (byte == PFX_ADDRSZ)
            data.adrsz_pfx = 1;

    }

    // 'two-byte' instructions
    if (byte == 0x0F) {
        table = x86_opcode_0f_table;

        byte = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
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
        byte2 = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));

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
        data.modrm = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
        update_eip8(cpu);

        switch (data.modrm & MODRM_MOD_BIT_MASK) {
            case 0: // MOD 00 RM 100
            case 1: // MOD 01 RM 100
            case 2: // MOD 10 RM 100
                if ((data.modrm & MODRM_RM_BIT_MASK) == 4) {
                    data.sib = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
                    update_eip8(cpu);
                }
                break;
            default:
                break;
        }
    }

    // handle instructions with type <prefix> <primary> [secondary]
    if (op.o_prefix && data.pfx) {
        if (table == x86_opcode_table)
            op = *op.o_prefix;
        else if (table == x86_opcode_0f_table)
            op = op.o_prefix[data.pfx & TABLE_0F_PREFIX_MASK];

        if (op.o_sec_table) {
            byte2 = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));

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
            data.imm1 = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip8(cpu);
            break;
        case imm16:
        case imm16_AX:
        case AX_imm16:
        case rm16_imm16:
        case r16_rm16_imm16:
        case rela16:
            data.imm1 = b_mmu_read16(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip16(cpu);
            break;
        case eAX_imm32:
        case rela32:
            data.imm1 = b_mmu_read32(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip32(cpu);
            break;
        case rela16_16:
        case ptr16_16:
            data.imm1 = b_mmu_read16(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip16(cpu);
            data.imm2 = b_mmu_read16(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip16(cpu);
            break;
        case rela16_32:
        case ptr16_32:
            data.imm1 = b_mmu_read16(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip16(cpu);
            data.imm2 = b_mmu_read32(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip32(cpu);
            break;
        case imm16_imm8:
            data.imm1 = b_mmu_read16(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip16(cpu);
            data.imm2 = b_mmu_read8(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
            update_eip8(cpu);
            break;
        case imm32:
        case imm32_eAX:
        case rm32_imm32:
        case r32_rm32_imm32:
            data.imm1 = b_mmu_read32(&cpu->mmu, C_x86_rdreg32(cpu, EIP));
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

    cpu->r_ESP = cpu->r_EBP = b_mmu_create_stack(&cpu->mmu, stack_flags);

    cpu->r_EIP = cpu->executable.g_entryp;

    while (1) {
        fetch(cpu, &instr);

        if (instr.fail_to_fetch) {
            s_info("emulator: Invalid Instruction at 0x%08x", cpu->r_EIP);
            break;
        }

        instr.handler(cpu, instr.data);
        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    c_x86_stopcpu(cpu);
    exit(0);
}
