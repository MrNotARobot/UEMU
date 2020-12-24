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

#include <ctype.h>
#include <string.h>

#include "../memory.h"
#include "../system.h"
#include "../string-utils.h"
#include "../tracer.h"

#include "dbg.h"
#include "disassembler.h"

static _Bool branch_is_taken(struct exec_data, struct EFlags, reg32_t);

static char *disassembleptr(x86CPU *, moffset32_t);
static _Bool ptr_has_string(x86CPU *, moffset32_t);
static char *getptrcontent(x86CPU *, moffset32_t);

static void print_register(x86CPU *, const char *, reg32_t);
static void print_eflags(traced_registers_t);

static const char *conf_x86dbg_separator_colorcode = "\033[34m";
static const char *conf_x86dbg_code_colorcode = "\033[31m";
static const char *conf_x86dbg_data_colorcode = "\033[35m";
static const char *conf_x86dbg_heap_colorcode = "\033[34m";
static const char *conf_x86dbg_stack_colorcode = "\033[33m";
static const char *conf_x86dbg_arrow_right = "\033[0m ─▶ ";
static const char *conf_x86dbg_arrow_left = "\033[0m ◀─ ";
static uint8_t conf_x86dbg_max_stack_entries = 8;
static uint8_t conf_x86dbg_disassemble_entries = 11;

static moffset32_t p_start_disassemble_here = 0;

static _Bool branch_is_taken(struct exec_data data, struct EFlags eflags, reg32_t ecx)
{
    switch (data.opc) {
        case 0x87:  // JA rel32     rel16
        case 0x77:  // JA rel8
            return (!eflags.CF) && !eflags.ZF;
            break;
        case 0x83:  // JAE rel32    rel16
        case 0x73:  // JAE rel8
            return !eflags.CF;
            break;
        case 0x82:  // JB rel32     rel16
        case 0x72:  // JB rel8
            return eflags.CF;
            break;
        case 0x86:  // JBE rel32    rel16
        case 0x76:  // JBE rel8
            return eflags.CF || eflags.ZF;
            break;
        case 0xE3:  // JCXZ rel8
            if (data.adrsz_pfx)
                return !(ecx & 0x0000FFFF);
            else
                return !ecx;
            break;
        case 0x84:  // JE rel32     rel16
        case 0x74:  // JE rel8
            return eflags.ZF;
            break;
        case 0x8F:  // JG rel32     rel16
        case 0x7F:  // JG rel7
            return !eflags.ZF && !eflags.SF;
            break;
        case 0x8D:  // JGE rel32    rel16
        case 0x7D:  // JGE rel8
            return !eflags.SF;
            break;
        case 0x8C:  // JL rel32     rel16
        case 0x7C:  // JL rel8
            return (eflags.SF && !eflags.OF) || (!eflags.SF && eflags.OF);
            break;
        case 0x8E:  // JLE rel32     rel16
        case 0x7E:  // JLE rel8
            return eflags.ZF || (eflags.SF && !eflags.OF) || (!eflags.SF && eflags.OF);
            break;
        case 0x85:  // JNE rel32     rel16
        case 0x75:  // JNE rel8
            return !eflags.ZF;
            break;
        case 0x81:  // JNO rel32     rel16
        case 0x71:  // JNO rel8
            return !eflags.OF;
            break;
        case 0x8B:  // JNP rel32     rel16
        case 0x7B:  // JNP rel8
            return !eflags.PF;
            break;
        case 0x89:  // JNS rel32     rel16
        case 0x79:  // JNS rel8
            return !eflags.SF;
            break;
        case 0x80:  // JO rel32     rel16
        case 0x70:  // JO rel8
            return eflags.OF;
            break;
        case 0x8A:  // JP rel32     rel16
        case 0x7A:  // JPE rel8
            return eflags.PF;
            break;
        case 0x88:  // JS rel32     rel16
        case 0x78:  // JS rel8
            return eflags.SF;
            break;
    }

    return 0;
}


static _Bool ptr_has_string(x86CPU *cpu, moffset32_t vaddr)
{
    // TODO: internationalization! change this to check for utf-8
    // should be enough to determine if it points to a string
    // chances are pretty low that four random bytes will happen
    // to be all ascii
    for (size_t i = 0; i < sizeof(moffset32_t); i++) {
        if (!isprint(x86_try_readM8(cpu, vaddr+i)))
            return 0;
    }

    return 1;
}

