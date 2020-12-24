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
 * Disassemble the instruction for pretty printing.
 */

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "../memory.h"
#include "../system.h"
#include "../string-utils.h"

#include "disassembler.h"

#include "x86-utils.h"

static char *modrm2str(uint8_t, uint8_t, uint32_t, int);

static const char *conf_disassm_mnemonic_colorcode = "\033[38;5;148m";
static const char *conf_disassm_operand_colorcode = "\033[38;5;81m";
static const char *conf_disassm_number_colorcode = "\033[38;5;141m";
static const char *conf_disassm_symbol_colorcode = "\033[38;5;141m";
static const char *conf_disassm_code_address_colorcode = "\033[31m";
static const char *conf_disassm_data_address_colorcode = "\033[35m";
//static const char *conf_disassm_symbols_colorcode = "\033[95m";


static char *modrm2str(uint8_t modrm, uint8_t sib, uint32_t imm, int oprnd_size)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    uint8_t ss_factor = sibss(sib);
    uint8_t index = sibindex(sib);
    uint8_t base = sibbase(sib);
    char *regstr = NULL;
    char *scaleregister = NULL;
    char *scaledindex;
    const char *scale = "";
    char *s = NULL;
    const char *operand_color = conf_disassm_operand_colorcode;
    const char *num_color = conf_disassm_number_colorcode;

    if (mod == 0) {
        if (rm != 0b100) {
            if (rm == 0b101) {
                char *immediate = int2hexstr(imm, 0);
                s = strcatall(4, "[", num_color, immediate, "\033[0m]");
                xfree(immediate);
                return s;
            }

            return strcatall(4, "[", operand_color, stringfyregister(rm, 32), "\033[0m]");
        }
    } else if (mod == 1 || mod == 2) {

        if (rm != 0b100) {
            char *immediate = int2hexstr(imm, 0);
            s = strcatall(7, "[", operand_color, stringfyregister(rm, 32), "\033[0m + ", num_color, immediate, "\033[0m]");
            xfree(immediate);
            return s;
        }
    } else {
            if (oprnd_size == 8)
                return strcatall(3, operand_color, stringfyregister(reg32to8(rm), oprnd_size), "\033[0m");
        return strcatall(3, operand_color, stringfyregister(rm, oprnd_size), "\033[0m");
    }

    switch (base) {
        case 0b101:
            if (mod == 0)
                regstr = int2hexstr(imm, 8);
            else
                regstr = xstrdup("ebp");
            break;
        case 0b000:
        case 0b001:
        case 0b010:
        case 0b011:
        case 0b100:
        case 0b110:
        case 0b111:
            regstr = xstrdup(stringfyregister(base, 32));
            break;
    }

    switch (index) {
        case 0b000: scaleregister = "eax"; break;
        case 0b001: scaleregister = "ecx"; break;
        case 0b010: scaleregister = "edx"; break;
        case 0b011: scaleregister = "ebx"; break;
        case 0b100: scaleregister = ""; break;
        case 0b101: scaleregister = "ebp"; break;
        case 0b110: scaleregister = "esi"; break;
        case 0b111: scaleregister = "edi"; break;
    }

    if (ss_factor == 0b01)
        scale = "2";
    else if (ss_factor == 0b10)
        scale = "4";
    else if (ss_factor == 0b10)
        scale = "8";

    if (index == 0b100)
        scaledindex = xstrdup("");
    else
        scaledindex = strcatall(6, operand_color, scaleregister, "\033[0m*", num_color, scale, "\033[0m + "); // reg*scale


    if (mod == 1 || mod == 2) {
        char *displacement = int2hexstr(imm, 0);

        s = strcatall(8, "[", scaledindex, operand_color, regstr, "\033[0m + ", num_color,  displacement, "\033[0m]");

        xfree(displacement);
    } else {

        s = strcatall(5, "[", scaledindex, operand_color, regstr, "\033[0m]");

    }

    xfree(scaledindex);
    xfree(regstr);

    return s;
}

