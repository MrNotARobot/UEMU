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
 *  handles showing execution data to users.
 */

#include <string.h>

#include "tracer.h"
#include "system.h"

#include "memory.h"

static void add_to_cache(cpu_state_t *, struct symbol_lookup_record);

//
// Initialization
//

void tracer_start(cpu_state_t *tracer, sym_resolver_t *resolver)
{

    if (!tracer)
        return;

    memset(tracer, 0, sizeof(*tracer));

    tracer->backtrace = xcalloc(TRACER_BACKTRACE_SIZE_INCREMENT, sizeof(*tracer->backtrace));
    tracer->backtracesz = TRACER_BACKTRACE_SIZE_INCREMENT;
    tracer->backtraceptr = 0;
    tracer->cacheptr = 0;
    tracer->resolver = resolver;
}


void tracer_stop(cpu_state_t *tracer)
{
    if (!tracer)
        return;

    xfree(tracer->backtrace);
}

//
// set/get the actual trace data
//

uint32_t tracer_get(cpu_state_t *tracer, int variable)
{
    if (!tracer)
        return 0;

    switch (variable) {
        case TRACE_VAR_EAX: return tracer->registers.eax;
        case TRACE_VAR_EBX: return tracer->registers.ebx;
        case TRACE_VAR_EDX: return tracer->registers.edx;
        case TRACE_VAR_ECX: return tracer->registers.ecx;
        case TRACE_VAR_EDI: return tracer->registers.edi;
        case TRACE_VAR_ESI: return tracer->registers.esi;
        case TRACE_VAR_EBP: return tracer->registers.ebp;
        case TRACE_VAR_ESP: return tracer->registers.esp;
        case TRACE_VAR_EIP: return tracer->registers.eip;
    }

    return 0;
}

void tracer_set(cpu_state_t *tracer, int variable, uint32_t value)
{
    if (!tracer)
        return;

    switch (variable) {
        case TRACE_VAR_EAX: tracer->registers.eax = (reg32_t)value; break;
        case TRACE_VAR_EBX: tracer->registers.ebx = (reg32_t)value; break;
        case TRACE_VAR_EDX: tracer->registers.edx = (reg32_t)value; break;
        case TRACE_VAR_ECX: tracer->registers.ecx = (reg32_t)value; break;
        case TRACE_VAR_EDI: tracer->registers.edi = (reg32_t)value; break;
        case TRACE_VAR_ESI: tracer->registers.esi = (reg32_t)value; break;
        case TRACE_VAR_EBP: tracer->registers.ebp = (reg32_t)value; break;
        case TRACE_VAR_ESP: tracer->registers.esp = (reg32_t)value; break;
        case TRACE_VAR_EIP:
            tracer->registers.eip = (reg32_t)value;
            moffset32_t start = tracer->backtrace[tracer->backtraceptr].st_start;
            tracer->backtrace[tracer->backtraceptr].st_rel = (reg32_t)value - start;
            break;
    }
}

void tracer_setptr(cpu_state_t *tracer, int variable, void *value)
{
    if (!tracer)
        return;

    switch (variable) {
        case TRACE_VARPTR_EFLAGS: tracer->registers.eflags = *(struct EFlags *)value; break;
    }
}

inline traced_registers_t tracer_get_savedstate(cpu_state_t *tracer)
{
    if (tracer)
        return tracer->registers;
    // yes, I wrote this
    return (traced_registers_t){0, 0, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
}

inline backtrace_record_t tracer_get_backtrace(cpu_state_t *tracer, uint32_t index)
{
    if (!tracer)
        return (backtrace_record_t){NULL, 0, 0, 0, 0, 0, 0};

    if (index > tracer->backtraceptr)
        return (backtrace_record_t){NULL, 0, 0, 0, 0, 0, 0};

    return tracer->backtrace[index];
}

inline uint32_t tracer_get_backtrace_size(cpu_state_t *tracer)
{
    if (tracer)
        return tracer->backtraceptr;
    return 0;
}


//
// backtrace
//

inline static void add_to_cache(cpu_state_t *tracer, struct symbol_lookup_record symbol)
{

    if (tracer->cacheptr == TRACER_LOOKUP_CACHE_SIZE)
        tracer->cacheptr = 0;

    tracer->cache[tracer->cacheptr++] = symbol;
}


void tracer_push(cpu_state_t *tracer, moffset32_t vaddr, moffset32_t returnvaddr, reg32_t stackframe)
{
    size_t st_ptr = 0;
    struct symbol_lookup_record *symbol = NULL;

    if (!tracer)
        return;

    // only increment starting at 1
    if (tracer->backtrace[0].st_start)
        st_ptr = ++tracer->backtraceptr;

    if (tracer->backtraceptr == tracer->backtracesz) {
        tracer->backtracesz += TRACER_BACKTRACE_SIZE_INCREMENT;
        tracer->backtrace = xreallocarray(tracer->backtrace, tracer->backtracesz, sizeof(*tracer->backtrace));
    }

    // First search our cache
    for (size_t i = 0; i < TRACER_LOOKUP_CACHE_SIZE; i++) {
        size_t sl_size = tracer->cache[i].sl_size;
        moffset32_t sl_start = tracer->cache[i].sl_start;

        if (!sl_start)
            break;

        if (vaddr >= sl_start && vaddr <= (sl_start + sl_size)) {
            symbol = &tracer->cache[i];
            break;
        }
    }

    if (!symbol) {
        struct symbol_lookup_record lookup = sr_lookup(tracer->resolver, vaddr);

        if (lookup.sl_start)
            add_to_cache(tracer, lookup);
        symbol = &lookup;
    }

    tracer->backtrace[st_ptr].st_symbol = symbol->sl_name;
    tracer->backtrace[st_ptr].st_start = symbol->sl_start;
    tracer->backtrace[st_ptr].st_end = symbol->sl_start + symbol->sl_size;
    tracer->backtrace[st_ptr].st_rel = vaddr - symbol->sl_start;
    tracer->backtrace[st_ptr].st_return = returnvaddr;
    tracer->backtrace[st_ptr].st_frame = stackframe;
    time(&tracer->backtrace[st_ptr].st_starttime);
}

inline void tracer_pop(cpu_state_t *tracer)
{
    if (tracer->backtraceptr == 0)
        return;
    tracer->backtraceptr--;
}
