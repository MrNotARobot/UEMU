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

#include "../generic-elf.h"
#include "../sym-resolver.h"
#include "../tracer.h"
#include "../conf.h"

#include "x86-types.h"
#include "x86-mmu.h"
#include "x86-utils.h"
#include "instructions.h"

typedef struct {
    x86MMU mmu;
    GenericELF executable;
    sym_resolver_t resolver;
    cpu_state_t tracer;
    config_t configuration;

    reg32_t EAX;
    reg32_t EBX;
    reg32_t ECX;
    reg32_t EDX;
    reg32_t EBP;
    reg32_t ESP;
    reg32_t EDI;
    reg32_t ESI;
    reg32_t EIP;

    reg16_t CS;
    reg16_t SS;
    reg16_t DS;
    reg16_t ES;
    reg16_t FS;
    reg16_t GS;

    struct EFlags eflags;

    struct EFlags *eflags_ptr_;
    reg16_t *sreg_table_[6];
} x86CPU;

    // member access macros
#define x86_tracer(cpu) (&((x86CPU *)(cpu))->tracer)
#define x86_mmu(cpu) (&((x86CPU *)(cpu))->mmu)
#define x86_elf(cpu) (&((x86CPU *)(cpu))->executable)
#define x86_resolver(cpu) (&((x86CPU *)(cpu))->resolver)
#define x86_conf(cpu) (&((x86CPU *)(cpu))->configuration)

enum x86ExceptionsInterrupts {
    INT_UD,     // invalid instruction
    INT_PF,     // Page Fault
};

void x86_startcpu(x86CPU *);
void x86_stopcpu(x86CPU *);

// the main loop
void x86_cpu_exec(char *, int argc, char **, char **);

void x86_raise_exception(x86CPU *, int);
void x86_raise_exception_d(x86CPU *, int, moffset32_t, const char *);

// don't use the MMU interface direcly for reading/writing (USE THESE)
uint8_t x86_readM8(x86CPU *, moffset32_t);
uint16_t x86_readM16(x86CPU *, moffset32_t);
uint32_t x86_readM32(x86CPU *, moffset32_t);

// Sames as read but doesn't raise a exception if fails
uint8_t x86_try_readM8(x86CPU *, moffset32_t);
uint16_t x86_try_readM16(x86CPU *, moffset32_t);
uint32_t x86_try_readM32(x86CPU *, moffset32_t);

void x86_writeM8(x86CPU *, moffset32_t, uint8_t);
void x86_writeM16(x86CPU *, moffset32_t, uint16_t);
void x86_writeM32(x86CPU *, moffset32_t, uint32_t);

uint8_t x86_atomic_readM8(x86CPU *, moffset32_t);
uint16_t x86_atomic_readM16(x86CPU *, moffset32_t);
uint32_t x86_atomic_readM32(x86CPU *, moffset32_t);

void x86_atomic_writeM8(x86CPU *, moffset32_t, uint8_t);
void x86_atomic_writeM16(x86CPU *, moffset32_t, uint16_t);
void x86_atomic_writeM32(x86CPU *, moffset32_t, uint32_t);

// write a sequence of bytes
void x86_wrseq(x86CPU *, moffset32_t, const uint8_t *, size_t);
// read a sequence of bytes into the buffer
void x86_rdseq(x86CPU *, moffset32_t, uint8_t *, size_t);
// same as above but will stop if it reaches the stop byte
void x86_rdseq2(x86CPU *, moffset32_t, uint8_t *, size_t, uint8_t);

enum PointerTypes {
    NOT_A_PTR,
    DATA_PTR,
    STACK_PTR,
    CODE_PTR
};

const uint8_t *x86_getptr(x86CPU *, moffset32_t);
int x86_ptrtype(x86CPU *, moffset32_t);


void x86_increment_eip(x86CPU *, moffset16_t);
void x86_update_eip_absolute(x86CPU *, moffset32_t);

// write to registers
void x86_writeR8(x86CPU *, uint8_t, uint8_t);
void x86_writeR16(x86CPU *, uint8_t, uint16_t);
void x86_writeR32(x86CPU *, uint8_t, uint32_t);

// read from registers
uint8_t x86_readR8(x86CPU *, uint8_t);
uint16_t x86_readR16(x86CPU *, uint8_t);
uint32_t x86_readR32(x86CPU *, uint8_t);

void x86_setflag(x86CPU *, uint8_t);
void x86_clearflag(x86CPU *, uint8_t);
_Bool x86_flag_on(x86CPU *, uint8_t);
_Bool x86_flag_off(x86CPU *, uint8_t);


#define x86_rdsreg(cpu, reg) *(    ((x86CPU *)(cpu))->sreg_table_[reg]    )

#define x86_wrsreg(cpu, reg, val) (    *(((x86CPU *)(cpu))->sreg_table_[reg]) = (val)    )

#endif /* CPU_H */
