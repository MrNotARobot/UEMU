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

#include "instructions.h"
#include <stdint.h>

#include "../basicmmu.h"

typedef uint16_t reg16_t;
typedef uint32_t reg32_t;

typedef struct {
    char *f_sym;
    addr_t f_val;
} callstack_record_t;

struct exec_state {
    struct instruction last_instr;
    callstack_record_t *callstack;
};

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

    struct exec_state e_state;

    reg32_t *reg_table[9];
    struct EFlags *eflags_ptr;
    reg16_t *sreg_table[6];
} x86CPU;

enum x86ExceptionsInterrupts {
    INT_UD,     // invalid instruction
    INT_PF,     // Page Fault
};

enum x86Registers {
    EAX = 0,
    AX = EAX,
    ECX = 1,
    CX = ECX,
    EDX = 2,
    DX = EDX,
    EBX = 3,
    BX = EBX,
    ESP = 4,
    SP = ESP,
    EBP = 5,
    BP = EBP,
    ESI = 6,
    SI = ESI,
    EDI = 7,
    DI = EDI,
    EIP = 8,
    AL, AH,
    CL, CH,
    DL, DH,
    BL, BH,
};

enum x86SegmentRegisters {
    CS,
    SS,
    DS,
    ES,
    FS,
    GS
};


void c_x86_startcpu(x86CPU *);
void c_x86_stopcpu(x86CPU *);

// the main loop
void c_x86_cpu_exec(char *);

void c_x86_raise_exception(x86CPU *, int);
void c_x86_raise_exception_d(x86CPU *, int, addr_t, const char *);

int modrm2sreg(uint8_t);
int reg8islb(int);
int reg8to32(int);
int effctvreg(uint8_t);

void c_x86_print_cpustate(x86CPU *);

addr_t c_x86_effctvaddr16(x86CPU *, uint8_t, uint32_t);
addr_t c_x86_effctvaddr32(x86CPU *, uint8_t, uint8_t, uint32_t);

// don't use the MMU interface direcly for reading/writing (USE THESE)
uint8_t c_x86_rdmem8(x86CPU *, addr_t);
uint16_t c_x86_rdmem16(x86CPU *, addr_t);
uint32_t c_x86_rdmem32(x86CPU *, addr_t);

void c_x86_wrmem8(x86CPU *, addr_t, uint8_t);
void c_x86_wrmem16(x86CPU *, addr_t, uint16_t);
void c_x86_wrmem32(x86CPU *, addr_t, uint32_t);

uint8_t c_x86_atomic_rdmem8(x86CPU *, addr_t);
uint16_t c_x86_atomic_rdmem16(x86CPU *, addr_t);
uint32_t c_x86_atomic_rdmem32(x86CPU *, addr_t);

void c_x86_atomic_wrmem8(x86CPU *, addr_t, uint8_t);
void c_x86_atomic_wrmem16(x86CPU *, addr_t, uint16_t);
void c_x86_atomic_wrmem32(x86CPU *, addr_t, uint32_t);


// I like this function-like way of accessing the registers
// read the 8 low/high bits
#define C_x86_rdreg8(cpu, reg) reg8islb(reg) ? \
    (  *(((x86CPU *)(cpu))->reg_table[reg8to32(reg)]) & 0x000000ff  ) : \
    (  *(((x86CPU *)(cpu))->reg_table[reg8to32(reg)]) & 0x0000ff00  )
// read the 16-bit register from a 32-bit register (i.e. eAX[ah])
#define C_x86_rdreg16(cpu, reg) (  *(((x86CPU *)(cpu))->reg_table[reg]) & 0x0000ffff  )
// read the register
#define C_x86_rdreg32(cpu, reg) (  *(((x86CPU *)(cpu))->reg_table[reg])  )

#define C_x86_rdsreg(cpu, reg) *(    ((x86CPU *)(cpu))->sreg_table[reg]    )

// write to the 8 low bits of a register
#define C_x86_wrreg8(cpu, reg, val) \
do { \
    if (reg8islb (reg)) \
        (  *(((x86CPU *)(cpu))->reg_table[reg8to32(reg)]) =\
        (  *((x86CPU *)(cpu))->reg_table[reg8to32(reg)] & 0xffffff00  ) | ((val) & 0x000000ff)  );\
   else \
        (  *(((x86CPU *)(cpu))->reg_table[reg8to32(reg)]) = \
        (  *((x86CPU *)(cpu))->reg_table[reg8to32(reg)] & 0xffff00ff  ) | ((val) & 0x0000ff00)  ); \
} while (0)

// write to the 16-bit register from a 32-bit register
#define C_x86_wrreg16(cpu, reg, val) (  *(((x86CPU *)(cpu))->reg_table[reg]) =  \
        (  *((x86CPU *)(cpu))->reg_table[reg] & 0xffff0000  ) | ((val) & 0x0000ffff)  )
#define C_x86_wrreg32(cpu, reg, val)(   *(((x86CPU *)(cpu))->reg_table[reg]) = (val)    )

#define C_x86_wrsreg(cpu, reg, val) (    *(((x86CPU *)(cpu))->sreg_table[reg]) = (val)    )

#define C_x86_setflag(cpu, flag) (  ((x86CPU *)(cpu))->eflags_ptr->f_ ## flag = 1  )
#define C_x86_clearflag(cpu, flag) (  ((x86CPU *)(cpu))->eflags_ptr->f_ ## flag = 0  )

#endif /* CPU_H */
