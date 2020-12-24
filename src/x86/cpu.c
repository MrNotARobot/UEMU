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

#include "../memory.h"
#include "../system.h"

#include "cpu.h"
#include "dbg.h"
#include "disassembler.h"

static uint32_t readMx(x86CPU *cpu, moffset32_t vaddr, int size, _Bool);
static void writeMx(x86CPU *cpu, moffset32_t vaddr, uint32_t src, int size);
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
    tracer_start(&cpu->tracer, &cpu->resolver);

    cpu->EAX = 0; cpu->ECX = 0; cpu->EDX = 0; cpu->EBX = 0; cpu->ESI = 0;
    cpu->EDI = 0; cpu->ESP = 0; cpu->EBP = 0; cpu->EIP = 0;

    cpu->CS = 0; cpu->SS = 0; cpu->DS = 0; cpu->ES = 0; cpu->FS = 0; cpu->GS = 0;

    cpu->eflags.AC = 0; cpu->eflags.VM = 0; cpu->eflags.RF = 0;
    cpu->eflags.NT = 0; cpu->eflags.IOPL = 0; cpu->eflags.OF = 0;
    cpu->eflags.DF = 0; cpu->eflags.IF = 0; cpu->eflags.TF = 0;
    cpu->eflags.SF = 0; cpu->eflags.ZF = 0; cpu->eflags.AF = 0;
    cpu->eflags.PF = 0; cpu->eflags.CF = 0;

    cpu->eflags_ptr_ = &cpu->eflags;

    cpu->sreg_table_[CS] = &cpu->CS; cpu->sreg_table_[SS] = &cpu->SS;
    cpu->sreg_table_[DS] = &cpu->DS; cpu->sreg_table_[ES] = &cpu->ES;
    cpu->sreg_table_[FS] = &cpu->FS; cpu->sreg_table_[GS] = &cpu->GS;

    conf_start(x86_conf(cpu));
    conf_add(x86_conf(cpu), "executable", "EXECUTABLE", 0, CONF_TP_STRING, CONF_REQUIRED, CONF_NO_ARG, NULL, 0);
    conf_end(x86_conf(cpu));
}


void x86_stopcpu(x86CPU *cpu)
{
    if (!cpu)
        return;

    x86_free_opcode_table();
    sr_closecache(x86_resolver(cpu));
    tracer_stop(x86_tracer(cpu));
    conf_freetables(x86_conf(cpu));
    elf_unload(x86_elf(cpu));
    mmu_unloadall(x86_mmu(cpu));

}

// exception handling
static moffset32_t faulty_addr = 0;
static const char *errstr = NULL;

void x86_raise_exception(x86CPU *cpu, int exct)
{
    reg32_t saved_eip;

    if (!cpu)
        return;

    saved_eip = x86_readR32(cpu, EIP);
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
                s_error(1, "emulator: Program received signal SIGSEGV (%s)", errstr);
            } else {
                s_error(1, "emulator: Program received signal SIGSEGV at 0x%08x", faulty_addr);
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



static uint32_t readMx(x86CPU *cpu, moffset32_t vaddr, int size, _Bool tryread)
{
    uint32_t bytes = 0;

    if (!cpu)
        return 0;

    switch (size) {
        case 8:
            bytes = mmu_read8(&cpu->mmu, vaddr);
            break;
        case 16:
            bytes = mmu_read16(&cpu->mmu, vaddr);
            break;
        case 32:
            bytes = mmu_read32(&cpu->mmu, vaddr);
            break;
    }

    if (!tryread && mmu_error(&cpu->mmu))
        x86_raise_exception_d(cpu, INT_PF, vaddr, mmu_errstr(&cpu->mmu));

    mmu_clrerror(&cpu->mmu);

    return bytes;
}

static void writeMx(x86CPU *cpu, moffset32_t vaddr, uint32_t src, int size)
{
    if (!cpu)
        return;

    switch (size) {
        case 8:
            mmu_write8(&cpu->mmu, src, vaddr);
            break;
        case 16:
            mmu_write16(&cpu->mmu, src, vaddr);
            break;
        case 32:
            mmu_write32(&cpu->mmu, src, vaddr);
            break;
    }

    if (mmu_error(&cpu->mmu))
        x86_raise_exception_d(cpu, INT_PF, vaddr, mmu_errstr(&cpu->mmu));
}

//
// memory reading
//

uint8_t x86_readM8(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 8, 0);
}

