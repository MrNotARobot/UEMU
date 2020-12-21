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

#ifndef CPUSTATE_H
#define CPUSTATE_H

#include "../types.h"
#include "instructions.h"

typedef struct {
    const char *f_sym;
    moffset32_t f_val;
    uint32_t f_rel;   // offset from the beggining of the function
} callstack_record_t;

typedef struct {
    // this shows the eip of the current instruction before the actual execution
    // See like this:
    // 1. EIP = cpustat.eip = X
    // 2. instruction = fetch() & update EIP
    // 3. EIP = X + instruction.size, cpustat.eip = X
    // 4. execute(instruction)
    // 5. EIP = cpustat.eip = Y
    reg32_t eip;

    callstack_record_t *callstack;
    size_t callstacksz;
    size_t callstacktop;
    moffset32_t stacktop;
    struct instruction instr;
    _Bool queryfailed;
} cpu_stat_t;

enum X86CPUStatQuery {
    STAT_EIP,
    STAT_STACKTOP,
    STAT_CALLSTACKSZ,
    STAT_CALLSTACKTOP,
    STAT_CURRENTOP
};

_Bool x86_cpustat_queryfailed(void *);

void x86_cpustat_print(void *);
uint64_t x86_cpustat_query(void *, int);
const callstack_record_t *x86_cpustat_callstack_i(void *, uint8_t);
const callstack_record_t *x86_cpustat_callstack(void *);

// these functions should be used only by the cpu code
void x86_cpustat_init(void *);
void x86_cpustat_set(void *, int, uint64_t);
const struct instruction *c_86_cpustat_current_op(void *);
void x86_cpustat_set_current_op(void *, struct instruction);
void x86_cpustat_push_callstack(void *, moffset32_t);
void x86_cpustat_pop_callstack(void *);
void x86_cpustat_update_callstack(void *);

#endif /* CPUSTATE_H */
