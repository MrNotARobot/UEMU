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
#include "disassembler.h"

static uint32_t rdmemx(x86CPU *cpu, addr_t effctvaddr, int size);
static void wrmemx(x86CPU *cpu, addr_t effctvaddr, uint32_t src, int size);
static void fetch(x86CPU *, struct instruction *);

static size_t conf_x86cpu_callstack_maxsize = 10;

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

    cpu->e_state.callstack = xcalloc(conf_x86cpu_callstack_maxsize, sizeof(cpu->e_state.callstack));
    cpu->e_state.callstacksz = conf_x86cpu_callstack_maxsize;
}


void c_x86_stopcpu(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    x86_free_opcode_table();

    if (cpu->executable.g_name)   // if we have an executable, then free it
        g_elf_unload(&cpu->executable);
    b_mmu_release_recs(&cpu->mmu);

    xfree(cpu->e_state.callstack);
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

static void c_x86_updatecallstack(x86CPU *cpu)
{
    callstack_record_t *record = &cpu->e_state.callstack[cpu->e_state.callstacktop];
    record->f_rel = cpu->EIP - record->f_val - cpu->e_state.last_instr.size;
}

void c_x86_add2callstack(x86CPU *cpu, addr_t faddr)
{
    ASSERT(cpu != NULL);
    callstack_record_t *previous_record = &cpu->e_state.callstack[cpu->e_state.callstacktop];
    size_t i = cpu->e_state.callstacktop + 1;
    const func_rangemap_t *func;
    static _Bool first_call = 1;
    if (first_call)
        i = 0;

    if (i >= conf_x86cpu_callstack_maxsize)
        return;

    // set the address to the next instruction
    previous_record->f_rel = cpu->EIP - previous_record->f_val;

    func = b_mmu_findfunction(&cpu->mmu, faddr);

    s_info("%lx", faddr);
    if (func)
        cpu->e_state.callstack[i].f_sym = g_elf_getfromstrtab(&cpu->executable, func->fr_name);

    cpu->e_state.callstack[i].f_val = func->fr_start;
    cpu->e_state.callstack[i].f_rel = 0;
    if (!first_call)
        cpu->e_state.callstacktop += 1;

    if (first_call)
        first_call = 0;

}

void c_x86_print_cpustate(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    struct instruction ins = cpu->e_state.last_instr;
    char *inst_str = x86_disassemble(&ins);
    uint32_t fsym_rel = cpu->e_state.callstack[cpu->e_state.callstacktop].f_rel;
    const char *fsym = cpu->e_state.callstack[cpu->e_state.callstacktop].f_sym;
    const func_rangemap_t *func;
    const char *branch_fname;
    addr_t calltarget;

    if (strcmp(ins.name, "CALL") == 0) {
        char *temp = inst_str;
        calltarget = x86_findcalltarget(cpu, ins.data);
        func = b_mmu_findfunction(&cpu->mmu, calltarget);
        if (!func)
            branch_fname = fsym;
        else
            branch_fname = g_elf_getfromstrtab(&cpu->executable, func->fr_name);

        if (calltarget != func->fr_start) {   // an offset inside the function
            char *offset = int2str(calltarget - func->fr_start);
            inst_str = xcalloc(strlen(branch_fname) + strlen(inst_str) + + strlen(offset) + 5, sizeof(*inst_str));
            coolstrcat(inst_str, 6, temp, " <", branch_fname, "+", offset, ">");
            xfree(offset);
        } else {
            inst_str = xcalloc(strlen(branch_fname) + strlen(inst_str) + 4, sizeof(*inst_str));
            coolstrcat(inst_str, 4, temp, " <", branch_fname, ">");
        }

        xfree(temp);
    }

    s_info("[   ----------------------------- x86CPU -----------------------------   ]");

    s_info("\n\033[1;36mEIP:\033[0m 0x%08x \033[1;90m( %02x %s ) \033[1;31m<%s+0x%02x>\033[0m", cpu->e_state.last_eip, ins.data.opc, inst_str, fsym ? fsym : "", fsym_rel);
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
    s_info("\033[1m0x%08lx:\033[0m  0x%08x", stack + 20, b_mmu_read32(&cpu->mmu, stack + 20));
    s_info("\033[1m0x%08lx:\033[0m  0x%08x", stack + 16, b_mmu_read32(&cpu->mmu, stack + 16));
    s_info("\033[1m0x%08lx:\033[0m  0x%08x", stack + 12, b_mmu_read32(&cpu->mmu, stack + 12));
    s_info("\033[1m0x%08lx:\033[0m  0x%08x", stack + 8, b_mmu_read32(&cpu->mmu, stack + 8));
    s_info("\033[1m0x%08lx:\033[0m  0x%08x", stack + 4, b_mmu_read32(&cpu->mmu, stack + 4));
    s_info("\033[1m0x%08lx:\033[0m  0x%08x\n", stack, b_mmu_read32(&cpu->mmu, stack));

    s_info("[CALLTRACE]");
    callstack_record_t *record;
    for (size_t i = cpu->e_state.callstacktop; i != 0; i--) {
        record = &cpu->e_state.callstack[i];
        s_info("\033[1m0x%08lx \033[1;31m<%s+0x%02x>\033[0m", record->f_val + record->f_rel, record->f_sym, record->f_rel);
    }
        record = &cpu->e_state.callstack[0];
        s_info("\033[1m0x%08lx \033[1;31m<%s+0x%02x>\033[0m", record->f_val + record->f_rel, record->f_sym, record->f_rel);

    xfree(inst_str);
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
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
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
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    addr_t effctvaddr = 0;
    _Bool use_sib = 0;
    uint8_t ss_factor = sibss(sib);
    uint8_t index = sibindex(sib);
    uint8_t base = sibbase(sib);

    if (mod == 0) {
        switch (rm) {
            case 0b000: effctvaddr = C_x86_rdreg32(cpu, EAX); break;
            case 0b001: effctvaddr = C_x86_rdreg32(cpu, ECX); break;
            case 0b010: effctvaddr = C_x86_rdreg32(cpu, EDX); break;
            case 0b011: effctvaddr = C_x86_rdreg32(cpu, EBX); break;
            case 0b100: use_sib = 1; break;
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
            case 0b100: use_sib = 1; break;
            case 0b101: effctvaddr = C_x86_rdreg32(cpu, EBP) + imm; break;
            case 0b110: effctvaddr = C_x86_rdreg32(cpu, ESI) + imm; break;
            case 0b111: effctvaddr = C_x86_rdreg32(cpu, EDI) + imm; break;
        }
    } // mod == 3 are registers, so we return zero

    if (use_sib) {
        if (ss_factor == 0b00)
            ss_factor = 1;
        else if (ss_factor == 0b01)
            ss_factor = 2;
        else if (ss_factor == 0b10)
            ss_factor = 4;
        else if (ss_factor == 0b10)
            ss_factor = 4;

        if (base == EBP) {
            if (index != 0b100)
                effctvaddr = c_x86_rdmem32(cpu, C_x86_rdreg32(cpu, index) * ss_factor);

            effctvaddr += imm;

                if (mod)
                    effctvaddr += C_x86_rdreg32(cpu, EBP);
        } else {
            if (index != 0b100)
                effctvaddr = c_x86_rdmem32(cpu, C_x86_rdreg32(cpu, index) * ss_factor);
            effctvaddr += C_x86_rdreg32(cpu, base);
        }

    }

    return effctvaddr;
}

int effctvreg(uint8_t modrm)
{
    int reg = -1;
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);

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

#define set_fetch_err(instr, byte) \
    do {        \
    } while (0)

// this will fetch the instruction and update the EIP accordingly
static void fetch(x86CPU *cpu, struct instruction *instr)
{
    *instr = x86_decode(&cpu->mmu, cpu->EIP);
    if (B_mmu_error(&cpu->mmu))
        c_x86_raise_exception_d(cpu, INT_PF, cpu->EIP, B_mmu_errstr(&cpu->mmu));

    if (instr->fail_to_fetch)
        c_x86_raise_exception(cpu, INT_UD);
    cpu->EIP += instr->size;
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
    b_mmu_mmap_loadable(&cpu->mmu, &cpu->executable);
    if (B_mmu_error(&cpu->mmu)) {
        c_x86_stopcpu(cpu);
        s_error(1, "%s", B_mmu_errstr(&cpu->mmu));
    }

    // create a stack
    if (G_elf_execstack(&cpu->executable))
        stack_flags |= B_STACKEXEC;

    cpu->ESP = b_mmu_create_stack(&cpu->mmu, stack_flags);

    cpu->EIP = cpu->executable.g_entryp;

    c_x86_add2callstack(cpu, cpu->EIP);

    while (1) {
        cpu->e_state.last_eip = cpu->EIP;
        fetch(cpu, &instr);

        cpu->e_state.last_instr = instr;
        c_x86_updatecallstack(cpu);

        c_x86_print_cpustate(cpu);

        instr.handler(cpu, instr.data);
        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    c_x86_stopcpu(cpu);
    exit(0);
}
