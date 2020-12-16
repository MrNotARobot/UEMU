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
#include "../system.h"
#include "../memory.h"

static uint32_t rdmemx(x86CPU *cpu, addr_t effctvaddr, int size);
static void wrmemx(x86CPU *cpu, addr_t effctvaddr, uint32_t src, int size);
static void fetch(x86CPU *, struct instruction *);
static uint8_t displsz16(uint8_t);
static uint8_t displsz32(uint8_t);

//
// initialization
//
void c_x86_startcpu(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    x86_init_opcode_table();
    b_mmu_init(&cpu->mmu);

    cpu->EAX = 0; cpu->ECX = 0; cpu->EDX = 0; cpu->EBX = 0; cpu->ESI = 0;
    cpu->EDI = 0; cpu->ESP = 0; cpu->EBP = 0; cpu->EIP = 0;

    cpu->CS = 0; cpu->SS = 0; cpu->DS = 0; cpu->ES = 0; cpu->FS = 0; cpu->GS = 0;

    cpu->eflags.f_AC = 0; cpu->eflags.f_VM = 0; cpu->eflags.f_RF = 0;
    cpu->eflags.f_NT = 0; cpu->eflags.f_IOPL = 0; cpu->eflags.f_OF = 0;
    cpu->eflags.f_DF = 0; cpu->eflags.f_IF = 0; cpu->eflags.f_TF = 0;
    cpu->eflags.f_SF = 0; cpu->eflags.f_ZF = 0; cpu->eflags.f_AF = 0;
    cpu->eflags.f_PF = 0; cpu->eflags.reserved1 = 1; cpu->eflags.f_CF = 0;

    cpu->reg_table[EAX] = &cpu->EAX; cpu->reg_table[ECX] = &cpu->ECX;
    cpu->reg_table[EDX] = &cpu->EDX; cpu->reg_table[EBX] = &cpu->EBX;
    cpu->reg_table[ESP] = &cpu->ESP; cpu->reg_table[EBP] = &cpu->EBP;
    cpu->reg_table[ESI] = &cpu->ESI; cpu->reg_table[EDI] = &cpu->EDI;
    cpu->reg_table[EIP] = &cpu->EIP;
    cpu->eflags_ptr = &cpu->eflags;

    cpu->sreg_table[CS] = &cpu->CS; cpu->sreg_table[SS] = &cpu->SS;
    cpu->sreg_table[DS] = &cpu->DS; cpu->sreg_table[ES] = &cpu->ES;
    cpu->sreg_table[FS] = &cpu->FS; cpu->sreg_table[GS] = &cpu->GS;
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

// exception handling
static addr_t faulty_addr = 0;
static const char *errstr = NULL;

void c_x86_raise_exception(x86CPU *cpu, int exct)
{
    ASSERT(cpu != NULL);

    switch (exct) {
        case INT_UD:
            c_x86_stopcpu(cpu);
            if (errstr) {
                s_error(1, "emulator: Invalid Instruction at 0x%08x (%s)", cpu->EIP, errstr);
            } else {
                s_error(1, "emulator: Invalid Instruction at 0x%08x", cpu->EIP);
            }
        case INT_PF:
            c_x86_stopcpu(cpu);
            if (errstr) {
                s_error(1, "emulator: Page Fault (%s)", errstr);
            } else {
                s_error(1, "emulator: Page Fault at 0x%08x", (uint32_t)faulty_addr);
            }
        default:
            break;
    }
}

void c_x86_raise_exception_d(x86CPU *cpu, int exct, addr_t fault_addr, const char *desc)
{
    ASSERT(cpu != NULL);

    faulty_addr = fault_addr;
    errstr = desc;
    c_x86_raise_exception(cpu, exct);
}

///////////////

void c_x86_print_cpustate(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    struct instruction ins = cpu->e_state.last_instr;

    s_info("[   -----------------------------------------------------   ]");

    s_info("\n\033[1;36mEIP:\033[0m 0x%08x \033[1;90m( %02x %s )\033[0m", cpu->EIP, ins.data.opc, ins.name);
    s_info("\033[1;97mESP:\033[0m 0x%08x     \033[1;97mEBP:\033[0m 0x%08x", cpu->ESP, cpu->EBP);
    s_info("\033[1;34mEAX:\033[0m 0x%08x     \033[1;34mECX:\033[0m 0x%08x     \033[1;34mEDX:\033[0m 0x%08x", cpu->EAX, cpu->ECX, cpu->EDX);
    s_info("\033[1;34mEBX:\033[0m 0x%08x     \033[1;34mESI:\033[0m 0x%08x     \033[1;34mEDI:\033[0m 0x%08x", cpu->EBX, cpu->ESI, cpu->EDI);
    s_info("[ %s %s %s %s %s %s %s %s %s ]\n",
            cpu->eflags.f_OF ? "\033[1;32mOF\033[0m" : "\033[1;90mOF\033[0m",
            cpu->eflags.f_DF ? "\033[1;32mDF\033[0m" : "\033[1;90mDF\033[0m",
            cpu->eflags.f_IF ? "\033[1;32mIF\033[0m" : "\033[1;90mIF\033[0m",
            cpu->eflags.f_TF ? "\033[1;32mTF\033[0m" : "\033[1;90mTF\033[0m",
            cpu->eflags.f_SF ? "\033[1;32mSF\033[0m" : "\033[1;90mSF\033[0m",
            cpu->eflags.f_ZF ? "\033[1;32mZF\033[0m" : "\033[1;90mZF\033[0m",
            cpu->eflags.f_AF ? "\033[1;32mAF\033[0m" : "\033[1;90mAF\033[0m",
            cpu->eflags.f_PF ? "\033[1;32mPF\033[0m" : "\033[1;90mPF\033[0m",
            cpu->eflags.f_CF ? "\033[1;32mCF\033[0m" : "\033[1;90mCF\033[0m"
            );

    addr_t stack = cpu->ESP;

    s_info("[STACK]");
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x", stack + 20, b_mmu_read32(&cpu->mmu, stack + 20));
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x", stack + 16, b_mmu_read32(&cpu->mmu, stack + 16));
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x", stack + 12, b_mmu_read32(&cpu->mmu, stack + 12));
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x", stack + 8, b_mmu_read32(&cpu->mmu, stack + 8));
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x", stack + 4, b_mmu_read32(&cpu->mmu, stack + 4));
    s_info("\033[1;37m0x%08lx:\033[0m  0x%08x\n", stack, b_mmu_read32(&cpu->mmu, stack));
}

int modrm2sreg(uint8_t modrm)
{
    return reg(modrm);
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


addr_t c_x86_effctvaddr32(x86CPU *cpu, uint8_t modrm, uint8_t sib, uint32_t imm)
{
    ASSERT(cpu != NULL);
    uint8_t mod = (modrm & MODRM_MOD_BIT_MASK) >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;
    addr_t effctvaddr = 0;
    (void) sib;

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

int effctvreg(uint8_t modrm)
{
    int reg = -1;
    uint8_t mod = (modrm & MODRM_MOD_BIT_MASK) >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;

    if (mod == 3) {
        switch (rm) {
            case 0b000: reg = EAX; break;
            case 0b001: reg = ECX; break;
            case 0b010: reg = EDX; break;
            case 0b011: reg = EBX; break;
            case 0b100: reg = ESP; break;
            case 0b101: reg = EBP; break;
            case 0b110: reg = ESI; break;
            case 0b111: reg = EDI; break;
        }
    }

    return reg;
}


static uint8_t displsz16(uint8_t modrm)
{
    uint8_t mod = (modrm & MODRM_MOD_BIT_MASK) >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;
    uint8_t sz = 0;

    if (mod == 1)
        sz = 8;
    else if (mod == 2 || (mod == 0 && rm == 0b110))
        sz = 16;

    return sz;
}

static uint8_t displsz32(uint8_t modrm)
{
    uint8_t mod = (modrm & MODRM_MOD_BIT_MASK) >> 6;
    uint8_t rm = modrm & MODRM_RM_BIT_MASK;
    uint8_t sz = 0;

    if (mod == 1)
        sz = 8;
    else if (mod == 2 || (mod == 0 && rm == 0b101))
        sz = 32;

    return sz;
}


static uint32_t rdmemx(x86CPU *cpu, addr_t effctvaddr, int size)
{
    ASSERT(cpu != NULL);
    uint32_t bytes = 0;

    switch (size) {
        case 8:
            bytes = b_mmu_read8(&cpu->mmu, effctvaddr);
            break;
        case 16:
            bytes = b_mmu_read16(&cpu->mmu, effctvaddr);
            break;
        case 32:
            bytes = b_mmu_read32(&cpu->mmu, effctvaddr);
            break;
    }

    if (B_mmu_error(&cpu->mmu)) {
        c_x86_raise_exception_d(cpu, INT_PF, effctvaddr, B_mmu_errstr(&cpu->mmu));
    }

    return bytes;
}

static void wrmemx(x86CPU *cpu, addr_t effctvaddr, uint32_t src, int size)
{
    ASSERT(cpu != NULL);

    switch (size) {
        case 8:
            b_mmu_write8(&cpu->mmu, src, effctvaddr);
            break;
        case 16:
            b_mmu_write16(&cpu->mmu, src, effctvaddr);
            break;
        case 32:
            b_mmu_write32(&cpu->mmu, src, effctvaddr);
            break;
    }

    if (B_mmu_error(&cpu->mmu))
        c_x86_raise_exception_d(cpu, INT_PF, effctvaddr, B_mmu_errstr(&cpu->mmu));
}

// memory reading/writing
uint8_t c_x86_rdmem8(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 8);
}
uint16_t c_x86_rdmem16(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 16);
}
uint32_t c_x86_rdmem32(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 32);
}

