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
#include <ctype.h>

#include "cpu.h"
#include "../system.h"
#include "../memory.h"
#include "disassembler.h"

static uint32_t rdmemx(x86CPU *cpu, moffset32_t effctvaddr, int size);
static void wrmemx(x86CPU *cpu, moffset32_t effctvaddr, uint32_t src, int size);
static void fetch(x86CPU *, struct instruction *);
static void build_environment(x86CPU *, int, char **, char **);

//
// initialization
//
void x86_startcpu(x86CPU *cpu)
{
    if (!cpu)
        return;

    x86_init_opcode_table();
    mmu_init(&cpu->mmu);
    x86_cpustat_init(cpu);

    cpu->EAX = 0; cpu->ECX = 0; cpu->EDX = 0; cpu->EBX = 0; cpu->ESI = 0;
    cpu->EDI = 0; cpu->ESP = 0; cpu->EBP = 0; cpu->EIP = 0;

    cpu->CS = 0; cpu->SS = 0; cpu->DS = 0; cpu->ES = 0; cpu->FS = 0; cpu->GS = 0;

    cpu->eflags.f_AC = 0; cpu->eflags.f_VM = 0; cpu->eflags.f_RF = 0;
    cpu->eflags.f_NT = 0; cpu->eflags.f_IOPL = 0; cpu->eflags.f_OF = 0;
    cpu->eflags.f_DF = 0; cpu->eflags.f_IF = 0; cpu->eflags.f_TF = 0;
    cpu->eflags.f_SF = 0; cpu->eflags.f_ZF = 0; cpu->eflags.f_AF = 0;
    cpu->eflags.f_PF = 0; cpu->eflags.reserved1 = 1; cpu->eflags.f_CF = 0;

    cpu->reg_table_[EAX] = &cpu->EAX; cpu->reg_table_[ECX] = &cpu->ECX;
    cpu->reg_table_[EDX] = &cpu->EDX; cpu->reg_table_[EBX] = &cpu->EBX;
    cpu->reg_table_[ESP] = &cpu->ESP; cpu->reg_table_[EBP] = &cpu->EBP;
    cpu->reg_table_[ESI] = &cpu->ESI; cpu->reg_table_[EDI] = &cpu->EDI;
    cpu->reg_table_[EIP] = &cpu->EIP;
    cpu->eflags_ptr_ = &cpu->eflags;

    cpu->sreg_table_[CS] = &cpu->CS; cpu->sreg_table_[SS] = &cpu->SS;
    cpu->sreg_table_[DS] = &cpu->DS; cpu->sreg_table_[ES] = &cpu->ES;
    cpu->sreg_table_[FS] = &cpu->FS; cpu->sreg_table_[GS] = &cpu->GS;
}


void x86_stopcpu(x86CPU *cpu)
{
    if (!cpu)
        return;

    x86_free_opcode_table();
    sr_closecache(x86_resolver(cpu));

    elf_unload(&cpu->executable);
    mmu_unloadall(&cpu->mmu);

    xfree(cpu->cpustat.callstack);
}

// exception handling
static moffset32_t faulty_addr = 0;
static const char *errstr = NULL;

void x86_raise_exception(x86CPU *cpu, int exct)
{
    reg32_t saved_eip;

    if (!cpu)
        return;

    saved_eip = x86_rdreg32(cpu, EIP);
    x86_stopcpu(cpu);
    switch (exct) {
        case INT_UD:
            if (errstr) {
                s_error(1, "emulator: Invalid Instruction at 0x%08x (%s)", faulty_addr, errstr);
            } else {
                s_error(1, "emulator: Invalid Instruction at 0x%08x", saved_eip);
            }
            break;
        case INT_PF:
            if (errstr) {
                s_error(1, "emulator: Page Fault at 0x%08x (%s)", faulty_addr, errstr);
            } else {
                s_error(1, "emulator: Page Fault at 0x%08x", faulty_addr);
            }
            break;
        default:
            break;
    }
}

void x86_raise_exception_d(x86CPU *cpu, int exct, moffset32_t fault_addr, const char *desc)
{

    if (!cpu)
        return;

    faulty_addr = fault_addr;
    errstr = desc;
    x86_raise_exception(cpu, exct);
}

///////////////