static char *disassembleptr(x86CPU *cpu, moffset32_t vaddr)
{
    struct symbol_lookup_record func = sr_lookup(x86_resolver(cpu), vaddr);
    struct instruction ins;
    char *s = NULL;
    char *functrel = NULL;
    char *addrstr;
    char *disasstr;
    const char *arrow_l = conf_x86dbg_arrow_left;
    const char *codecolor = conf_x86dbg_code_colorcode;

    // it will create a string in this format:  0xdeadbeef <main+123> ◀- pop EDI

    addrstr = int2hexstr(vaddr, 8);

    // do we add the offset?
    if (vaddr - func.sl_start)
        functrel = int2str(vaddr - func.sl_start);

    // get the instruction
    ins = x86_decode(cpu, vaddr);

    // disassemble the instruction
    disasstr = x86_disassemble(cpu, ins);

    if (!func.sl_name) {
        s = strcatall(6, codecolor, addrstr, arrow_l, " ", disasstr, "\033[0m");
    } else if (functrel) {
        s = strcatall(11, codecolor, addrstr, " <", func.sl_name, "+", functrel, ">", arrow_l, " ", disasstr, "\033[0m");
    } else {
        s = strcatall(9, codecolor, addrstr, " <", func.sl_name, ">", arrow_l, " ", disasstr, "\033[0m");
    }


    xfree(functrel);
    xfree(addrstr);
    xfree(disasstr);

        return s;
}


static char *getptrcontent(x86CPU *cpu, moffset32_t vaddr)
{
    moffset32_t ptr = vaddr;
    static char *s = NULL;
    char *ptrstr = NULL;
    const char *data_color = conf_x86dbg_data_colorcode;
    const char *stack_color = conf_x86dbg_stack_colorcode;
    const char *arrow_r = conf_x86dbg_arrow_right;
    const char *arrow_l = conf_x86dbg_arrow_left;
    int ptrtype;

    s = xstrdup("");

    for (;;ptr = x86_try_readM32(cpu, ptr)) {
        ptrtype = x86_ptrtype(cpu, ptr);

        // try and see if there is a string
        if ((ptrtype == STACK_PTR || ptrtype == DATA_PTR) && ptr_has_string(cpu, ptr)) {
            const char *str = (const char *)x86_getptr(cpu, ptr);
            ptrstr = int2hexstr(ptr, 8);
            char *temp = s;

            if (ptr == vaddr)
                arrow_r = "";

            if (ptrtype == STACK_PTR)
                s = strcatall(8, s, arrow_r, stack_color, ptrstr, arrow_l, "'\033[0m", str, "'");
            else
                s = strcatall(8, s, arrow_r, data_color, ptrstr, arrow_l, "'\033[0m", str, "'");

            xfree(temp);
            xfree(ptrstr);
            break;
        }

        if (ptrtype == STACK_PTR) {
            char *temp = s;
            ptrstr = int2hexstr(ptr, 8);

            // first ptr?
            if (ptr == vaddr)
                s = strcatall(4, s, stack_color, ptrstr, "\033[0m");
            else
                s = strcatall(5, s, arrow_r, stack_color, ptrstr, "\033[0m");

            xfree(temp);
            xfree(ptrstr);
        } else if (ptrtype == DATA_PTR) {
            char *temp = s;
            ptrstr = int2hexstr(ptr, 8);
            struct symbol_lookup_record symbol = sr_lookup(x86_resolver(cpu), ptr);

            if (!symbol.sl_name)
                s = strcatall(4, s, data_color, ptrstr, "\033[0m");
            else if (ptr == vaddr)
                s = strcatall(7, s, data_color, ptrstr, " <", symbol.sl_name, ">", "\033[0m");
            else
                s = strcatall(7, s, arrow_r, data_color, ptrstr, " <", symbol.sl_name, ">", "\033[0m");


            xfree(temp);
            xfree(ptrstr);
        } else if (ptrtype == CODE_PTR) {
            char *ptrstr = disassembleptr(cpu, ptr);
            char *temp = s;

            if (ptr == vaddr)
                arrow_l = "";

            s = strcatall(4, s, arrow_l, ptrstr, "\033[0m");

            xfree(temp);
            xfree(ptrstr);
                break;
            } else {
            // just a plain value
            ptrstr = int2hexstr(ptr, 0);
            char *temp = s;

            if (ptr == vaddr)
                s = strcatall(3, s, "\033[0m", ptrstr);
            else
                s = strcatall(4, s, "\033[0m", arrow_l, ptrstr);

            xfree(temp);
            xfree(ptrstr);
            break;
        }

    }

    return s;
}