void c_x86_wrmem8(x86CPU *cpu, addr_t effctvaddr, uint8_t byte)
{
    wrmemx(cpu, effctvaddr, byte, 8);
}

void c_x86_wrmem16(x86CPU *cpu, addr_t effctvaddr, uint16_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 16);
}

void c_x86_wrmem32(x86CPU * cpu, addr_t effctvaddr, uint32_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 32);
}

// the atomic versions will be left to be worked on when we start to support processes and threads
uint8_t c_x86_atomic_rdmem8(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 8);
}

uint16_t c_x86_atomic_rdmem16(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 16);
}

uint32_t c_x86_atomic_rdmem32(x86CPU *cpu, addr_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 32);
}

void c_x86_atomic_wrmem8(x86CPU *cpu, addr_t effctvaddr, uint8_t byte)
{
    wrmemx(cpu, effctvaddr, byte, 8);
}

void c_x86_atomic_wrmem16(x86CPU *cpu, addr_t effctvaddr, uint16_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 16);
}

void c_x86_atomic_wrmem32(x86CPU * cpu, addr_t effctvaddr, uint32_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 32);
}

//////////////

#define update_eip8(cpu) (cpu)->EIP+=1
#define update_eip16(cpu) (cpu)->EIP+=2
#define update_eip32(cpu) (cpu)->EIP+=4
#define set_fetch_err(instr, byte) \
    do {        \
    } while (0)