char *x86_disassemble(x86CPU *cpu, struct instruction ins)
{
    char *mnemonic = NULL;
    char *arg = NULL;
    char *immediate = NULL;
    char *temp = NULL;
    char *s = NULL;
    uint8_t mnemonic_spacing = 8;
    size_t mnemonic_size;

    if (ins.encoding == no_encoding)
        return NULL;

    mnemonic_size = strlen(ins.name);
    if (mnemonic_size >= mnemonic_spacing)
        mnemonic_spacing = mnemonic_size + 1;

    mnemonic = xcalloc(mnemonic_spacing, sizeof(*s));

    for (size_t i = 0; i < mnemonic_spacing; i++) {
        if (i < mnemonic_size)
            mnemonic[i] = tolower(ins.name[i]);
        else
            mnemonic[i] = ' ';
    }

    mnemonic[mnemonic_spacing-1] = '\0';

    temp = mnemonic;
    mnemonic = strcatall(3, conf_disassm_mnemonic_colorcode, mnemonic, "\033[0m");
    xfree(temp);
    temp = NULL;

    if (ins.handler == x86_mm_call || ins.handler == x86_mm_jcc) {
        moffset32_t branchaddress = x86_findbranchtarget_relative(cpu, ins.eip, ins.data);
        struct symbol_lookup_record lookup = sr_lookup(x86_resolver(cpu), branchaddress);
        char *rel = NULL;
        char *target = int2hexstr(branchaddress, 8);

        if (lookup.sl_name) {
            if (lookup.sl_start != branchaddress) {
                rel = int2str(branchaddress - lookup.sl_start);
                s = strcatall(9, mnemonic, conf_disassm_symbol_colorcode, lookup.sl_name, "+", rel, "\033[0m <", conf_disassm_code_address_colorcode, target, "\033[0m>");
            } else {
                s = strcatall(7, mnemonic, conf_disassm_symbol_colorcode, lookup.sl_name, "\033[0m <", conf_disassm_code_address_colorcode, target, "\033[0m>");
            }

            xfree(mnemonic);
            xfree(rel);
            xfree(target);
            return s;
        }
    }

    switch (ins.encoding) {
        case rm32_imm32:
        case rm32_imm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            if (x86_ptrtype(cpu, ins.data.imm1) == DATA_PTR) {
                struct symbol_lookup_record lookup = sr_lookup(x86_resolver(cpu), ins.data.imm1);
                if (lookup.sl_name) {
                    temp = int2hexstr(ins.data.imm1, 0);
                    immediate = strcatall(6, conf_disassm_symbol_colorcode, lookup.sl_name, "\033[0m <", conf_disassm_data_address_colorcode, temp, "\033[0m>");
                } else {
                    immediate = int2hexstr(ins.data.imm1, 0);
                }
            } else {
                immediate = int2hexstr(ins.data.imm1, 0);
            }
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case rm8_imm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case rm16_imm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case rela8:
        case imm8:
        case imm16:
        case rela16:
        case rela32:
        case imm32:
            if (x86_ptrtype(cpu, ins.data.imm1) == DATA_PTR) {
                struct symbol_lookup_record lookup = sr_lookup(x86_resolver(cpu), ins.data.imm1);
                if (lookup.sl_name) {
                    temp = int2hexstr(ins.data.imm1, 0);
                    immediate = strcatall(6, conf_disassm_symbol_colorcode, lookup.sl_name, "\033[0m <", conf_disassm_data_address_colorcode, arg, "\033[0m>");
                }
            } else {
                immediate = int2hexstr(ins.data.imm1, 0);
            }
            s = strcatall(4, mnemonic, conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case imm8_AL:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(7, mnemonic, conf_disassm_number_colorcode,  immediate, ", ", conf_disassm_operand_colorcode, "al", "\033[0m");
            break;
        case imm8_AX:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(7, mnemonic, conf_disassm_number_colorcode, immediate, ", ", conf_disassm_operand_colorcode, "ax", "\033[0m");
            break;
        case imm32_eAX:
        case imm8_eAX:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(7, mnemonic, conf_disassm_number_colorcode, immediate, ", ", conf_disassm_operand_colorcode, "eax", "\033[0m");
            break;
        case AL_imm8:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(7, mnemonic, conf_disassm_operand_colorcode, "al\033[0m", ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case AX_imm16:
        case AX_imm8:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(7, mnemonic, conf_disassm_operand_colorcode, "ax\033[0m", ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case eAX_imm32:
        case eAX_imm8:
            if (x86_ptrtype(cpu, ins.data.imm1) == DATA_PTR) {
                struct symbol_lookup_record lookup = sr_lookup(x86_resolver(cpu), ins.data.imm1);
                if (lookup.sl_name) {
                    temp = int2hexstr(ins.data.imm1, 0);
                    immediate = strcatall(6, conf_disassm_symbol_colorcode, lookup.sl_name, "\033[0m <", conf_disassm_data_address_colorcode, arg, "\033[0m>");
                }
            } else {
                immediate = int2hexstr(ins.data.imm1, 0);
            }
            s = strcatall(8, mnemonic, conf_disassm_operand_colorcode, "eax", "\033[0m", ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case mm1_imm8:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(8, mnemonic, conf_disassm_operand_colorcode, "mm1", "\033[0m", ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case xmm1_imm8:
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(8, mnemonic, conf_disassm_operand_colorcode, "xmm1", "\033[0m", ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case rm16_imm16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            immediate = int2hexstr(ins.data.imm1, 0);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case imm16_imm8:
            arg = int2hexstr(ins.data.imm1, 0);
            immediate = int2hexstr(ins.data.imm2, 0);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, immediate, "\033[0m");
            break;
        case rm8:
        case m8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(2, mnemonic, arg);
            break;
        case rm16:
        case m16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(2, mnemonic, arg);
            break;
        case rm32:
        case m32:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(2, mnemonic, arg);
            break;
        case rm8_1:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, "1", "\033[0m");
            break;
        case rm8_CL:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, "CL", "\033[0m");
            break;
        case rm8_r8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, stringfyregister(reg32to8(ins.data.modrm), 8), "\033[0m");
            break;
        case rm16_1:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, "1", "\033[0m");
            break;
        case rm16_CL:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, "CL", "\033[0m");
            break;
        case rm16_r16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 16), "\033[0m");
            break;
        case rm16_r16_CL:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(8, mnemonic, arg, ", ", conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 16), "\033[0m, ", conf_disassm_operand_colorcode, "CL\033[0m");
            break;
        case rm32_1:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_number_colorcode, "1", "\033[0m");
            break;
        case rm32_CL:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, "CL", "\033[0m");
            break;
        case m32_r32:
        case rm32_r32:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(6, mnemonic, arg, ", ", conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m");
            break;
        case rm32_r32_CL:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(7, mnemonic, arg, ", ", conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m", ", ", conf_disassm_operand_colorcode, "CL\033[0m");
            break;
        case r8_rm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 8), "\033[0m, ", arg);
            break;
        case r16_rm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 16), "\033[0m, ", arg);
            break;
        case r16_m16:
        case r16_rm16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 16), "\033[0m, ", arg);
            break;
        case r32_m32:
        case r32_rm32:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m, ", arg);
            break;
        case r32_rm8:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 8);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m, ", arg);
            break;
        case r32_rm16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m, ", arg);
            break;
        case AX_r16:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 16);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(AX, 16), "\033[0m, ", arg);
            break;
        case eAX_r32:
            arg = modrm2str(ins.data.modrm, ins.data.sib, ins.data.moffset, 32);
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, stringfyregister(EAX, 32), "\033[0m, ", arg);
            break;
        case mm1_mm2:
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, "mm1\033[0m, ", conf_disassm_operand_colorcode, "mm2\033[0m");
            break;
        case xmm1_xmm2:
            s = strcatall(5, mnemonic, conf_disassm_operand_colorcode, "xmm1\033[0m, ", conf_disassm_operand_colorcode, "xmm2\033[0m");
            break;
        case r16:
            s = strcatall(4, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 16), "\033[0m");
            break;
        case r32:
            s = strcatall(4, mnemonic, conf_disassm_operand_colorcode, stringfyregister(reg(ins.data.modrm), 32), "\033[0m");
            break;
        default:
            s = xstrdup(mnemonic);
            break;
    }

    xfree(mnemonic);
    xfree(temp);
    xfree(arg);
    xfree(immediate);

    return s;
}


