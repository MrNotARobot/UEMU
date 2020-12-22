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
#include <ctype.h>

#include "../system.h"
#include "../memory.h"
#include "../generic-elf.h"
#include "../sym-resolver.h"
#include "disassembler.h"
#include "cpustat.h"
#include "cpu.h"

static _Bool ptr_has_string(void *, moffset32_t);
static void print_register(void *, uint8_t);
static char *getptrcontent(void *, moffset32_t);
static char *add_branch_target_to_instruction(moffset32_t, struct symbol_lookup_record, char *);
static char *disassembleptr(void *, moffset32_t);
static void print_eflags(void *);

static size_t conf_x86_cpustat_callstack_size = 10;
static const char *conf_x86_cpustat_register_color = "\033[1;34m";
static const char *conf_x86_cpustat_code_color = "\033[1;31m";
static const char *conf_x86_cpustat_instruction_color = "\033[1;90m";
static const char *conf_x86_cpustat_data_color = "\033[1;35m";
static const char *conf_x86_cpustat_stack_color = "\033[1;33m";
static const char *conf_x86_cpustat_arrow_right = "\033[0m\033[1m ─▶ \033[1m";
static const char *conf_x86_cpustat_arrow_left = "\033[0m\033[1m ◀─ \033[1m";
static size_t conf_x86_cpustat_maxptrs = 4;
static size_t conf_x86_cpustat_stack_entries = 7;


static _Bool ptr_has_string(void *cpu, moffset32_t vaddr)
{
    // TODO: internationalization! change this to check for utf-8
    // should be enough to determine if it points to a string
    // chances are pretty low that four random bytes will happen
    // to be all ascii
    for (size_t i = 0; i < sizeof(moffset32_t); i++) {
        if (!isprint(x86_rdmem8(cpu, vaddr+i)))
            return 0;
    }

    return 1;
}

static void print_register(void *cpu, uint8_t reg)
{
    reg32_t value = x86_rdreg32(cpu, reg);
    char *content;

    // the register EIP holds the address to the next instruction
    // to get the address of the current one we query the cpu
    if (reg == EIP)
        value = x86_cpustat_query(cpu, STAT_EIP);

    content = getptrcontent(cpu, value);

    s_info("%s%s:\033[0m %s \033[0m", conf_x86_cpustat_register_color, stringfyregister(reg, 32), content);

    xfree(content);
}

static char *getptrcontent(void *cpu, moffset32_t vaddr)
{
    static char *s = NULL;
    moffset32_t ptr = vaddr;
    char *ptrstr = NULL;
    size_t nptrs = 1;
    const char *data_color = conf_x86_cpustat_data_color;
    const char *stack_color = conf_x86_cpustat_stack_color;
    const char *arrow_r = conf_x86_cpustat_arrow_right;
    const char *arrow_l = conf_x86_cpustat_arrow_left;

    s = xstrdup("");
    for (;;ptr = x86_rdmem32(cpu, ptr)) {

        // try and see if there is a string
        if (mmu_isdataptr(x86_mmu(cpu), ptr) && ptr_has_string(cpu, ptr)) {
            const char *str = (const char *)mmu_getptr(x86_mmu(cpu), ptr);
            ptrstr = int2hexstr(ptr, 8);
            char *temp = s;

            if (mmu_isstackptr(x86_mmu(cpu), ptr))
                s = strcatall(7, s, stack_color, ptrstr, arrow_l, "'\033[0m", str, "'");
            else
                s = strcatall(7, s, data_color, ptrstr, arrow_l, "'\033[0m", str, "'");

            xfree(temp);
            xfree(ptrstr);
            break;
        }

        if (mmu_isstackptr(x86_mmu(cpu), ptr)) {
            if (nptrs == conf_x86_cpustat_maxptrs) {
                s = strcatall(4, s, arrow_r, "...", "\033[0m");
                break;
            }

            char *temp = s;
            ptrstr = int2hexstr(ptr, 8);

            s = strcatall(5, s, stack_color, ptrstr, arrow_r, "\033[0m");

            xfree(temp);
            xfree(ptrstr);
            nptrs++;
        } else if (mmu_isdataptr(x86_mmu(cpu), ptr)) {
            if (nptrs == conf_x86_cpustat_maxptrs) {
                s = strcatall(4, s, arrow_r, "...", "\033[0m");
                break;
            }
            char *temp = s;
            ptrstr = int2hexstr(ptr, 8);
            struct symbol_lookup_record symbol = sr_lookup(x86_resolver(cpu), ptr);
            s = strcatall(8, s, data_color, ptrstr, " <", symbol.sl_name, ">", arrow_r, "\033[0m");

            xfree(temp);
            xfree(ptrstr);
            nptrs++;
        } else if (mmu_iscodeptr(x86_mmu(cpu), ptr)) {
            char *ptrstr = disassembleptr(cpu, ptr);
            char *temp = s;

            s = strcatall(3, s, ptrstr, "\033[0m");

            xfree(temp);
            xfree(ptrstr);
            break;
        } else {
            // just a plain value
            ptrstr = int2hexstr(ptr, 0);
            char *temp = s;

            s = strcatall(3, s, "\033[0m", ptrstr);

            xfree(temp);
            xfree(ptrstr);
            break;
        }

    }

    return s;
}