static uint32_t rdmemx(x86CPU *cpu, moffset32_t effctvaddr, int size)
{
    uint32_t bytes = 0;

    if (!cpu)
        return 0;

    switch (size) {
        case 8:
            bytes = mmu_read8(&cpu->mmu, effctvaddr);
            break;
        case 16:
            bytes = mmu_read16(&cpu->mmu, effctvaddr);
            break;
        case 32:
            bytes = mmu_read32(&cpu->mmu, effctvaddr);
            break;
    }

    if (mmu_error(&cpu->mmu)) {
        x86_raise_exception_d(cpu, INT_PF, effctvaddr, mmu_errstr(&cpu->mmu));
    }

    return bytes;
}

static void wrmemx(x86CPU *cpu, moffset32_t effctvaddr, uint32_t src, int size)
{
    if (!cpu)
        return;

    switch (size) {
        case 8:
            mmu_write8(&cpu->mmu, src, effctvaddr);
            break;
        case 16:
            mmu_write16(&cpu->mmu, src, effctvaddr);
            break;
        case 32:
            mmu_write32(&cpu->mmu, src, effctvaddr);
            break;
    }

    if (mmu_error(&cpu->mmu))
        x86_raise_exception_d(cpu, INT_PF, effctvaddr, mmu_errstr(&cpu->mmu));
}

// memory reading/writing
uint8_t x86_rdmem8(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 8);
}
uint16_t x86_rdmem16(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 16);
}
uint32_t x86_rdmem32(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 32);
}

void x86_wrmem8(x86CPU *cpu, moffset32_t effctvaddr, uint8_t byte)
{
    wrmemx(cpu, effctvaddr, byte, 8);
}

void x86_wrmem16(x86CPU *cpu, moffset32_t effctvaddr, uint16_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 16);
}

void x86_wrmem32(x86CPU * cpu, moffset32_t effctvaddr, uint32_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 32);
}

// the atomic versions will be left to be worked on when we start to support processes and threads
uint8_t x86_atomic_rdmem8(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 8);
}

uint16_t x86_atomic_rdmem16(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 16);
}

uint32_t x86_atomic_rdmem32(x86CPU *cpu, moffset32_t effctvaddr)
{
    return rdmemx(cpu, effctvaddr, 32);
}

void x86_atomic_wrmem8(x86CPU *cpu, moffset32_t effctvaddr, uint8_t byte)
{
    wrmemx(cpu, effctvaddr, byte, 8);
}

void x86_atomic_wrmem16(x86CPU *cpu, moffset32_t effctvaddr, uint16_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 16);
}

void x86_atomic_wrmem32(x86CPU * cpu, moffset32_t effctvaddr, uint32_t bytes)
{
    wrmemx(cpu, effctvaddr, bytes, 32);
}

void x86_wrseq(x86CPU *cpu, moffset32_t effctvaddr, const uint8_t *buffer, size_t size)
{
    if (!cpu)
        return;

    for (size_t i = 0; i < size; i++) {
        x86_wrmem8(cpu, effctvaddr, buffer[i]);
        effctvaddr++;
    }
}

void x86_rrseq(x86CPU *cpu, moffset32_t effctvaddr, uint8_t *dest, size_t size)
{
    if (!cpu)
        return;

    for (size_t i = 0; i < size; i++) {
        dest[i] = x86_rdmem8(cpu, effctvaddr);
        effctvaddr++;
    }
}

//////////////

// this will fetch the instruction and update the EIP accordingly
static void fetch(x86CPU *cpu, struct instruction *instr)
{
    *instr = x86_decode(x86_mmu(cpu), x86_rdreg32(cpu, EIP));
    if (mmu_error(&cpu->mmu))
        x86_raise_exception_d(cpu, INT_PF, x86_rdreg32(cpu, EIP), mmu_errstr(&cpu->mmu));

    if (instr->fail_to_fetch)
        x86_raise_exception(cpu, INT_UD);

    x86_wrreg32(cpu, EIP, x86_rdreg32(cpu, EIP) + instr->size);
}