#define decode_fail(ins, byte)  \
    do {    \
        (ins).fail_to_fetch = 1;    \
        (ins).fail_byte = (byte);    \
        return (ins); \
    } while (0)

struct instruction x86_decode(x86CPU *cpu, moffset32_t eip)
{
    struct instruction ins;
    uint8_t byte;
    int encoding;
    struct opcode *table = x86_opcode_table;
    struct opcode op;
    struct exec_data data;
    uint8_t last_prefix = 0;
    moffset32_t old_eip = eip;

    if (!cpu)
        decode_fail(ins, 0);

    data.ext = 0;
    data.sec = 0;
    data.opc = 0;
    data.modrm = 0;
    data.sib = 0;
    data.imm1 = 0;
    data.imm2 = 0;
    data.oprsz_pfx = 0;
    data.adrsz_pfx = 0;
    data.lock = 0;
    data.repnz = 0;
    data.rep = 0;
    data.segovr = 0;

    ins.fail_to_fetch = 0;
    ins.fail_byte = 0;

    for (size_t isprefix = 1; isprefix != 0;) {
        byte = x86_readM8(cpu, eip);
        eip += 1;
        if (x86_byteispfx(byte)) {

            if (byte == PFX_OPRSZ)
                data.oprsz_pfx = 1;
            else if (byte == PFX_ADDRSZ)
                data.adrsz_pfx = 1;
            else if (byte == PFX_LOCK)
                data.lock = 1;
            else if (byte == PFX_REPNZ)
                data.repnz = 1;
            else if (byte == PFX_REP)
                data.rep = 1;
            else
                data.segovr = byte2segovr(byte);

            last_prefix = byte;
            byte = x86_readM8(cpu, eip);

            eip += 1;
            if (x86_byteispfx(byte))
                continue;
        }
        isprefix = 0;
    }

