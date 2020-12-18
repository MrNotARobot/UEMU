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
 * Description:
 *  defines used by the emulated x86 cpu.
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "instructions.h"
#include "cpustat.h"
#include "../basicmmu.h"
#include "../types.h"
#include "x86-utils.h"

typedef struct {
    BasicMMU mmu;
    GenericELF executable;

    reg32_t EAX;
    reg32_t ECX;
    reg32_t EDX;
    reg32_t EBX;
    reg32_t ESI;
    reg32_t EDI;
    reg32_t ESP;
    reg32_t EBP;

    reg32_t EIP;

    reg16_t CS;
    reg16_t SS;
    reg16_t DS;
    reg16_t ES;
    reg16_t FS;
    reg16_t GS;

    struct EFlags {
        reg32_t reserved : 10;
        reg32_t f_ID : 1;
        reg32_t f_VIP : 1;
        reg32_t f_VIF : 1;
        reg32_t f_AC : 1;
        reg32_t f_VM : 1;
        reg32_t f_RF : 1;
        reg32_t reserved15 : 1;
        reg32_t f_NT : 1;
        reg32_t f_IOPL : 2;
        reg32_t f_OF : 1;
        reg32_t f_DF : 1;
        reg32_t f_IF : 1;
        reg32_t f_TF : 1;
        reg32_t f_SF : 1;
        reg32_t f_ZF : 1;
        reg32_t reserved5 : 1;
        reg32_t f_AF : 1;
        reg32_t reserved3 : 1;
        reg32_t f_PF : 1;
        reg32_t reserved1 : 1;
        reg32_t f_CF : 1;
    } eflags;

    cpu_stat_t cpustat;

    addr_t environ_;
    addr_t argv_;
    int  argc_;

    reg32_t *reg_table_[9];
    struct EFlags *eflags_ptr_;
    reg16_t *sreg_table_[6];
} x86CPU;
    // member access macros
#define x86_cpustat(cpu) (&((x86CPU *)(cpu))->cpustat)
#define x86_mmu(cpu) (&((x86CPU *)(cpu))->mmu)
#define x86_elf(cpu) (&((x86CPU *)(cpu))->executable)

enum x86ExceptionsInterrupts {
        INT_UD,     // invalid instruction
        INT_PF,     // Page Fault
    };

    void x86_startcpu(x86CPU *);
    void x86_stopcpu(x86CPU *);

    // the main loop
    void x86_cpu_exec(char *, int argc, char **, char **);

    void x86_raise_exception(x86CPU *, int);
    void x86_raise_exception_d(x86CPU *, int, addr_t, const char *);

    // don't use the MMU interface direcly for reading/writing (USE THESE)
    uint8_t x86_rdmem8(x86CPU *, addr_t);
    uint16_t x86_rdmem16(x86CPU *, addr_t);
    uint32_t x86_rdmem32(x86CPU *, addr_t);

    void x86_wrmem8(x86CPU *, addr_t, uint8_t);
    void x86_wrmem16(x86CPU *, addr_t, uint16_t);
    void x86_wrmem32(x86CPU *, addr_t, uint32_t);

    uint8_t x86_atomic_rdmem8(x86CPU *, addr_t);
    uint16_t x86_atomic_rdmem16(x86CPU *, addr_t);
    uint32_t x86_atomic_rdmem32(x86CPU *, addr_t);

    void x86_atomic_wrmem8(x86CPU *, addr_t, uint8_t);
    void x86_atomic_wrmem16(x86CPU *, addr_t, uint16_t);
    void x86_atomic_wrmem32(x86CPU *, addr_t, uint32_t);

    // write a sequence of bytes
    void x86_wrseq(x86CPU *, addr_t, const uint8_t *, size_t);
    // read a sequence of bytes into the buffer
    void x86_rdseq(x86CPU *, addr_t, uint8_t *, size_t);
    // same as above but will stop if it reaches the stop byte
    void x86_rdseq2(x86CPU *, addr_t, uint8_t *, size_t, uint8_t);

    // I like this function-like way of accessing the registers
    // read the 8 low/high bits
#define x86_rdreg8(cpu, reg) reg8_islsb(reg) ? \
        (  *(((x86CPU *)(cpu))->reg_table_[reg8to32(reg)]) & 0x000000ff  ) : \
        (  *(((x86CPU *)(cpu))->reg_table_[reg8to32(reg)]) & 0x0000ff00  )
    // read the 16-bit register from a 32-bit register (i.e. eAX[ah])
#define x86_rdreg16(cpu, reg) (  *(((x86CPU *)(cpu))->reg_table_[reg]) & 0x0000ffff  )
    // read the register
#define x86_rdreg32(cpu, reg) (  *(((x86CPU *)(cpu))->reg_table_[reg])  )

#define x86_rdsreg(cpu, reg) *(    ((x86CPU *)(cpu))->sreg_table_[reg]    )

    // write to the 8 low bits of a register
#define x86_wrreg8(cpu, reg, val) \
    do { \
        if (reg8_islsb (reg)) \
            (  *(((x86CPU *)(cpu))->reg_table_[reg8to32(reg)]) =\
            (  *((x86CPU *)(cpu))->reg_table_[reg8to32(reg)] & 0xffffff00  ) | ((val) & 0x000000ff)  );\
       else \
            (  *(((x86CPU *)(cpu))->reg_table_[reg8to32(reg)]) = \
            (  *((x86CPU *)(cpu))->reg_table_[reg8to32(reg)] & 0xffff00ff  ) | ((val) & 0x0000ff00)  ); \
    } while (0)

    // write to the 16-bit register from a 32-bit register
#define x86_wrreg16(cpu, reg, val) (  *(((x86CPU *)(cpu))->reg_table_[reg]) =  \
            (  *((x86CPU *)(cpu))->reg_table_[reg] & 0xffff0000  ) | ((val) & 0x0000ffff)  )
#define x86_wrreg32(cpu, reg, val)(   *(((x86CPU *)(cpu))->reg_table_[reg]) = (val)    )

#define x86_wrsreg(cpu, reg, val) (    *(((x86CPU *)(cpu))->sreg_table_[reg]) = (val)    )

#define x86_setflag(cpu, flag) (  ((x86CPU *)(cpu))->eflags_ptr_->f_ ## flag = 1  )
#define x86_clearflag(cpu, flag) (  ((x86CPU *)(cpu))->eflags_ptr_->f_ ## flag = 0  )

#define x86_queryflag(cpu, flag) (   ((x86CPU *)(cpu))->eflags.f_ ## flag    )

#endif /* CPU_H */