static void print_register(x86CPU *cpu, const char *name, reg32_t value)
{
    char *content = getptrcontent(cpu, value);

    s_info("%s  %s", name, content);

    xfree(content);
}


static void print_eflags(traced_registers_t registers)
{
    s_info("\033[1mEFLAGS  \033[0m[ %s %s %s %s %s %s %s %s %s ]",
            registers.eflags.OF ? "\033[1;32mOF\033[0m" : "\033[1;90mOF\033[0m",
            registers.eflags.DF ? "\033[1;32mDF\033[0m" : "\033[1;90mDF\033[0m",
            registers.eflags.IF ? "\033[1;32mIF\033[0m" : "\033[1;90mIF\033[0m",
            registers.eflags.TF ? "\033[1;32mTF\033[0m" : "\033[1;90mTF\033[0m",
            registers.eflags.SF ? "\033[1;32mSF\033[0m" : "\033[1;90mSF\033[0m",
            registers.eflags.ZF ? "\033[1;32mZF\033[0m" : "\033[1;90mZF\033[0m",
            registers.eflags.AF ? "\033[1;32mAF\033[0m" : "\033[1;90mAF\033[0m",
            registers.eflags.PF ? "\033[1;32mPF\033[0m" : "\033[1;90mPF\033[0m",
            registers.eflags.CF ? "\033[1;32mCF\033[0m" : "\033[1;90mCF\033[0m"
            );
}