uint16_t x86_readM16(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 16, 0);
}

uint32_t x86_readM32(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 32, 0);
}

uint8_t x86_try_readM8(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 8, 1);
}

uint16_t x86_try_readM16(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 16, 1);
}

uint32_t x86_try_readM32(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 32, 1);
}

//
//  write to memory
//

void x86_writeM8(x86CPU *cpu, moffset32_t vaddr, uint8_t byte)
{
    writeMx(cpu, vaddr, byte, 8);
}

void x86_writeM16(x86CPU *cpu, moffset32_t vaddr, uint16_t bytes)
{
    writeMx(cpu, vaddr, bytes, 16);
}

void x86_writeM32(x86CPU * cpu, moffset32_t vaddr, uint32_t bytes)
{
    writeMx(cpu, vaddr, bytes, 32);
}

// the atomic versions will be left to be worked on when we start to support processes and threads
uint8_t x86_atomic_readM8(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 8, 0);
}

uint16_t x86_atomic_readM16(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 16, 0);
}

uint32_t x86_atomic_readM32(x86CPU *cpu, moffset32_t vaddr)
{
    return readMx(cpu, vaddr, 32, 0);
}

void x86_atomic_writeM8(x86CPU *cpu, moffset32_t vaddr, uint8_t byte)
{
    writeMx(cpu, vaddr, byte, 8);
}

void x86_atomic_writeM16(x86CPU *cpu, moffset32_t vaddr, uint16_t bytes)
{
    writeMx(cpu, vaddr, bytes, 16);
}

void x86_atomic_writeM32(x86CPU * cpu, moffset32_t vaddr, uint32_t bytes)
{
    writeMx(cpu, vaddr, bytes, 32);
}

void x86_wrseq(x86CPU *cpu, moffset32_t vaddr, const uint8_t *buffer, size_t size)
{
    if (!cpu)
        return;

    for (size_t i = 0; i < size; i++) {
        x86_writeM8(cpu, vaddr, buffer[i]);
        vaddr++;
    }
}

void x86_rrseq(x86CPU *cpu, moffset32_t vaddr, uint8_t *dest, size_t size)
{
    if (!cpu)
        return;

    for (size_t i = 0; i < size; i++) {
        dest[i] = x86_readM8(cpu, vaddr);
        vaddr++;
    }
}

//
// Deal with registers
//

void x86_increment_eip(x86CPU *cpu, moffset16_t offset)
{
    x86_update_eip_absolute(cpu, cpu->EIP + offset);
}

void x86_update_eip_absolute(x86CPU *cpu, moffset32_t vaddr)
{
    if (!cpu)
        return;

    cpu->EIP = vaddr;
    tracer_set(&cpu->tracer, TRACE_VAR_EIP, cpu->EIP);
}

// write to registers
void x86_writeR8(x86CPU *cpu, uint8_t register8, uint8_t value)
{

    if (!cpu)
        return;

    switch (register8) {
        case AL: 
            cpu->EAX = (cpu->EAX & 0xffffff00) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EAX, cpu->EAX);
            break;
        case AH:
            cpu->EAX = (cpu->EAX & 0xffff00ff) | (value << 4);
            tracer_set(&cpu->tracer, TRACE_VAR_EAX, cpu->EAX);
            break;
        case BL: 
            cpu->EBX = (cpu->EBX & 0xffffff00) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EBX, cpu->EBX);
            break;
        case BH:
            cpu->EBX = (cpu->EBX & 0xffff00ff) | (value << 4);
            tracer_set(&cpu->tracer, TRACE_VAR_EBX, cpu->EBX);
            break;
        case CL:
            cpu->ECX = (cpu->ECX & 0xffffff00) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_ECX, cpu->ECX);
            break;
        case CH:
            cpu->ECX = (cpu->ECX & 0xffff00ff) | (value << 4);
            tracer_set(&cpu->tracer, TRACE_VAR_ECX, cpu->ECX);
            break;
        case DL:
            cpu->EDX = (cpu->EDX & 0xffffff00) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EDX, cpu->EDX);
            break;
        case DH:
            cpu->EDX = (cpu->EDX & 0xffff00ff) | (value << 4);
            tracer_set(&cpu->tracer, TRACE_VAR_EDX, cpu->EDX);
            break;
    }
}

