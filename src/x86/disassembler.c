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

#include "disassembler.h"
#include "x86-utils.h"
#include "cpu.h"
#include "../memory.h"
#include "../system.h"

static char *modrm2str(uint8_t, uint8_t, uint32_t, int);
static char *strcatimm(char *, const char *, uint32_t, const char *);
static char *strcatimm2(char *, const char *, const char *, uint32_t);

static char *strcatimm(char *dest, const char *beginning, uint32_t displacement, const char *end)
{
    char *arg = NULL;

    arg = int2hexstr(displacement, 0);
    coolstrcat(dest, 3, beginning, arg, end);
    xfree(arg);


    return dest;
}

static char *strcatimm2(char *dest, const char *beginning, const char *middle, uint32_t displacement)
{
    char *arg = NULL;

    arg = int2hexstr(displacement, 0);
    coolstrcat(dest, 3, beginning, middle, arg);
    xfree(arg);


    return dest;
}


static char *modrm2str(uint8_t modrm, uint8_t sib, uint32_t imm, int oprnd_size)
{
    uint8_t mod = mod(modrm);
    uint8_t rm = rm(modrm);
    uint8_t ss_factor = sibss(sib);
    uint8_t index = sibindex(sib);
    uint8_t base = sibbase(sib);
    char *s = xcalloc(17, sizeof(*s)); /// enough for [EAX*8+EBP+0xdeadbeef]
    _Bool use_sib = 0;

    if (mod == 0) {
        switch (rm) {
            case 0b000: return strcat(s, "[EAX]");
            case 0b001: return strcat(s, "[ECX]");
            case 0b010: return strcat(s, "[EDX]");
            case 0b011: return strcat(s, "[EBX]");
            case 0b100: use_sib = 1; break;
            case 0b101: return strcatimm(s, "[", imm, "]");
            case 0b110: return strcat(s, "[ESI]");
            case 0b111: return strcat(s, "[EDI]");
        }
    } else if (mod == 1 || mod == 2) {
        switch (rm) {
            case 0b000: return strcatimm(s, "[EAX+", imm, "]");
            case 0b001: return strcatimm(s, "[ECX+", imm, "]");
            case 0b010: return strcatimm(s, "[EDX+", imm, "]");
            case 0b011: return strcatimm(s, "[EBX+", imm, "]");
            case 0b100: use_sib = 1; break;
            case 0b101: return strcatimm(s, "[EBP+", imm, "]");
            case 0b110: return strcatimm(s, "[ESI+", imm, "]");
            case 0b111: return strcatimm(s, "[EDI+", imm, "]");
        }
    } else {
        return strcat(s, stringfyregister(rm, oprnd_size));
    }

    if (use_sib) {
        char *regstr = NULL;
        _Bool is_ebp = 0;
        switch (base) {
            case 0b101:
                regstr = xcalloc(15, sizeof(*s)); // enough for EBP+0xdeadbeef
                if (mod == 0)
                    regstr = int2hexstr(imm, 0);
                else
                    strcatimm(regstr, "EBP+", imm, "");
                is_ebp = 1;
                break;
            case 0b000:
            case 0b001:
            case 0b010:
            case 0b011:
            case 0b100:
            case 0b110:
            case 0b111:
                regstr = (char *)stringfyregister(base, 32);
        }

        if (ss_factor == 0b00) {
            switch (index) {
                case 0b000: coolstrcat(s, 3, "[EAX+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b001: coolstrcat(s, 3, "[ECX+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b010: coolstrcat(s, 3, "[EDX+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b011: coolstrcat(s, 3, "[EBX+", regstr, "]"); (is_ebp) ? xfree(regstr): 0; break;
                case 0b100: coolstrcat(s, 3, "[", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b101: coolstrcat(s, 3, "[EBP+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b110: coolstrcat(s, 3, "[ESI+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b111: coolstrcat(s, 3, "[EDI+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
            }
        } else if (ss_factor == 0b01) {
            switch (index) {
                case 0b000: coolstrcat(s, 3, "[EAX*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b001: coolstrcat(s, 3, "[ECX*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b010: coolstrcat(s, 3, "[EDX*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b011: coolstrcat(s, 3, "[EBX*2+", regstr, "]"); (is_ebp) ? xfree(regstr): 0; break;
                case 0b100: coolstrcat(s, 3, "[", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b101: coolstrcat(s, 3, "[EBP*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b110: coolstrcat(s, 3, "[ESI*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b111: coolstrcat(s, 3, "[EDI*2+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
            }
        } else if (ss_factor == 0b10) {
            switch (index) {
                case 0b000: coolstrcat(s, 3, "[EAX*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b001: coolstrcat(s, 3, "[ECX*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b010: coolstrcat(s, 3, "[EDX*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b011: coolstrcat(s, 3, "[EBX*4+", regstr, "]"); (is_ebp) ? xfree(regstr): 0; break;
                case 0b100: coolstrcat(s, 3, "[", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b101: coolstrcat(s, 3, "[EBP*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b110: coolstrcat(s, 3, "[ESI*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b111: coolstrcat(s, 3, "[EDI*4+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
            }
        } else if (ss_factor == 0b10) {
            switch (index) {
                case 0b000: coolstrcat(s, 3, "[EAX*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b001: coolstrcat(s, 3, "[ECX*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b010: coolstrcat(s, 3, "[EDX*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b011: coolstrcat(s, 3, "[EBX*8+", regstr, "]"); (is_ebp) ? xfree(regstr): 0; break;
                case 0b100: coolstrcat(s, 3, "[", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b101: coolstrcat(s, 3, "[EBP*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b110: coolstrcat(s, 3, "[ESI*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
                case 0b111: coolstrcat(s, 3, "[EDI*8+", regstr, "]"); (is_ebp) ? xfree(regstr) : 0; break;
            }
        }
    }

    return s;
}

char *x86_disassemble(struct instruction instr)
{
    char *s = NULL;
    size_t s_size = 35;
    size_t index = 0;
    char *arg = NULL;

    if (instr.encoding == no_encoding)
        return NULL;

    s = xcalloc(s_size, sizeof(*s));

    for (size_t i = 0; i < strlen(instr.name); i++)
        s[index++] = tolower(instr.name[i]);

    s[index++] = ' ';

    switch (instr.encoding) {
        case rm32_imm32:
        case rm32_imm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            strcatimm2(s, arg, ", ", instr.data.imm1);
            xfree(arg);
            break;
        case rm8_imm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            strcatimm2(s, arg, ", ", instr.data.imm1);
            xfree(arg);
            break;
        case rm16_imm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            strcatimm2(s, arg, ", ", instr.data.imm1);
            xfree(arg);
            break;
        case rela8:
        case imm8:
        case imm16:
        case rela16:
        case rela32:
        case imm32:
            arg = int2hexstr(instr.data.imm1, 0);
            strcat(s, arg);
            xfree(arg);
            break;
        case imm8_AL:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, arg, ", ", "AL");
            xfree(arg);
            break;
        case imm8_AX:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, arg, ", ", "AX");
            xfree(arg);
            break;
        case imm32_eAX:
        case imm8_eAX:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, arg, ", ", "EAX");
            xfree(arg);
            break;
        case AL_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, "AL", ", ", arg);
            xfree(arg);
            break;
        case AX_imm16:
        case AX_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, "AX", ", ", arg);
            xfree(arg);
            break;
        case eAX_imm32:
        case eAX_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, "EAX", ", ", arg);
            xfree(arg);
            break;
        case mm1_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, "mm1", ", ", arg);
            xfree(arg);
            break;
        case xmm1_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            coolstrcat(s, 3, "xmm1", ", ", arg);
            xfree(arg);
            break;
        case rm16_imm16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            strcatimm2(s, arg, ", ", instr.data.imm1);
            xfree(arg);
            break;
        case imm16_imm8:
            arg = int2hexstr(instr.data.imm1, 0);
            strcatimm2(s, arg, ", ", instr.data.imm2);
            xfree(arg);
            break;
        case rm8:
        case m8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            strcat(s, arg);
            xfree(arg);
            break;
        case rm16:
        case m16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            strcat(s, arg);
            xfree(arg);
            break;
        case rm32:
        case m32:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            strcat(s, arg);
            xfree(arg);
            break;
        case rm8_1:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, arg, ", ", "1");
            xfree(arg);
            break;
        case rm8_CL:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, arg, ", ", "CL");
            xfree(arg);
            break;
        case rm8_r8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, arg, ", ", stringfyregister(reg32to8(instr.data.modrm), 8));
            xfree(arg);
            break;
        case rm16_1:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, arg, ", ", "1");
            xfree(arg);
            break;
        case rm16_CL:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, arg, ", ", "CL");
            xfree(arg);
            break;
        case rm16_r16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, arg, ", ", stringfyregister(reg(instr.data.modrm), 16));
            xfree(arg);
            break;
        case rm16_r16_CL:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 5, arg, ", ", stringfyregister(reg(instr.data.modrm), 16), ", ", "CL");
            xfree(arg);
            break;
        case rm32_1:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 3, arg, ", ", "1");
            xfree(arg);
            break;
        case rm32_CL:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 3, arg, ", ", "CL");
            xfree(arg);
            break;
        case m32_r32:
        case rm32_r32:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 3, arg, ", ", stringfyregister(reg(instr.data.modrm), 32));
            xfree(arg);
            break;
        case rm32_r32_CL:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 5, arg, ", ", stringfyregister(reg(instr.data.modrm), 32), ", ", "CL");
            xfree(arg);
            break;
        case r8_rm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 8), ", ", arg);
            xfree(arg);
            break;
        case r16_rm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 16), ", ", arg);
            xfree(arg);
            break;
        case r16_m16:
        case r16_rm16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 16), ", ", arg);
            xfree(arg);
            break;
        case r32_m32:
        case r32_rm32:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 32), ", ", arg);
            xfree(arg);
            break;
        case r32_rm8:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 8);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 32), ", ", arg);
            xfree(arg);
            break;
        case r32_rm16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, stringfyregister(reg(instr.data.modrm), 32), ", ", arg);
            xfree(arg);
            break;
        case AX_r16:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 16);
            coolstrcat(s, 3, "AX", ", ", arg);
            xfree(arg);
            break;
        case eAX_r32:
            arg = modrm2str(instr.data.modrm, instr.data.sib, instr.data.imm1, 32);
            coolstrcat(s, 3, "EAX", ", ", arg);
            xfree(arg);
            break;
        case mm1_mm2:
            coolstrcat(s, 3, "mm1", ", ", "mm2");
            break;
        case xmm1_xmm2:
            coolstrcat(s, 3, "xmm1", ", ", "xmm2");
            break;
        case r16:
            strcat(s, stringfyregister(instr.data.opc & EDI, 16));
            break;
        case r32:
            strcat(s, stringfyregister(instr.data.opc & EDI, 32));
            break;
    }

    return s;
}