// this will fetch the instruction and update the EIP accordingly
static void fetch(x86CPU *cpu, struct instruction *instr)
{
    uint8_t byte;
    int encoding;
    struct opcode *table = x86_opcode_table;
    struct opcode op;
    struct exec_data data;
    uint32_t old_eip = cpu->EIP;
    uint8_t last_prefix = 0;

    data.ext = 0;
    data.sec = 0;
    data.opc = 0;
    data.modrm = 0;
    data.sib = 0;
    data.imm1 = 0;
    data.imm2 = 0;
    data.oprsz_pfx = 0;
    data.adrsz_pfx = 0;
    data.lock = 0;
    data.repnz = 0;
    data.rep = 0;
    data.segovr = 0;

    instr->fail_to_fetch = 0;
    instr->fail_byte = 0;

    for (size_t isprefix = 1; isprefix != 0;) {
        byte = b_mmu_fetch(&cpu->mmu, cpu->EIP);
        if (B_mmu_error(&cpu->mmu))
            c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
        update_eip8(cpu);
        if (x86_byteispfx(byte)) {

            if (byte == PFX_OPRSZ)
                data.oprsz_pfx = 1;
            else if (byte == PFX_ADDRSZ)
                data.adrsz_pfx = 1;
            else if (byte == PFX_LOCK)
                data.lock = 1;
            else if (byte == PFX_REPNZ)
                data.repnz = 1;
            else if (byte == PFX_REP)
                data.rep = 1;
            else
                data.segovr = byte2segovr(byte);

            last_prefix = byte;
            byte = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            if (x86_byteispfx(byte))
                continue;

            update_eip8(cpu);
        }
        isprefix = 0;
    }
    // handles prefixes
    // 'two-byte' instructions
    if (byte == 0x0F) {
        table = x86_opcode_0f_table;

        byte = b_mmu_fetch(&cpu->mmu, cpu->EIP);
        if (B_mmu_error(&cpu->mmu))
            c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
        update_eip8(cpu);

    }

    op = table[byte];
    data.opc = byte;

    // handle instructions with secondary bytes (see AAM/AAD)
    if (op.o_sec_table) {
        byte = b_mmu_fetch(&cpu->mmu, cpu->EIP);
        if (B_mmu_error(&cpu->mmu))
            c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));

        for (size_t i = 0; i < op.o_sec_tablesz; i++) {
            if (op.o_sec_table[i].o_opcode == byte) {

                data.sec = byte;
                op = op.o_sec_table[i];
                update_eip8(cpu);
            }
        }
    }

    // handle Mod/RM, SIB and opcode extensions
    if (op.o_use_rm) {
        data.modrm = b_mmu_fetch(&cpu->mmu, cpu->EIP);
        if (B_mmu_error(&cpu->mmu))
            c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
        update_eip8(cpu);

        if (op.o_use_op_extension) {
            if (op.o_extensions[reg(data.modrm)].o_opcode)
                op = op.o_extensions[reg(data.modrm)];
        }

        switch (mod(data.modrm)) {
            case 0: // MOD 00 RM 100
            case 1: // MOD 01 RM 100
            case 2: // MOD 10 RM 100
                if (rm(data.modrm) == 4) {
                    data.sib = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip8(cpu);
                }
                break;
            default:
                break;
        }
    }

    // handle instructions with type <prefix> <primary> [secondary]
    if (op.o_prefix && last_prefix) {
        if (table == x86_opcode_table) {
            if (last_prefix == op.o_prefix->o_opcode)
                op = *op.o_prefix;
        } else if (table == x86_opcode_0f_table) {
            if (last_prefix == op.o_prefix[last_prefix & TABLE_0F_PREFIX_MASK].o_opcode)
                op = op.o_prefix[last_prefix & TABLE_0F_PREFIX_MASK];
        }

        if (op.o_sec_table) {
            byte = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));

            for (size_t i = 0; i < op.o_sec_tablesz; i++) {
                if (op.o_sec_table[i].o_opcode == byte) {

                    data.sec = byte;
                    op = op.o_sec_table[i];
                    update_eip8(cpu);
                }
            }
        }
    }

    // select a encoding according to the operand-size
    if (data.oprsz_pfx)
        encoding = op.o_encoding16bit;
    else
        encoding = op.o_encoding;

    uint8_t dispsz;
    switch (encoding) {
        case rm8_imm8:
        case rm8_xmm2_imm8:
        case mm_r32m16_imm8:
        case mm1_mm2m64_imm8:
        case rm16_imm8:
        case rm16_xmm1_imm8:
        case rm16_r16_imm8:
        case rm32_r32_imm8:
        case rm32_imm8:
        case rm32_xmm1_imm8:
        case rm32_xmm2_imm8:
        case r16_rm16_imm8:
        case r32_rm32_imm8:
        case xmm_r32m16_imm8:
        case xmm1_r32m8_imm8:
        case xmm1_xmm2m32_imm8:
        case xmm1_xmm2m64_imm8:
        case xmm1_xmm2m128_imm8:
        case rm16_imm16:
        case r16_rm16_imm16:
            if (data.adrsz_pfx)
                dispsz = displsz16(data.modrm);
            else
                dispsz = displsz32(data.modrm);

            if (!dispsz) {
                data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                if (B_mmu_error(&cpu->mmu))
                    c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                update_eip8(cpu);
            } else {
                if (dispsz == 8) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip8(cpu);
                } else if (dispsz == 16) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip16(cpu);
                } else {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip32(cpu);
                }
                data.imm2 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                if (B_mmu_error(&cpu->mmu))
                    c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                update_eip8(cpu);
            }
            break;
        case imm8:
        case imm8_AL:
        case imm8_AX:
        case imm8_eAX:
        case AL_imm8:
        case AX_imm8:
        case eAX_imm8:
        case mm_imm8:
        case mm1_imm8:
        case r32_mm_imm8:
        case r32_xmm_imm8:
        case rela8:
        case xmm1_imm8:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip8(cpu);
            break;
        case imm16:
        case imm16_AX:
        case AX_imm16:
        case rela16:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip16(cpu);
            break;
        case eAX_imm32:
        case rela32:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip32(cpu);
            break;
        case rela16_16:
        case ptr16_16:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip16(cpu);
            data.imm2 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip16(cpu);
            break;
        case rela16_32:
        case ptr16_32:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip16(cpu);
            data.imm2 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip32(cpu);
            break;
        case imm16_imm8:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip16(cpu);
            data.imm2 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip8(cpu);
            break;
        case imm32:
        case imm32_eAX:
            data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
            if (B_mmu_error(&cpu->mmu))
                c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
            update_eip32(cpu);
            break;
        case rm32_imm32:
        case r32_rm32_imm32:
            if (data.adrsz_pfx)
                dispsz = displsz16(data.modrm);
            else
                dispsz = displsz32(data.modrm);

            if (!dispsz) {
                data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                if (B_mmu_error(&cpu->mmu))
                    c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                update_eip8(cpu);
            } else {
                if (dispsz == 8) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip8(cpu);
                } else if (dispsz == 16) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip16(cpu);
                } else {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip32(cpu);
                }
                data.imm2 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                if (B_mmu_error(&cpu->mmu))
                    c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                update_eip8(cpu);
            }
            break;
        case bnd_sib:
        case sib_bnd:
            if (!data.sib) {
                data.sib = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                if (B_mmu_error(&cpu->mmu))
                    c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                update_eip8(cpu);
            }
            break;
        case bnd_rm32:
        case bnd1m64_bnd2:
        case bnd1_bnd2m64:
        case m8:
        case m16:
        case m32:
        case m64:
        case rm8:
        case rm8_1:
        case rm8_CL:
        case rm8_r8:
        case mm1_mm2m32:
        case mm1_mm2m64:
        case mm_xmm1m64:
        case mm_xmm1m128:
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
        case m16_16:
        case m16_32:
        case m32_r32:
        case m64_mm:
        case m64_xmm1:
        case m128_xmm1:
        case mm_m64:
        case mm_rm32:
        case r16_m16:
        case r16_rm8:
        case r16_rm16:
        case r16_r16m16:
        case r32m16:
        case r32_rm32:
        case r32_rm8:
        case r32_rm16:
        case r32_r32m16:
        case xmm1_xmm2m16:
        case xmm1_xmm2m32:
        case xmm1_xmm2m64:
        case xmm1_xmm2m128:
        case xmm1m64_xmm2:
        case xmm2m32_xmm1:
        case xmm2m64_xmm1:
        case xmm2m128_xmm1:
        case xmm_rm32:
        case xmm1_m32:
        case xmm1_m64:
        case xmm1_m128:
        case xmm1_mm1m64:
        case xmm1_rm32:
        case xmm1_r32m32:
        case sreg_rm16:
        case r32_xmm1m32:
        case r32_xmm1m64:
        case r32_m32:
        case r16_m16_16:
        case r32_m16_32:
            if (data.adrsz_pfx)
                dispsz = displsz16(data.modrm);
            else
                dispsz = displsz32(data.modrm);

            if (dispsz) {
                if (dispsz == 8) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip8(cpu);
                } else if (dispsz == 16) {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip16(cpu);
                } else {
                    data.imm1 = b_mmu_fetch(&cpu->mmu, cpu->EIP);
                    if (B_mmu_error(&cpu->mmu))
                        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));
                    update_eip32(cpu);
                }
            }
            break;
        case AX_r16:
        case eAX_r32:
        case mm_xmm:
        case mm1_mm2:
        case r16:
        case r32:
        case r32_mm:
        case r32_xmm:
        case OP:
        case xmm_mm:
        case xmm1_mm:
        case xmm1_xmm2:
            break;
        case no_encoding:
        default:
            c_x86_raise_exception(cpu, INT_UD);
            return;
    }

    instr->name = op.o_name;
    instr->handler = op.o_handler;
    instr->data = data;
    instr->size = cpu->EIP - old_eip;
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

    instr.name = NULL;
    instr.size = 0;
    instr.handler = NULL;

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

        cpu->e_state.last_instr = instr;

        c_x86_print_cpustate(cpu);

        instr.handler(cpu, instr.data);
        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    c_x86_stopcpu(cpu);
    exit(0);
}