void x86_writeR16(x86CPU *cpu, uint8_t register16, uint16_t value)
{

    if (!cpu)
        return;

    switch (register16) {
        case AX:
            cpu->EAX = (cpu->EAX & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EAX, cpu->EAX);
            break;
        case BX:
            cpu->EBX = (cpu->EBX & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EBX, cpu->EBX);
            break;
        case CX:
            cpu->ECX = (cpu->ECX & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_ECX, cpu->ECX);
            break;
        case DX:
            cpu->EDX = (cpu->EDX & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EDX, cpu->EDX);
            break;
        case SP:
            cpu->ESP = (cpu->ESP & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_ESP, cpu->ESP);
            break;
        case BP:
            cpu->EBP = (cpu->EBP & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EBP, cpu->EBP);
            break;
        case DI:
            cpu->EDI = (cpu->EDI & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_EDI, cpu->EDI);
            break;
        case SI:
            cpu->ESI = (cpu->ESI & 0xffff0000) | value;
            tracer_set(&cpu->tracer, TRACE_VAR_ESI, cpu->ESI);
            break;
    }
}

void x86_writeR32(x86CPU *cpu, uint8_t register32, uint32_t value)
{
    if (!cpu)
        return;

    switch (register32) {
        case EIP:
            x86_update_eip_absolute(cpu, value);
            break;
        case EAX:
            cpu->EAX = value;
            tracer_set(&cpu->tracer, TRACE_VAR_EAX, cpu->EAX);
            break;
        case EBX:
            cpu->EBX = value;
            tracer_set(&cpu->tracer, TRACE_VAR_EBX, cpu->EBX);
            break;
        case ECX:
            cpu->ECX = value;
            tracer_set(&cpu->tracer, TRACE_VAR_ECX, cpu->ECX);
            break;
        case EDX:
            cpu->EDX = value;
            tracer_set(&cpu->tracer, TRACE_VAR_EDX, cpu->EDX);
            break;
        case ESP:
            cpu->ESP = value;
            tracer_set(&cpu->tracer, TRACE_VAR_ESP, cpu->ESP);
            break;
        case EBP:
            cpu->EBP = value;
            tracer_set(&cpu->tracer, TRACE_VAR_EBP, cpu->EBP);
            break;
        case EDI:
            cpu->EDI = value;
            tracer_set(&cpu->tracer, TRACE_VAR_EDI, cpu->EDI);
            break;
        case ESI:
            cpu->ESI = value;
            tracer_set(&cpu->tracer, TRACE_VAR_ESI, cpu->ESI);
            break;
    }
}

// read from registers
uint8_t x86_readR8(x86CPU *cpu, uint8_t register8)
{
    if (!cpu)
        return 0;

    switch (register8) {
        case AL: return cpu ->EAX & 0x000000ff;
        case AH: return cpu ->EAX & 0x0000ff00;
        case BL: return cpu ->EBX & 0x000000ff;
        case BH: return cpu ->EBX & 0x0000ff00;
        case CL: return cpu ->ECX & 0x000000ff;
        case CH: return cpu ->ECX & 0x0000ff00;
        case DL: return cpu ->EDX & 0x000000ff;
        case DH: return cpu ->EDX & 0x0000ff00;
    }

    return 0;
}

uint16_t x86_readR16(x86CPU *cpu, uint8_t register16)
{
    if (!cpu)
        return 0;

    return x86_readR32(cpu, register16) & 0x0000ffff;
}

uint32_t x86_readR32(x86CPU *cpu, uint8_t register32)
{
    if (!cpu)
        return 0;

    switch (register32) {
        case EIP: return cpu->EIP;
        case EAX: return cpu ->EAX;
        case EBX: return cpu ->EBX;
        case ECX: return cpu ->ECX;
        case EDX: return cpu ->EDX;
        case ESP: return cpu ->ESP;
        case EBP: return cpu ->EBP;
        case EDI: return cpu ->EDI;
        case ESI: return cpu ->ESI;
    }

    return 0;
}