static char *add_branch_target_to_instruction(moffset32_t target, struct symbol_lookup_record func, char *instruction)
{
    char *relative = NULL;
    char *s;

    if (target != func.sl_start) {
        relative = int2hexstr(target - func.sl_start, 2);
        s = strcatall(6, instruction, " <", func.sl_name, "+", relative, ">");
    } else {
        s = strcatall(4, instruction, " <", func.sl_name, ">");
    }

    xfree(relative);
    return s;
}

static char *disassembleptr(void *cpu, moffset32_t vaddr)
{
    struct symbol_lookup_record func = sr_lookup(x86_resolver(cpu), vaddr);
    struct instruction ins;
    char *s = NULL;
    char *functrel = NULL;
    char *addrstr;
    char *disasstr;
    char *opcodestr;
    const char *arrow_l = conf_x86_cpustat_arrow_left;
    const char *inscolor = conf_x86_cpustat_instruction_color;
    const char *codecolor = conf_x86_cpustat_code_color;

    // it will create a string in this format:  0xdeadbeef <main+0x123> ◀- 0x5f pop EDI
    if (func.sl_start) {

        addrstr = int2hexstr(vaddr, 8);

        // do we add the offset?
        if (vaddr - func.sl_start)
            functrel = int2hexstr(vaddr - func.sl_start, 0);

        // get the instruction
        ins = x86_decode(x86_mmu(cpu), vaddr);
        opcodestr = int2hexstr(ins.data.opc, 2);

        // disassemble the instruction
        disasstr = x86_disassemble(ins);

        // add the name of the called function
        if (ins.handler == x86_mm_call || ins.handler == x86_mm_jcc) {
            moffset32_t calladdress = x86_findbranchtarget(cpu, ins.data);

            char *temp = add_branch_target_to_instruction(calladdress, sr_lookup(x86_resolver(cpu), calladdress), disasstr);
            xfree(disasstr);
            disasstr = temp;
        }

        if (functrel)
            s = strcatall(13, codecolor, addrstr, " <", func.sl_name, "+", functrel, ">", arrow_l, inscolor, opcodestr, " ", disasstr, "\033[0m");
        else
            s = strcatall(11, codecolor, addrstr, " <", func.sl_name, ">", arrow_l, inscolor, opcodestr, " ", disasstr, "\033[0m");

        xfree(functrel);
        xfree(addrstr);
        xfree(disasstr);
        xfree(opcodestr);
    }

        return s;
}

