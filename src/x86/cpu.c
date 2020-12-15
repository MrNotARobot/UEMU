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

#include "cpu.h"
#include "instructions.h"   // for the initialization function
#include "../system.h"
#include "../memory.h"

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

    cpu->eflags.AC = 0;
    cpu->eflags.VM = 0;
    cpu->eflags.RF = 0;
    cpu->eflags.NT = 0;
    cpu->eflags.IOPL = 0;
    cpu->eflags.OF = 0;
    cpu->eflags.DF = 0;
    cpu->eflags.IF = 0;
    cpu->eflags.TF = 0;
    cpu->eflags.SF = 0;
    cpu->eflags.ZF = 0;
    cpu->eflags.AF = 0;
    cpu->eflags.PF = 0;
    cpu->eflags.reserved1 = 1;
    cpu->eflags.CF = 0;
}


void c_x86_stopcpu(x86CPU *cpu)
{
    ASSERT(cpu != NULL);

    x86_free_opcode_table();
    if (cpu->executable.g_name)   // if we have a executable, then free it
        g_elf_unload(&cpu->executable);
    b_mmu_release_recs(&cpu->mmu);

    xfree(cpu);
}


/*
 * this is the main part.
 *
 * NOTE: executable should be malloc'ed buffer as this function does not return to the caller.
 */
void c_x86_cpu_exec(char *executable)
{
    x86CPU *cpu;

    int stack_flags = 0;

    if (!executable)
        s_error(1, "invalid reference at c_x86_cpu_exec");

    cpu = xcalloc(1, sizeof(*cpu));
    c_x86_startcpu(cpu);

    // map the executable segments to memory.
    // no shared library is loaded just yet.
    g_elf_load(&cpu->executable, executable);
    if (G_elf_error(&cpu->executable))
        s_error(1, "%s", G_elf_errstr(&cpu->executable));

    // we don't need it anymore.
    xfree(executable);

    // actually map the segments
    b_mmu_mmap_loadable(&cpu->executable, &cpu->mmu);
    if (B_mmu_error(&cpu->mmu))
        s_error(1, "%s", B_mmu_errstr(&cpu->mmu));

    // create a stack
    if (G_elf_execstack(&cpu->executable))
        stack_flags |= B_STACKEXEC;

    cpu->ESP = cpu->EBP = b_mmu_create_stack(&cpu->mmu, stack_flags);

    c_x86_stopcpu(cpu);
    exit(0);
}