void x86_setflag(x86CPU *cpu, uint8_t flag)
{
    switch (flag) {
        case ID: cpu->eflags.ID = 1; break;
        case VIP: cpu->eflags.VIP = 1; break;
        case VIF: cpu->eflags.VIF = 1; break;
        case AC: cpu->eflags.AC = 1; break;
        case VM: cpu->eflags.VM = 1; break;
        case RF: cpu->eflags.RF = 1; break;
        case NT: cpu->eflags.NT = 1; break;
        case IOPL: cpu->eflags.IOPL = 1; break;
        case DF: cpu->eflags.DF = 1; break;
        case IF: cpu->eflags.IF = 1; break;
        case TF: cpu->eflags.TF = 1; break;
        case SF: cpu->eflags.SF = 1; break;
        case ZF: cpu->eflags.ZF = 1; break;
        case AF: cpu->eflags.AF = 1; break;
        case PF: cpu->eflags.PF = 1; break;
        case CF: cpu->eflags.CF = 1; break;
            break;
    }

    tracer_setptr(x86_tracer(cpu), TRACE_VARPTR_EFLAGS, &cpu->eflags);
}

void x86_clearflag(x86CPU *cpu, uint8_t flag)
{
    switch (flag) {
        case ID: cpu->eflags.ID = 0; break;
        case VIP: cpu->eflags.VIP = 0; break;
        case VIF: cpu->eflags.VIF = 0; break;
        case AC: cpu->eflags.AC = 0; break;
        case VM: cpu->eflags.VM = 0; break;
        case RF: cpu->eflags.RF = 0; break;
        case NT: cpu->eflags.NT = 0; break;
        case IOPL: cpu->eflags.IOPL = 0; break;
        case DF: cpu->eflags.DF = 0; break;
        case IF: cpu->eflags.IF = 0; break;
        case TF: cpu->eflags.TF = 0; break;
        case SF: cpu->eflags.SF = 0; break;
        case ZF: cpu->eflags.ZF = 0; break;
        case AF: cpu->eflags.AF = 0; break;
        case PF: cpu->eflags.PF = 0; break;
        case CF: cpu->eflags.CF = 0; break;
        break;
    }

    tracer_setptr(x86_tracer(cpu), TRACE_VARPTR_EFLAGS, &cpu->eflags);
}

_Bool x86_flag_on(x86CPU *cpu, uint8_t flag)
{
    return !x86_flag_off(cpu, flag);
}

_Bool x86_flag_off(x86CPU *cpu, uint8_t flag)
{
    switch (flag) {
        case ID: return  cpu->eflags.ID == 0; break;
        case VIP: return  cpu->eflags.VIP == 0; break;
        case VIF: return  cpu->eflags.VIF == 0; break;
        case AC: return  cpu->eflags.AC == 0; break;
        case VM: return  cpu->eflags.VM == 0; break;
        case RF: return  cpu->eflags.RF == 0; break;
        case NT: return  cpu->eflags.NT == 0; break;
        case IOPL: return  cpu->eflags.IOPL == 0; break;
        case DF: return  cpu->eflags.DF == 0; break;
        case IF: return  cpu->eflags.IF == 0; break;
        case TF: return  cpu->eflags.TF == 0; break;
        case SF: return  cpu->eflags.SF == 0; break;
        case ZF: return  cpu->eflags.ZF == 0; break;
        case AF: return  cpu->eflags.AF == 0; break;
        case PF: return  cpu->eflags.PF == 0; break;
        case CF: return  cpu->eflags.CF == 0; break;
        break;
    }

    return 0;
}

const uint8_t *x86_getptr(x86CPU *cpu, moffset32_t vaddr)
{
    if (!cpu)
        return NULL;

    return mmu_getptr(&cpu->mmu, vaddr);
}

