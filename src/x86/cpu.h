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

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "../basicmmu.h"

typedef uint16_t reg16_t;
typedef uint32_t reg32_t;

struct exception {
};

typedef struct {
    BasicMMU mmu;
    GenericELF executable;
    struct exception last_exception;

    reg32_t EAX;
    reg32_t ECX;
    reg32_t EDX;
    reg32_t EBX;
    reg32_t ESI;
    reg32_t EDI;
    reg32_t ESP;
    reg32_t EBP;

    reg32_t EIP;

    struct {
        reg32_t reserved : 10;
        reg32_t ID : 1;
        reg32_t VIP : 1;
        reg32_t VIF : 1;
        reg32_t AC : 1;
        reg32_t VM : 1;
        reg32_t RF : 1;
        reg32_t reserved15 : 1;
        reg32_t NT : 1;
        reg32_t IOPL : 2;
        reg32_t OF : 1;
        reg32_t DF : 1;
        reg32_t IF : 1;
        reg32_t TF : 1;
        reg32_t SF : 1;
        reg32_t ZF : 1;
        reg32_t reserved5 : 1;
        reg32_t AF : 1;
        reg32_t reserved3 : 1;
        reg32_t PF : 1;
        reg32_t reserved1 : 1;
        reg32_t CF : 1;
    } eflags;
} x86CPU;


void c_x86_startcpu(x86CPU *);
void c_x86_stopcpu(x86CPU *);

// the main loop
void c_x86_cpu_exec(char *);


// I like this function-like way of accessing the registers
// read the 8 low bits of a register (i.e. eAX[al])
#define C_x86_rdregl8(cpu, reg) ((cpu)->(reg) & 0x000000ff)
// read the 8 high bits of a register (i.e. eAX[ah])
#define C_x86_rdregh8(cpu, reg) ((cpu)->(reg) & 0x0000ff00)
// read the 16-bit register from a 32-bit register (i.e. eAX[ah])
#define C_x86_rdreg16(cpu, reg) ((cpu)->(reg) & 0x0000ffff)
// read the register
#define C_x86_rdreg32(cpu, reg) (cpu)->(reg)

// write to the 8 low bits of a register
#define C_x86_wrregl8(cpu, reg, val) ((cpu)->(reg) = ((cpu)->(reg) & 0xffffff00) | ((val) & 0x000000ff))
// write to the 8 high bits of a register
#define C_x86_wrregh8(cpu, reg, val) ((cpu)->(reg) = ((cpu)->(reg) & 0xffff00ff) | ((val) & 0x000000ff))
// write to the 16-bit register from a 32-bit register
#define C_x86_wrreg16(cpu, reg, val) ((cpu)->(reg) = ((cpu)->(reg) & 0xffff0000) | ((val) & 0x0000ffff))
#define C_x86_wrreg32(cpu, reg, val) ((cpu)->(reg) = (val))

#define C_x86_setflag(cpu, eflags_flag) ((cpu)->eflags.(eflags_reg) = 1)
#define C_x86_clearflag(cpu, eflags_flag) ((cpu)->eflags.(eflags_reg) = 0)

#endif /* CPU_H */