#define decode_fail(instr, byte)  \
    do {    \
        (instr).fail_to_fetch = 1;    \
        (instr).fail_byte = (byte);    \
        return (instr); \
    } while (0)

struct instruction x86_decode(x86MMU *mmu, moffset32_t eip)
{
    struct instruction instr;
    uint8_t byte;
    int encoding;
    struct opcode *table = x86_opcode_table;
    struct opcode op;
    struct exec_data data;
    uint8_t last_prefix = 0;
    moffset32_t old_eip = eip;

    if (!mmu)
        decode_fail(instr, 0);

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

    instr.fail_to_fetch = 0;
    instr.fail_byte = 0;

    for (size_t isprefix = 1; isprefix != 0;) {
        byte = mmu_fetch(mmu, eip);
        if (mmu_error(mmu))
            decode_fail(instr, byte);
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
            byte = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            if (x86_byteispfx(byte))
                continue;
        }
        isprefix = 0;
    }

    // 'two-byte' instructions
    if (byte == 0x0F) {
        table = x86_opcode_0f_table;

        byte = mmu_fetch(mmu, eip);
        if (mmu_error(mmu))
            decode_fail(instr, byte);
        eip += 1;

    }

    op = table[byte];
    data.opc = byte;

    // handle instructions with secondary bytes (see AAM/AAD)
    if (op.o_sec_table) {
        byte = mmu_fetch(mmu, eip);
        if (mmu_error(mmu))
            decode_fail(instr, byte);

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
        data.modrm = mmu_fetch(mmu, eip);
        if (mmu_error(mmu))
            decode_fail(instr, byte);
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
                    data.sib = mmu_fetch(mmu, eip);
                    if (mmu_error(mmu))
                        decode_fail(instr, byte);
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
            byte = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

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
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 32) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 16;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 24;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
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
            data.imm1 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);
            eip += 1;
            break;
        case rm16_imm16:
        case r16_rm16_imm16:
            dispsz = displacement16(data.modrm);

            if (dispsz == 8) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            }
            // FALL THROUGH
        case imm16:
        case imm16_AX:
        case AX_imm16:
        case rela16:
            data.imm1 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 8;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            break;
        case rm32_imm32:
        case r32_rm32_imm32:
            dispsz = displacement32(data.modrm);

            if (dispsz == 8) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 32) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 16;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 24;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            }
        // FALL THROUGH
        case rela16_16:
        case ptr16_16:
        case eAX_imm32:
        case rela32:
        case imm32:
        case imm32_eAX:
            data.imm1 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 8;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 16;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 24;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            break;
        case rela16_32:
        case ptr16_32:
            // first 16 bytes
            data.imm1 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 8;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;

            // last four bytes
            data.imm2 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm2 |= mmu_fetch(mmu, eip) << 8;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm2 |= mmu_fetch(mmu, eip) << 16;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm2 |= mmu_fetch(mmu, eip) << 24;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            break;
        case imm16_imm8:
            // fist two bytes
            data.imm1 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;
            data.imm1 |= mmu_fetch(mmu, eip) << 8;
            if (mmu_error(mmu))
                decode_fail(instr, byte);

            eip += 1;

            // last byte
            data.imm2 = mmu_fetch(mmu, eip);
            if (mmu_error(mmu))
                decode_fail(instr, byte);
            eip += 1;
            break;
        case bnd_sib:
        case sib_bnd:
            if (!data.sib) {
                data.sib = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);
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
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 16) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
            } else if (dispsz == 32) {
                data.moffset = mmu_fetch(mmu, eip);
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 8;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 16;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
                data.moffset |= mmu_fetch(mmu, eip) << 24;
                if (mmu_error(mmu))
                    decode_fail(instr, byte);

                eip += 1;
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
            decode_fail(instr, byte);
            return instr;
    }

    instr.name = op.o_name;
    instr.handler = op.o_handler;
    instr.data = data;
    instr.size = eip - old_eip;
    instr.encoding = encoding;
    return instr;
}

