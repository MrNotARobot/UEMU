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

#ifndef TRACER_H
#define TRACER_H

#include <time.h>

#include "types.h"
#include "sym-resolver.h"
#include "x86/x86-types.h"

typedef struct {
    const char *st_symbol;
    moffset32_t st_start;
    moffset32_t st_end;
    moffset16_t st_rel;
    time_t st_starttime;
    moffset32_t st_return;
    reg32_t st_frame;
} backtrace_record_t;

#define TRACER_BACKTRACE_SIZE_INCREMENT 10
#define TRACER_LOOKUP_CACHE_SIZE 10

typedef struct {
    reg32_t eax;
    reg32_t ebx;
    reg32_t edx;
    reg32_t ecx;
    reg32_t edi;
    reg32_t esi;
    reg32_t ebp;
    reg32_t esp;
    reg32_t eip;
    struct EFlags eflags;
} traced_registers_t;

typedef struct {
    traced_registers_t registers;

    backtrace_record_t *backtrace;
    uint32_t backtracesz;
    uint32_t backtraceptr;

    sym_resolver_t *resolver;
    struct symbol_lookup_record cache[TRACER_LOOKUP_CACHE_SIZE];
    uint8_t cacheptr;
} cpu_state_t;

void tracer_start(cpu_state_t *, sym_resolver_t *);
void tracer_stop(cpu_state_t *);

enum TracerVariables {
    TRACE_VAR_EAX,
    TRACE_VAR_EBX,
    TRACE_VAR_EDX,
    TRACE_VAR_ECX,
    TRACE_VAR_EDI,
    TRACE_VAR_ESI,
    TRACE_VAR_EBP,
    TRACE_VAR_ESP,
    TRACE_VAR_EIP,
    TRACE_VARPTR_EFLAGS
};

uint32_t tracer_get(cpu_state_t *, int);
void tracer_set(cpu_state_t *, int, uint32_t);
void tracer_setptr(cpu_state_t *, int, void *);
traced_registers_t tracer_get_savedstate(cpu_state_t *);
backtrace_record_t tracer_get_backtrace(cpu_state_t *, uint32_t);
uint32_t tracer_get_backtrace_size(cpu_state_t *);

void tracer_push(cpu_state_t *, moffset32_t, moffset32_t, reg32_t);
void tracer_pop(cpu_state_t *);

#endif /* TRACER_H */