static void build_environment(x86CPU *cpu, int argc, char *argv[], char **envp)
{
    size_t environsz = 0;
    moffset32_t *environ_;
    moffset32_t *argv_;
    uint8_t alignment = 0;

    if (argc == 1) {
        x86_stopcpu(cpu);
        s_error(1, "emulator: invalid argv");
    }

    argc--; // we don't use argv[0]
    argv[1] = realpath(argv[1], NULL);
    if (!argv[1])
        argv[1] = find_executable(argv[1]);

    for (size_t i = 0; envp[i] != NULL; i++)
        environsz++;

    environ_ = xcalloc(environsz + 1, sizeof (*environ_));

    // write the environment variables to the stack
    for (int i = environsz - 1; i >= 0; i--)
    {
        size_t envsz = strlen(envp[i]) + 1;

        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - envsz);

        x86_wrseq(cpu, x86_rdreg32(cpu, ESP), (uint8_t *)envp[i], envsz);
        environ_[i] = x86_rdreg32(cpu, ESP);
    }

    argv_ = xcalloc(argc + 1, sizeof (*argv_));

    // write the arguments to the stack
    for (int i = argc; i >= 1; i--) {
        size_t argsz = strlen(argv[i]) + 1;

        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - argsz);

        x86_wrseq(cpu, cpu->ESP, (uint8_t *)argv[i], argsz);

        argv_[i-1] = x86_rdreg32(cpu, ESP);
    }

    // align the stack
    alignment = ((cpu->ESP & 0xfffffff0) - ((environsz + argc + 2)*4 + 4)) & 0x0000000f;
    cpu->ESP = ((cpu->ESP) & 0xfffffff0) - alignment;

    // the NULL ptr at the end of envp
    environ_[environsz] = 0;

    for (int i = environsz; i >= 0; i--) {
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - 4);

        x86_wrmem32(cpu, cpu->ESP, environ_[i]);
    }

    // the NULL ptr at the end of argv
    argv_[argc] = 0;

    for (int i = argc; i >= 0; i--) {
        x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - 4);
        x86_wrmem32(cpu, cpu->ESP, argv_[i]);
    }


    x86_wrreg32(cpu, ESP, x86_rdreg32(cpu, ESP) - 4);
    x86_wrmem32(cpu, cpu->ESP, argc);

    xfree(argv[1]);
    xfree(environ_);
    xfree(argv_);
}

/*
 * this is the main loop.
 *
 * NOTE: executable should be malloc'ed buffer as this function does not return to the caller.
 */
void x86_cpu_exec(char *executable, int argc, char *argv[], char **envp)
{
    x86CPU cpu;
    struct instruction instr;

    instr.name = NULL;
    instr.size = 0;
    instr.handler = NULL;

    int stack_flags = 0;

    if (!executable || !argv || !envp)
        return;

    memset(&cpu, 0, sizeof(cpu));
    x86_startcpu(&cpu);

    // map the executable segments to memory.
    // no shared library is loaded just yet.
    elf_load(&cpu.executable, executable);
    if (elf_error(&cpu.executable)) {
        x86_stopcpu(&cpu);
        s_error(1, "%s", elf_errstr(&cpu.executable));
    }

    // actually map the segments
    mmu_mmap_loadable(&cpu.mmu, &cpu.executable);
    if (mmu_error(&cpu.mmu)) {
        x86_stopcpu(&cpu);
        s_error(1, "%s", mmu_errstr(&cpu.mmu));
    }

    sr_loadcache(x86_resolver(&cpu), executable);

    // we don't need it anymore.
    xfree(executable);

    // create a stack
    if (elf_execstack(&cpu.executable))
        stack_flags |= B_STACKEXEC;

    x86_wrreg32(&cpu, ESP, mmu_create_stack(&cpu.mmu, stack_flags));

    x86_cpustat_set(&cpu, STAT_STACKTOP, x86_rdreg32(&cpu, ESP));

    build_environment(&cpu, argc, argv, envp);

    x86_wrreg32(&cpu, EIP, elf_entrypoint(x86_elf(&cpu)));

    x86_cpustat_push_callstack(&cpu, x86_rdreg32(&cpu, EIP));

    while (1) {
        x86_cpustat_set(&cpu, STAT_EIP, x86_rdreg32(&cpu, EIP));

        fetch(&cpu, &instr);

        x86_cpustat_set_current_op(&cpu, instr);
        x86_cpustat_update_callstack(&cpu);
        x86_cpustat_print(&cpu);

        instr.handler(&cpu, instr.data);
        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    x86_stopcpu(&cpu);
    exit(0);
}