int x86_ptrtype(x86CPU *cpu, moffset32_t vaddr)
{
    int type;

    if (!cpu)
        return 0;

    type = mmu_ptrtype(&cpu->mmu, vaddr);
    if (type == ST_RODATA || type == ST_RWDATA)
        return DATA_PTR;
    else if (type == ST_RXCODE || type == ST_XOCODE || type == ST_RWXCODE)
        return CODE_PTR;
    else if (type == ST_RWSTACK || type == ST_RWXSTACK)
        return STACK_PTR;
    else
        return NOT_A_PTR;
}

//////////////

static void build_environment(x86CPU *cpu, int argc, char *argv[], char **envp)
{
    size_t environsz = 0;
    moffset32_t *environ_;
    moffset32_t *argv_;
    uint8_t alignment = 0;

    argv[0] = realpath(argv[0], NULL);
    if (!argv[0])
        argv[0] = find_executable(argv[0]);

    for (size_t i = 0; envp[i] != NULL; i++)
        environsz++;

    environ_ = xcalloc(environsz + 1, sizeof (*environ_));

    // write the environment variables to the stack
    for (int i = environsz - 1; i >= 0; i--)
    {
        size_t envsz = strlen(envp[i]) + 1;

        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - envsz);

        x86_wrseq(cpu, x86_readR32(cpu, ESP), (uint8_t *)envp[i], envsz);
        environ_[i] = x86_readR32(cpu, ESP);
    }

    argv_ = alloca((argc + 1) * sizeof(*argv_));

    // write the arguments to the stack
    for (int i = argc - 1; i >= 0; i--) {
        size_t argsz = strlen(argv[i]) + 1;

        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - argsz);

        x86_wrseq(cpu, cpu->ESP, (uint8_t *)argv[i], argsz);

        argv_[i-1] = x86_readR32(cpu, ESP);
    }


    // align the stack
    alignment = ((cpu->ESP & 0xfffffff0) - ((environsz + argc + 2)*4 + 4)) & 0x0000000f;
    cpu->ESP = ((cpu->ESP) & 0xfffffff0) - alignment;

    // the NULL ptr at the end of envp
    environ_[environsz] = 0;

    for (int i = environsz; i >= 0; i--) {
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - 4);

        x86_writeM32(cpu, cpu->ESP, environ_[i]);
    }

    // the NULL ptr at the end of argv
    argv_[argc] = 0;

    for (int i = argc; i >= 0; i--) {
        x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - 4);
        x86_writeM32(cpu, cpu->ESP, argv_[i]);
    }


    x86_writeR32(cpu, ESP, x86_readR32(cpu, ESP) - 4);
    x86_writeM32(cpu, cpu->ESP, argc);

    s_info("%p", argv[0]);
    xfree(argv[0]);
    s_info("%p", environ_);
    xfree(environ_);
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
    int start_argv;

    instr.name = NULL;
    instr.size = 0;
    instr.handler = NULL;

    int stack_flags = 0;

    if (!executable || !argv || !envp)
        return;

    memset(&cpu, 0, sizeof(cpu));
    x86_startcpu(&cpu);

    start_argv = conf_parse_argv(x86_conf(&cpu), argv);
    argc = argc - start_argv;

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

    x86_writeR32(&cpu, ESP, mmu_create_stack(&cpu.mmu, stack_flags));

    build_environment(&cpu, argc, &argv[start_argv], envp);

    x86_writeR32(&cpu, EIP, elf_entrypoint(&cpu.executable));

    tracer_set(&cpu.tracer, TRACE_VAR_ESP, cpu.ESP);
    tracer_set(&cpu.tracer, TRACE_VAR_EIP, cpu.EIP);

    tracer_push(&cpu.tracer, cpu.EIP, 0, cpu.ESP);

    while (1) {

        x86dbg_print_state(&cpu);

        instr = x86_decode(&cpu, cpu.EIP);

        if (mmu_error(&cpu.mmu))
            x86_raise_exception_d(&cpu, INT_PF, cpu.EIP, mmu_errstr(&cpu.mmu));

        if (instr.fail_to_fetch)
            x86_raise_exception(&cpu, INT_UD);

        x86_increment_eip(&cpu, instr.size);

        instr.handler(&cpu, instr.data);

        // TODO: handle exceptions? I think it would be cool to imitate a real x86 cpu
        // handling of exceptions
    }

    x86_stopcpu(&cpu);
    exit(0);
}