moffset32_t x86_decodeuntil(x86MMU *mmu, moffset32_t eip, moffset32_t stop_eip)
{
    struct instruction instr;
    moffset32_t addr = 0;

    if (!mmu || stop_eip < eip)
        return 0;

    while (1) {
        instr = x86_decode(mmu, eip);

        if (instr.fail_to_fetch) {
            mmu_clrerror(mmu);
            break;
        }

        if (eip + instr.size >= stop_eip) {
            addr = eip;
            break;
        }

        eip += instr.size;
    }

    return addr;
}

moffset32_t x86_findcalltarget(void *cpu, struct exec_data data)
{
    ASSERT(cpu != NULL);
    moffset32_t effctvaddr;

    if (data.oprsz_pfx)
        effctvaddr = x86_effctvaddr16(cpu, data.modrm, low16(data.moffset));
    else
        effctvaddr = x86_effctvaddr32(cpu, data.modrm, data.sib, data.moffset);

    switch (data.opc) {
        case 0xE8:  // CALL rel32   CALL rel16
            return x86_rdreg32(cpu, EIP) + data.imm1;
        case 0xFF:
            if (data.ext == 2) {    // FF /2 CALL r/m32 CALL r/m16
                if (effctvaddr) {
                    if (data.oprsz_pfx)
                        return x86_rdmem16(cpu, effctvaddr);
                    else
                        return x86_rdmem32(cpu, effctvaddr);
                } else {
                    if (data.oprsz_pfx)
                        return x86_rdreg16(cpu, effctvregister(data.modrm));
                    else
                        return x86_rdreg32(cpu, effctvregister(data.modrm));
                }
            } else {    // FF /3 CALL m16:32  CALL m16:16
                if (data.oprsz_pfx)
                    return x86_rdmem16(cpu, effctvaddr + 2) + x86_rdmem16(cpu, effctvaddr);
                else
                    return x86_rdmem16(cpu, effctvaddr + 4) + x86_rdmem32(cpu, effctvaddr);
            }
            break;
        case 0x9A:
            if (data.oprsz_pfx)
                return high16(data.imm1) + low16(data.imm1);
            else
                return low16(data.imm1) + data.imm2;
            break;
    }

    return 0;
}