static void print_eflags(void *cpu)
{
    s_info("%sEFLAGS:\033[0m [ %s %s %s %s %s %s %s %s %s ]\n", conf_x86_cpustat_register_color,
            x86_flag_on(cpu, OF) ? "\033[1;32mOF\033[0m" : "\033[1;90mOF\033[0m",
            x86_flag_on(cpu, DF) ? "\033[1;32mDF\033[0m" : "\033[1;90mDF\033[0m",
            x86_flag_on(cpu, IF) ? "\033[1;32mIF\033[0m" : "\033[1;90mIF\033[0m",
            x86_flag_on(cpu, TF) ? "\033[1;32mTF\033[0m" : "\033[1;90mTF\033[0m",
            x86_flag_on(cpu, SF) ? "\033[1;32mSF\033[0m" : "\033[1;90mSF\033[0m",
            x86_flag_on(cpu, ZF) ? "\033[1;32mZF\033[0m" : "\033[1;90mZF\033[0m",
            x86_flag_on(cpu, AF) ? "\033[1;32mAF\033[0m" : "\033[1;90mAF\033[0m",
            x86_flag_on(cpu, PF) ? "\033[1;32mPF\033[0m" : "\033[1;90mPF\033[0m",
            x86_flag_on(cpu, CF) ? "\033[1;32mCF\033[0m" : "\033[1;90mCF\033[0m"
            );
}

void x86_cpustat_print(void *cpu)
{
    ASSERT(cpu != NULL);
    char *stack;

    s_info("[   ----------------------------- x86CPU -----------------------------   ]");

    print_register(cpu, EAX);
    print_register(cpu, ECX);
    print_register(cpu, EDX);
    print_register(cpu, EBX);
    print_register(cpu, ESI);
    print_register(cpu, EDI);
    print_register(cpu, EBP);
    print_register(cpu, ESP);
    print_register(cpu, EIP);
    print_eflags(cpu);

    // to print the name of the registers that has a stack value loaded, we start by
    // getting the padding by determining the max amount of registers that has the same value
    // we can then pad using spaces in the entries that don't have any registers
    int padding = 0;
    for (size_t i = 0, k = 0; i < conf_x86_cpustat_stack_entries; i++, k += 4) {
        int registers = 0;
        for (size_t i = 0; i < 7; i++) {
            if (x86_rdreg32(cpu,i) == x86_rdreg32(cpu, ESP) + k)
                registers++;
        }

        if (registers > padding)
            padding = registers;
    }


    s_info("[STACK]");
    for (size_t i = 0, k = 0; i < conf_x86_cpustat_stack_entries; i++, k += 4) {
        stack = getptrcontent(cpu, x86_rdreg32(cpu, ESP) + k);
        char *registers = NULL;
        int registerssz = 0;
        int temp_pad;

        // check if there's any register with this stack value
        for (size_t i = 0; i < 7; i++) {
            if (x86_rdreg32(cpu, i) == x86_rdreg32(cpu, ESP) + k) {
                // 3 = register name, 2 = space + null byte
                registers = xreallocarray(registers, registerssz + 5, sizeof(registers));
                const char *temp = stringfyregister(i, 32);

                while (*temp)
                    registers[registerssz++] = tolower(*temp++);
                registers[registerssz++] = ' ';
                registers[registerssz] = 0;
            }
        }
        // entries that have a registers don't need as many padding spaces
        temp_pad = (padding - (registerssz / 4)) * 4;

        s_info("%02lx:%04lx| %*s%s\033[0m%s", i, k, temp_pad, registers ? "\033[1m" : "    ", registers ? registers : "",  stack);
        if (registers)
            xfree(registers);

        xfree(stack);
    }

    s_info("\n[CALLTRACE]");
    const callstack_record_t *record;
    for (int i = x86_cpustat_query(cpu, STAT_CALLSTACKTOP); i >= 0; i--) {
        record = x86_cpustat_callstack_i(cpu, i);
        char *s = disassembleptr(cpu, record->f_val + record->f_rel);
        s_info("%s", s);
        xfree(s);
    }

    s_info(" ");
}


///////////////////

inline _Bool x86_cpustat_queryfailed(void *cpu)
{
    ASSERT(cpu != NULL);
    return x86_cpustat(cpu)->queryfailed;
}