    // 'two-byte' instructions
    if (byte == 0x0F) {
        table = x86_opcode_0f_table;

        byte = x86_readM8(cpu, eip);
        eip += 1;

    }

    op = table[byte];
    data.opc = byte;

    // handle instructions with secondary bytes (see AAM/AAD)
    if (op.o_sec_table) {
        byte = x86_readM8(cpu, eip);

        for (size_t i = 0; i < op.o_sec_tablesz; i++) {
            if (op.o_sec_table[i].o_opcode == byte) {

                data.sec = byte;
                op = op.o_sec_table[i];
                eip += 1;
            }
        }
    }

    // handle Mod/RM, SIB and opcode extensions
    if (op.o_use_rm) {
        data.modrm = x86_readM8(cpu, eip);
        eip += 1;

        if (op.o_use_op_extension) {
            if (op.o_extensions[reg(data.modrm)].o_opcode)
                op = op.o_extensions[reg(data.modrm)];
        }

        switch (mod(data.modrm)) {
            case 0: // MOD 00 RM 100
            case 1: // MOD 01 RM 100
            case 2: // MOD 10 RM 100
                if (rm(data.modrm) == 4) {
                    data.sib = x86_readM32(cpu, eip);
                    eip += 1;
                }
                break;
            default:
                break;
        }
    }

    // handle instructions with type <prefix> <primary> [secondary]
    if (op.o_prefix && last_prefix) {
        if (table == x86_opcode_table) {
            if (last_prefix == op.o_prefix->o_opcode)
                op = *op.o_prefix;
        } else if (table == x86_opcode_0f_table) {
            if (last_prefix == op.o_prefix[last_prefix & TABLE_0F_PREFIX_MASK].o_opcode)
                op = op.o_prefix[last_prefix & TABLE_0F_PREFIX_MASK];
        }

        if (op.o_sec_table) {
            byte = x86_readM32(cpu, eip);

            for (size_t i = 0; i < op.o_sec_tablesz; i++) {
                if (op.o_sec_table[i].o_opcode == byte) {

                    data.sec = byte;
                    op = op.o_sec_table[i];
                    eip += 1;
                }
            }
        }
    }

    // select a encoding according to the operand-size
    if (data.oprsz_pfx)
        encoding = op.o_encoding16bit;
    else
        encoding = op.o_encoding;