//  Yes, I copied pwndbg...
//  Its just such a good debugging environment that I couldn't help it
void x86dbg_print_state(x86CPU *cpu)
{

    traced_registers_t registers;
    backtrace_record_t backtrace;
    uint32_t backtrace_size;
    uint8_t registers_on_stack_padding;
    static uint8_t last_backtrace = 0;

    if (!cpu)
        return;

    registers = tracer_get_savedstate(x86_tracer(cpu));
    // get the current function
    backtrace_size = tracer_get_backtrace_size(x86_tracer(cpu));
    backtrace = tracer_get_backtrace(x86_tracer(cpu), backtrace_size);

    // resets the address where we start disassembling
    if (backtrace_size != last_backtrace || !p_start_disassemble_here) {
        p_start_disassemble_here = backtrace.st_start + backtrace.st_rel;
        last_backtrace = backtrace_size;
    }


    s_info("\033[34m%08x\033[0m in \033[33m%s\033[0m ()", registers.eip, backtrace.st_symbol);
    s_info("LEGEND: %sSTACK\033[0m | %sHEAP\033[0m | %sCODE\033[0m | %sDATA\033[0m | RODATA",
            conf_x86dbg_stack_colorcode, conf_x86dbg_heap_colorcode, conf_x86dbg_code_colorcode, conf_x86dbg_data_colorcode);

    s_info("%s───[x86]─────────────────────────────────────────────────[ REGISTERS ]──────────────────────────────────────────────────\033[0m", conf_x86dbg_separator_colorcode);

    print_register(cpu, "\033[1mEAX\033[0m", registers.eax);
    print_register(cpu, "\033[1mEBX\033[0m", registers.ebx);
    print_register(cpu, "\033[1mECX\033[0m", registers.ecx);
    print_register(cpu, "\033[1mEDX\033[0m", registers.edx);
    print_register(cpu, "\033[1mEDI\033[0m", registers.edi);
    print_register(cpu, "\033[1mESI\033[0m", registers.esi);
    print_register(cpu, "\033[1mEBP\033[0m", registers.ebp);
    print_register(cpu, "\033[1mESP\033[0m", registers.esp);
    print_register(cpu, "\033[1mEIP\033[0m", registers.eip);
    print_eflags(registers);

    s_info("%s───[x86]──────────────────────────────────────────────────[ DISASSM ]───────────────────────────────────────────────────\033[0m", conf_x86dbg_separator_colorcode);

    uint32_t first_instruction_size;
    uint32_t first_symbol_size = 0;
    for (uint32_t i = 0, eip = p_start_disassemble_here; i < conf_x86dbg_disassemble_entries; i++) {
        struct instruction ins = x86_decode(cpu, eip);
        char *disassembled;
        uint32_t padding = first_symbol_size + 4;
        char *symbolname = NULL;
        char *temp = NULL;

        if (ins.fail_to_fetch) {
            s_info("\033[1;91m  ✗\033[0m");
            break;
        }

        if (eip == backtrace.st_end)
            break;

        if (i == 0)
            first_instruction_size = ins.size;

        disassembled = x86_disassemble(cpu, ins);


        if (eip == backtrace.st_start && eip == registers.eip) {
            symbolname = strcatall(3, "<", backtrace.st_symbol, ">");

            if (!first_symbol_size) {
                first_symbol_size = strlen(symbolname);
                padding = first_symbol_size + 4;
            } 

            if (padding < strlen(symbolname))
                padding = strlen(symbolname) + 1;

            s_info(" \033[1;32m◆ 0x%08x %-*s\033[0m\033[1m%s\033[0m", eip, padding, symbolname, disassembled);
        } else if (eip == backtrace.st_start) {
            symbolname = strcatall(3, "<", backtrace.st_symbol, ">");

            if (!first_symbol_size) {
                first_symbol_size = strlen(symbolname);
                padding = first_symbol_size + 4;
            } 

            if (padding < strlen(symbolname))
                padding = strlen(symbolname) + 1;

            s_info("   0x%08x %-*s%s", eip, first_symbol_size + 4, symbolname, disassembled);
        } else if (eip == registers.eip) {
            temp = int2str(eip - backtrace.st_start);
            symbolname = strcatall(5, "<", backtrace.st_symbol, "+", temp,  ">");

            if (!first_symbol_size) {
                first_symbol_size = strlen(symbolname);
                padding = first_symbol_size + 4;
            } 

            if (padding < strlen(symbolname))
                padding = strlen(symbolname) + 1;

            s_info(" \033[1;32m◆ 0x%08x %-*s\033[0m\033[1m%s\033[0m", eip, padding, symbolname, disassembled);
            if (i == 5)
                p_start_disassemble_here += first_instruction_size;

            if (ins.handler == x86_mm_call) {
                char *arg = getptrcontent(cpu, registers.esp);
                s_info("    \033[1marg[0]: %s", arg);
                xfree(arg);
                arg = getptrcontent(cpu, registers.esp + 4);
                s_info("    \033[1marg[1]: %s", arg);
                xfree(arg);
                arg = getptrcontent(cpu, registers.esp + 8);
                s_info("    \033[1marg[2]: %s", arg);
                xfree(arg);
                arg = getptrcontent(cpu, registers.esp + 12);
                s_info("    \033[1marg[3]: %s", arg);
                xfree(arg);
            }
        } else {
            temp = int2str(eip - backtrace.st_start);
            symbolname = strcatall(5, "<", backtrace.st_symbol, "+", temp,  ">");

            if (!first_symbol_size) {
                first_symbol_size = strlen(symbolname);
                padding = first_symbol_size + 4;
            } 

            if (padding < strlen(symbolname))
                padding = strlen(symbolname) + 1;

            s_info("   0x%08x %-*s%s", eip, padding, symbolname, disassembled);

        }

        xfree(temp);
        xfree(symbolname);
        xfree(disassembled);
        eip += ins.size;

        if ((eip - ins.size) == registers.eip && ins.handler == x86_mm_jcc) {
            if (branch_is_taken(ins.data, registers.eflags, registers.ecx)) {
                eip = x86_findbranchtarget_relative(cpu, ins.eip, ins.data);
                p_start_disassemble_here = eip;
                s_info("        \033[1m↓\033[0m");
            }
        } else if (ins.handler == x86_mm_call || ins.handler == x86_mm_jcc) {
            s_info(" ");
        } else if (ins.handler == x86_mm_ret && backtrace_size) {
            s_info("        \033[1m↓\033[0m");
            backtrace = tracer_get_backtrace(x86_tracer(cpu), backtrace_size-1);
            eip = backtrace.st_start + backtrace.st_rel;
        }
    }

    s_info("%s───[x86]───────────────────────────────────────────────────[ STACK ]────────────────────────────────────────────────────\033[0m", conf_x86dbg_separator_colorcode);

    // discover the padding so that we can print the name of the register besides its value
    // with all others values still aligning
    // like this:
    // 00:0000|  esp  0xffffffff ─▶ 0x0
    // 01:0004|       0xffffffff ─▶ 0x0
    // ...
    // its not a elegant way of doing it but its pratical, so I did it like this.
    registers_on_stack_padding = 1;
    for (uint8_t i = 0, k = 0; i < conf_x86dbg_max_stack_entries; i++, k += sizeof(registers.esp)) {
        uint8_t current_padding = 0;
        if (k == 0)
                current_padding++;
        if (registers.eax == registers.esp + k)
                current_padding++;
        if (registers.ebx == registers.esp + k)
                current_padding++;
        if (registers.ecx == registers.esp + k)
                current_padding++;
        if (registers.edx == registers.esp + k)
                current_padding++;
        if (registers.edi == registers.esp + k)
                current_padding++;
        if (registers.esi == registers.esp + k)
                current_padding++;

        if (current_padding > registers_on_stack_padding)
            registers_on_stack_padding = current_padding;
    }

    for (uint8_t i = 0, k = 0; i < conf_x86dbg_max_stack_entries; i++, k += sizeof(registers.esp)) {
        char *stack = getptrcontent(cpu, registers.esp + k);
        char *registers_names = xstrdup("");
        uint8_t nregisters = 0;
        uint8_t padding = 0;

        // check if there's any register with this stack value
        // more stupid coding ahead...
        if (i == 0) {
                char *temp = registers_names;
                nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(ESP, 32));
                xfree(temp);
        } if (registers.eax == registers.esp + k) {
                char *temp = registers_names;
                nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(EAX, 32));
                xfree(temp);
        } if(registers.ebx == registers.esp + k) {
                char *temp = registers_names;
                 nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(EBX, 32));
                xfree(temp);
        } if (registers.ecx == registers.esp + k) {
                char *temp = registers_names;
                 nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(ECX, 32));
                xfree(temp);
        } if (registers.edx == registers.esp + k) {
                char *temp = registers_names;
                 nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(EDX, 32));
                xfree(temp);
        } if (registers.edi == registers.esp + k) {
                char *temp = registers_names;
                 nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(EDI, 32));
                xfree(temp);
        } if (registers.esi == registers.esp + k) {
                char *temp = registers_names;
                nregisters++;
                registers_names = strcatall(3, registers_names, " ", stringfyregister(ESI, 32));
                xfree(temp);
        }

        for (uint8_t i = 0; i < strlen(registers_names); i++)
            registers_names[i] = tolower(registers_names[i]);

        // entries that have a registers don't need as many padding spaces
        padding = (registers_on_stack_padding - nregisters) * 4;

        s_info("%02x:%04x|%*s\033[1m%s\033[0m  %s", i, k, padding, "", registers_names, stack);

        xfree(registers_names);
        xfree(stack);
    }



    s_info("%s───[x86]─────────────────────────────────────────────────[ BACKTRACE ]──────────────────────────────────────────────────\033[0m", conf_x86dbg_separator_colorcode);

    backtrace = tracer_get_backtrace(x86_tracer(cpu), backtrace_size);
    if (backtrace.st_rel)
        s_info(" ◆  0   %08x %s+%d", backtrace.st_start + backtrace.st_rel, backtrace.st_symbol, backtrace.st_rel);
    else
        s_info(" ◆  0   %08x %s", backtrace.st_start, backtrace.st_symbol);

    for (int i = backtrace_size - 1, k = 1; i >= 0; i--, k++) {
        backtrace = tracer_get_backtrace(x86_tracer(cpu), i);
        s_info("    %d   %08x %s+%d", k, backtrace.st_start + backtrace.st_rel, backtrace.st_symbol, backtrace.st_rel);
    }
    s_info("%s───[x86]────────────────────────────────────────────────────────────────────────────────────────────────────────────────\033[0m", conf_x86dbg_separator_colorcode);
}