inline uint64_t x86_cpustat_query(void *cpu, int item)
{
    ASSERT(cpu != NULL);
    x86_cpustat(cpu)->queryfailed = 0;

    switch (item) {
        case STAT_EIP:
            return (uint64_t) x86_cpustat(cpu)->eip;
        case STAT_STACKTOP:
            return (uint64_t) x86_cpustat(cpu)->stacktop;
        case STAT_CALLSTACKSZ:
            return (uint64_t) x86_cpustat(cpu)->callstacksz;
        case STAT_CALLSTACKTOP:
            return (uint64_t) x86_cpustat(cpu)->callstacktop;
        default:
            x86_cpustat(cpu)->queryfailed = 1;
    }

    return 0;
}

inline void x86_cpustat_set(void *cpu, int item, uint64_t value)
{
    ASSERT(cpu != NULL);

    switch (item) {
        case STAT_EIP:
            x86_cpustat(cpu)->eip = (reg32_t)value;
            break;
        case STAT_STACKTOP:
            x86_cpustat(cpu)->stacktop = (moffset32_t) value;
            break;
        case STAT_CALLSTACKSZ:
            x86_cpustat(cpu)->callstacksz = (size_t) value;
            break;
        case STAT_CALLSTACKTOP:
            x86_cpustat(cpu)->callstacktop = (size_t) value;
            break;
    }
}

inline const callstack_record_t *x86_cpustat_callstack_i(void *cpu, uint8_t idx)
{
    ASSERT(cpu != NULL);

    if (idx > x86_cpustat(cpu)->callstacktop)
        return NULL;

    return &x86_cpustat(cpu)->callstack[idx];
}

inline const callstack_record_t *x86_cpustat_callstack(void *cpu)
{
    ASSERT(cpu != NULL);

    return &x86_cpustat(cpu)->callstack[x86_cpustat(cpu)->callstacktop];
}
inline const struct instruction *c_86_cpustat_current_op(void *cpu)
{
    ASSERT(cpu != NULL);

    return &x86_cpustat(cpu)->instr;
}

void x86_cpustat_init(void *cpu)
{
    x86_cpustat(cpu)->callstack = xcalloc(conf_x86_cpustat_callstack_size, sizeof(callstack_record_t));
    x86_cpustat(cpu)->callstacksz = conf_x86_cpustat_callstack_size;
}

void x86_cpustat_push_callstack(void *cpu, moffset32_t faddr)
{
    ASSERT(cpu != NULL);
    size_t i = x86_cpustat(cpu)->callstacktop;
    callstack_record_t *prev = &x86_cpustat(cpu)->callstack[i];
    callstack_record_t *new;
    struct symbol_lookup_record func;
    static _Bool first_call = 1;

    // we reached the top
    if (i >= conf_x86_cpustat_callstack_size)
        return;

    // set the previous record value to where it will begin once returned
    prev->f_rel = x86_rdreg32(cpu, EIP) - prev->f_val;

    func = sr_lookup(x86_resolver(cpu), faddr);
    new = &x86_cpustat(cpu)->callstack[i+1];

    new->f_rel = 0;
    new->f_val = func.sl_start;
    new->f_sym = func.sl_name;

    if (!first_call)
        x86_cpustat(cpu)->callstacktop += 1;

    if (first_call)
        first_call = 0;

}

inline void x86_cpustat_pop_callstack(void *cpu)
{
    if (x86_cpustat_query(cpu, STAT_CALLSTACKTOP) == 0)
        return;
    x86_cpustat(cpu)->callstacktop -= 1;
}

inline void x86_cpustat_update_callstack(void *cpu)
{
    size_t i = x86_cpustat(cpu)->callstacktop;
    callstack_record_t *record = &x86_cpustat(cpu)->callstack[i];

    record->f_rel = x86_rdreg32(cpu, EIP) - record->f_val - x86_cpustat(cpu)->instr.size;
}

inline void x86_cpustat_set_current_op(void *cpu, struct instruction op)
{
    x86_cpustat(cpu)->instr = op;
}