    uint8_t dispsz;
    switch (encoding) {
        case rm8_imm8:
        case rm8_xmm2_imm8:
        case rm32_r32_imm8:
        case rm32_imm8:
        case rm32_xmm1_imm8:
        case rm32_xmm2_imm8:
        case r32_rm32_imm8:
        case xmm1_r32m8_imm8:
        case mm_r32m16_imm8:
        case mm1_mm2m64_imm8:
        case rm16_imm8:
        case rm16_xmm1_imm8:
        case rm16_r16_imm8:
        case r16_rm16_imm8:
        case xmm_r32m16_imm8:
        case xmm1_xmm2m32_imm8:
        case xmm1_xmm2m64_imm8:
        case xmm1_xmm2m128_imm8:
            // if we have a displacement to add to the effective address
            // we get the size of the displacement
            if (data.adrsz_pfx)
                dispsz = displacement16(data.modrm);
            else
                dispsz = displacement32(data.modrm);

            if (dispsz == 8) {
                data.moffset = x86_readM8(cpu, eip);
                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = x86_readM16(cpu, eip);
                eip += 2;
            } else if (dispsz == 32) {
                data.moffset = x86_readM32(cpu, eip);
                eip += 4;
            }
            // FALL THROUGH
        case imm8:
        case imm8_AL:
        case imm8_AX:
        case imm8_eAX:
        case AL_imm8:
        case AX_imm8:
        case eAX_imm8:
        case mm_imm8:
        case mm1_imm8:
        case r32_mm_imm8:
        case r32_xmm_imm8:
        case rela8:
        case xmm1_imm8:
            data.imm1 = x86_readM8(cpu, eip);
            eip += 1;
            break;
        case rm16_imm16:
        case r16_rm16_imm16:
            dispsz = displacement16(data.modrm);

            if (dispsz == 8) {
                data.moffset = x86_readM8(cpu, eip);
                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = x86_readM16(cpu, eip);
                eip += 2;
            }
            // FALL THROUGH
        case imm16:
        case imm16_AX:
        case AX_imm16:
        case rela16:
            data.imm1 = x86_readM16(cpu, eip);
            eip += 2;
            break;
        case rm32_imm32:
        case r32_rm32_imm32:
            dispsz = displacement32(data.modrm);

            if (dispsz == 8) {
                data.moffset = x86_readM8(cpu, eip);
                eip += 1;
            } else if (dispsz == 32) {
                data.moffset = x86_readM32(cpu, eip);
                eip += 4;
            }
        // FALL THROUGH
        case rela16_16:
        case ptr16_16:
        case eAX_imm32:
        case rela32:
        case imm32:
        case imm32_eAX:
            data.imm1 = x86_readM32(cpu, eip);
            eip += 4;
            break;
        case rela16_32:
        case ptr16_32:
            // first 16 bytes
            data.imm1 = x86_readM16(cpu, eip);
            eip += 2;

            // last four bytes
            data.imm2 = x86_readM32(cpu, eip);
            eip += 4;
            break;
        case imm16_imm8:
            // fist two bytes
            data.imm1 = x86_readM16(cpu, eip);
            eip += 2;

            // last byte
            data.imm2 = x86_readM8(cpu, eip);
            eip += 1;
            break;
        case bnd_sib:
        case sib_bnd:
            if (!data.sib) {
                data.sib = x86_readM8(cpu, eip);
                eip += 1;
            }
            break;
        case bnd_rm32:
        case bnd1m64_bnd2:
        case bnd1_bnd2m64:
        case m8:
        case m16:
        case m32:
        case m64:
        case rm8:
        case rm8_1:
        case rm8_CL:
        case rm8_r8:
        case mm1_mm2m32:
        case mm1_mm2m64:
        case mm_xmm1m64:
        case mm_xmm1m128:
        case rm16:
        case rm16_1:
        case rm16_CL:
        case rm16_r16:
        case rm16_r16_CL:
        case rm16_sreg:
        case rm32:
        case rm32_1:
        case rm32_CL:
        case rm32_mm:
        case rm32_r32:
        case rm32_r32_CL:
        case rm32_xmm:
        case r8_rm8:
        case m16_16:
        case m16_32:
        case m32_r32:
        case m64_mm:
        case m64_xmm1:
        case m128_xmm1:
        case mm_m64:
        case mm_rm32:
        case r16_m16:
        case r16_rm8:
        case r16_rm16:
        case r16_r16m16:
        case r32m16:
        case r32_rm32:
        case r32_rm8:
        case r32_rm16:
        case r32_r32m16:
        case xmm1_xmm2m16:
        case xmm1_xmm2m32:
        case xmm1_xmm2m64:
        case xmm1_xmm2m128:
        case xmm1m64_xmm2:
        case xmm2m32_xmm1:
        case xmm2m64_xmm1:
        case xmm2m128_xmm1:
        case xmm_rm32:
        case xmm1_m32:
        case xmm1_m64:
        case xmm1_m128:
        case xmm1_mm1m64:
        case xmm1_rm32:
        case xmm1_r32m32:
        case sreg_rm16:
        case r32_xmm1m32:
        case r32_xmm1m64:
        case r32_m32:
        case r16_m16_16:
        case r32_m16_32:
            if (data.adrsz_pfx)
                dispsz = displacement16(data.modrm);
            else
                dispsz = displacement32(data.modrm);

            if (dispsz == 8) {
                data.moffset = x86_readM8(cpu, eip);
                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = x86_readM16(cpu, eip);
                eip += 2;
            } else if (dispsz == 32) {
                data.moffset = x86_readM32(cpu, eip);
                eip += 4;
            }
            break;
        case AX_r16:
        case eAX_r32:
        case mm_xmm:
        case mm1_mm2:
        case r16:
        case r32:
        case r32_mm:
        case r32_xmm:
        case OP:
        case xmm_mm:
        case xmm1_mm:
        case xmm1_xmm2:
            break;
        case no_encoding:
        default:
            decode_fail(ins, byte);
            return ins;
    }

    ins.name = op.o_name;
    ins.handler = op.o_handler;
    ins.data = data;
    ins.size = eip - old_eip;
    ins.encoding = encoding;
    ins.data.bytes = ins.size;
    ins.eip = old_eip;
    return ins;
}

moffset32_t x86_findbranchtarget_relative(x86CPU *cpu, moffset32_t eip, struct exec_data data)
{
    moffset32_t effctvaddr;

    if (!cpu)
        return 0;

    if (data.oprsz_pfx)
        effctvaddr = x86_effectiveaddress16(cpu, data.modrm, low16(data.moffset));
    else
        effctvaddr = x86_effectiveaddress32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0x77:  // JA rel8
        case 0x73:  // JAE rel8
        case 0x72:  // JB rel8
        case 0x76:  // JBE rel8
        case 0xE3:  // JCXZ rel8
        case 0x74:  // JE rel8
        case 0x7F:  // JG rel7
        case 0x7D:  // JGE rel8
        case 0x7C:  // JL rel8
        case 0x7E:  // JLE rel8
        case 0x75:  // JNE rel8
        case 0x71:  // JNO rel8
        case 0x7B:  // JNP rel8
        case 0x79:  // JNS rel8
        case 0x70:  // JO rel8
        case 0x7A:  // JPE rel8
        case 0x78:  // JS rel8
            return eip + data.bytes + sign8to32(data.imm1);
        case 0x80:  // JO rel32     rel16
        case 0x81:  // JNO rel32     rel16
        case 0x82:  // JB rel32     rel16
        case 0x83:  // JAE rel32    rel16
        case 0x84:  // JE rel32     rel16
        case 0x85:  // JNE rel32     rel16
        case 0x86:  // JBE rel32    rel16
        case 0x87:  // JA rel32     rel16
        case 0x88:  // JS rel32     rel16
        case 0x89:  // JNS rel32     rel16
        case 0x8A:  // JP rel32     rel16
        case 0x8B:  // JNP rel32     rel16
        case 0x8D:  // JGE rel32    rel16
        case 0x8C:  // JL rel32     rel16
        case 0x8E:  // JLE rel32     rel16
        case 0x8F:  // JG rel32     rel16
            if (data.oprsz_pfx)
                return moffset16(eip) + data.bytes + low16(data.imm1);
            else
                return eip + data.bytes + data.imm1;
        case 0xE8:  // CALL rel32   CALL rel16
            return eip + data.bytes + data.imm1;
        case 0xFF:
            if (data.ext == 2) {    // FF /2 CALL r/m32 CALL r/m16
                if (effctvaddr) {
                    if (data.oprsz_pfx)
                        return x86_readM16(cpu, effctvaddr);
                    else
                        return x86_readM32(cpu, effctvaddr);
                } else {
                    if (data.oprsz_pfx)
                        return x86_readR16(cpu, effctvregister(data.modrm, 16));
                    else
                        return x86_readR32(cpu, effctvregister(data.modrm, 32));
                }
            } else {    // FF /3 CALL m16:32  CALL m16:16
                if (data.oprsz_pfx)
                    return x86_readM16(cpu, effctvaddr + 2) + x86_readM16(cpu, effctvaddr);
                else
                    return x86_readM16(cpu, effctvaddr + 4) + x86_readM32(cpu, effctvaddr);
            }
            break;
        case 0x9A:
            ASSERT_NOTREACHED();
            break;
    }

    return 0;
}

moffset32_t x86_findbranchtarget(x86CPU *cpu, struct exec_data data)
{
    return x86_findbranchtarget_relative(cpu, x86_readR32(cpu, EIP), data);
}
