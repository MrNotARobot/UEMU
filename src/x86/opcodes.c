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
 *      this file creates the opcode table that the x86cpu relies on.
 */

#include <stddef.h>

#include "instructions.h"
#include "../memory.h"

//
// UGH, I hate this source file so much for some reason...
//

static size_t alloc_counter = 0;
static size_t alloc_tableend = 10;
static void **alloc_table = NULL;

uint8_t x86_prefix_table[] = { PFX_LOCK, PFX_REPNZ, PFX_REP, PFX_BND, PFX_CS, PFX_SS, PFX_DS,
                               PFX_ES, PFX_FS, PFX_GS, PFX_OPRSZ, PFX_ADDRSZ };

static void add_to_alloc_table(void *);
static void add_to_alloc_table_realloc(void *, void *);

static void register_op_struct(struct opcode *, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_op_table(struct opcode *, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);

static void register_op(uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_op_sec(uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_op_ext(uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_op_prefix_ext(uint8_t, uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_op_prefix_sec(uint8_t, uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);

static void register_0f_op(uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_0f_op_ext(uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_0f_op_sec(uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_0f_op_prefix(uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);
static void register_0f_op_prefix_sec(uint8_t, uint8_t, uint8_t, const char *, int, int, int, int, int, d_x86_instruction_handler);


static void add_to_alloc_table(void *addr)
{
    alloc_table[alloc_counter] = addr;


    alloc_counter++;

    if (alloc_counter == alloc_tableend) {
        alloc_tableend += 10;
        alloc_table = xreallocarray(alloc_table, alloc_tableend, sizeof(*alloc_table));
    }
}

static void add_to_alloc_table_realloc(void *old_addr, void *new_addr)
{
    if (old_addr == NULL) {
        add_to_alloc_table(new_addr);
        return;
    }

    if (old_addr == new_addr)
        return;

    for (size_t i = 0; i < alloc_counter; i++) {
        if (alloc_table[i] == old_addr) {
            alloc_table[i] = new_addr;
            break;
        }
    }
}

// I'm lazy and tired of repeating this in functions so now we got this beauty of function
static void register_op_struct(struct opcode *table, uint8_t opcode,
                                const char *name, int class, int encoding, int encoding16bit,
                                int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    table->o_opcode = opcode;
    table->o_name = name;
    table->o_class = class;
    table->o_encoding = encoding;
    table->o_encoding16bit = encoding16bit;
    table->o_handler = handler;
    table->o_use_rm = use_rm;
    table->o_is_prefix = is_prefix;
    table->o_use_op_extension = 0;
    table->o_sec_table = NULL;
    table->o_sec_tablesz = 0;
    table->o_extensions = NULL;
    table->o_prefix = NULL;
}

static void register_op_table(struct opcode *table, uint8_t opcode,
                                const char *name, int class, int encoding, int encoding16bit,
                                int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    register_op_struct(&table[opcode], opcode, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

// one-byte table

static void register_op(uint8_t opcode, const char *name, int class, int encoding,
        int encoding16bit, int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    register_op_table(x86_opcode_table, opcode, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

static void register_op_sec(uint8_t primary, uint8_t secondary, const char * name,
                            int class, int encoding, int encoding16bit, int use_rm,
                            int is_prefix, d_x86_instruction_handler handler)
{
    size_t n;
    struct opcode *primary_op = &x86_opcode_table[primary];
    void *old_sec_table = primary_op->o_sec_table;

    n = primary_op->o_sec_tablesz += 1;

    primary_op->o_sec_table = xreallocarray(primary_op->o_sec_table, n, sizeof(struct opcode));

    add_to_alloc_table_realloc(old_sec_table, primary_op->o_sec_table);

    register_op_struct(&primary_op->o_sec_table[n-1], secondary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

static void register_op_ext(uint8_t primary, uint8_t extension, const char *name,
                            int class, int encoding, int encoding16bit, int use_rm,
                            int is_prefix, d_x86_instruction_handler handler)
{
    struct opcode *primary_op = &x86_opcode_table[primary];

    if (!primary_op->o_extensions) {
        primary_op->o_extensions = xcalloc(8, sizeof(struct opcode));
        primary_op->o_use_op_extension = 1;
        primary_op->o_use_rm = 1;

        add_to_alloc_table(primary_op->o_extensions);
    }

    register_op_struct(&primary_op->o_extensions[extension], primary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

static void register_op_prefix_ext(uint8_t prefix, uint8_t primary, uint8_t extension,
                                const char * name, int class, int encoding, int encoding16bit,
                                int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    struct opcode *primary_op = &x86_opcode_table[primary];
    struct opcode *ext_op;

    if (!primary_op->o_extensions) {
        primary_op->o_extensions = xcalloc(8, sizeof(struct opcode));
        primary_op->o_use_op_extension = 1;
        primary_op->o_use_rm = 1;

        add_to_alloc_table(primary_op->o_extensions);
    }

    ext_op = &primary_op->o_extensions[extension];
    ext_op->o_opcode = primary;

    ext_op->o_prefix = xcalloc(1, sizeof(struct opcode));
        add_to_alloc_table(ext_op->o_prefix);

    register_op_struct(ext_op->o_prefix, prefix, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

static void register_op_prefix_sec(uint8_t prefix, uint8_t primary, uint8_t secondary,
        const char *name, int class, int encoding, int encoding16bit,
        int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    size_t n;
    struct opcode *primary_op = &x86_opcode_table[primary];
    struct opcode *primary_prefix;
    void *old_sec_table;

    if (!primary_op->o_prefix) {
        primary_op->o_prefix = xcalloc(1, sizeof(struct opcode));

        add_to_alloc_table(primary_op->o_prefix);
    }

    primary_prefix = primary_op->o_prefix;
    old_sec_table = primary_prefix->o_sec_table;

    primary_prefix->o_opcode = prefix;

    n = primary_prefix->o_sec_tablesz += 1;

    primary_prefix->o_sec_table = xreallocarray(primary_prefix->o_sec_table, n, sizeof(struct opcode));

    add_to_alloc_table_realloc(old_sec_table, primary_prefix->o_sec_table);

    register_op_struct(&primary_prefix->o_sec_table[n-1], secondary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

// two-bytes table

static void register_0f_op(uint8_t opcode, const char *name, int class, int encoding,
        int encoding16bit, int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    register_op_table(x86_opcode_0f_table, opcode, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

static void register_0f_op_ext(uint8_t primary, uint8_t extension, const char *name,
                            int class, int encoding, int encoding16bit, int use_rm,
                            int is_prefix, d_x86_instruction_handler handler)
{
    struct opcode *primary_op = &x86_opcode_0f_table[primary];
    if (!primary_op->o_extensions) {
        primary_op->o_extensions = xcalloc(8, sizeof(struct opcode));
        primary_op->o_use_op_extension = 1;
        primary_op->o_use_rm = 1;

        add_to_alloc_table(primary_op->o_extensions);
    }

    register_op_struct(&primary_op->o_extensions[extension], primary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}


static void register_0f_op_prefix(uint8_t prefix, uint8_t primary, const char *name,
                            int class, int encoding, int encoding16bit, int use_rm, int is_prefix,
                            d_x86_instruction_handler handler)
{
    struct opcode *primary_op = &x86_opcode_0f_table[primary];

    if (!primary_op->o_prefix) {
        primary_op->o_prefix = xcalloc(6, sizeof(struct opcode));

        add_to_alloc_table(primary_op->o_prefix);
    }

    register_op_table(primary_op->o_prefix, prefix & 5, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);

    primary_op->o_prefix[prefix & 5].o_opcode = prefix;
}

static void register_0f_op_sec(uint8_t primary, uint8_t secondary, const char *name,
                            int class, int encoding, int encoding16bit, int use_rm,
                            int is_prefix, d_x86_instruction_handler handler)
{
    size_t n;
    struct opcode *primary_op = &x86_opcode_0f_table[primary];
    void *old_sec_table = primary_op->o_sec_table;

    n = primary_op->o_sec_tablesz += 1;
    primary_op->o_sec_table = xreallocarray(primary_op->o_sec_table, n, sizeof(struct opcode));

    add_to_alloc_table_realloc(old_sec_table, primary_op->o_sec_table);

    register_op_struct(&primary_op->o_sec_table[n-1], secondary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}


static void register_0f_op_prefix_sec(uint8_t prefix, uint8_t primary, uint8_t secondary,
                            const char *name, int class, int encoding, int encoding16bit,
                            int use_rm, int is_prefix, d_x86_instruction_handler handler)
{
    size_t n;
    struct opcode *primary_op = &x86_opcode_0f_table[primary];
    struct opcode *primary_prefix;
    void *old_sec_table;

    if (!primary_op->o_prefix) {
        primary_op->o_prefix = xcalloc(6, sizeof(struct opcode));

        add_to_alloc_table(primary_op->o_prefix);
    }

    primary_prefix = &primary_op->o_prefix[prefix & 5];
    primary_prefix->o_opcode = prefix;

    old_sec_table = primary_prefix->o_sec_table;

    n = primary_prefix->o_sec_tablesz += 1;

    primary_prefix->o_sec_table = xreallocarray(primary_prefix->o_sec_table, n, sizeof(struct opcode));

    add_to_alloc_table_realloc(old_sec_table, primary_prefix->o_sec_table);

    register_op_struct(&primary_prefix->o_sec_table[n-1], secondary, name, class, encoding, encoding16bit, use_rm, is_prefix, handler);
}

/////////////////

_Bool x86_byteispfx(uint8_t byte)
{
    for (size_t i = 0; i < 11; i++) {
        if (x86_prefix_table[i] == byte)
            return 1;
    }

    return 0;
}

uint8_t byte2segovr(uint8_t byte)
{
    uint8_t segovr = 0;
    switch (byte) {
        case PFX_FS: segovr = SEG_FS; break;
        case PFX_CS: segovr = SEG_CS; break;
        case PFX_SS: segovr = SEG_SS; break;
        case PFX_DS: segovr = SEG_DS; break;
        case PFX_ES: segovr = SEG_ES; break;
        case PFX_GS: segovr = SEG_GS; break;
    }

    return segovr;
}

void x86_free_opcode_table(void)
{
    for (size_t i = 0; i < alloc_counter; i++)
        xfree(alloc_table[i]);

    xfree(alloc_table);
}

void x86_init_opcode_table(void)
{
    alloc_table = xcalloc(alloc_tableend, sizeof(*alloc_table));

    register_op(0x00, "ADD", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_add);
    register_op(0x01, "ADD", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_add);
    register_op(0x02, "ADD", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_add);
    register_op(0x03, "ADD", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_add);
    register_op(0x04, "ADD", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_add);
    register_op(0x05, "ADD", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_add);
    register_op(0x06, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_op(0x07, "POP", NONE, OP, OP, NO_RM, INSTR, x86_mm_pop);
    register_op(0x08, "OR", NONE, rm8_r8, rm8_r8, NO_RM, INSTR, x86_or);
    register_op(0x09, "OR", NONE, rm32_r32, rm16_r16, NO_RM, INSTR, x86_or);
    register_op(0x0A, "OR", NONE, r8_rm8, r8_rm8, NO_RM, INSTR, x86_or);
    register_op(0x0B, "OR", NONE, r32_rm32, r16_rm16, NO_RM, INSTR, x86_or);
    register_op(0x0C, "OR", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_or);
    register_op(0x0D, "OR", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_or);
    register_op(0x0E, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_op(0x0F, NULL, NONE, no_encoding, no_encoding, NO_RM, PREFIX, NULL);
    register_op(0x10, "ADC", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_adc);
    register_op(0x11, "ADC", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_adc);
    register_op(0x12, "ADC", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_adc);
    register_op(0x13, "ADC", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_adc);
    register_op(0x14, "ADC", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_adc);
    register_op(0x15, "ADC", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_adc);
    register_op(0x16, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_op(0x17, "POP", NONE, OP, OP, NO_RM, INSTR, x86_mm_pop);
    register_op(0x18, "SBB", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_sbb);
    register_op(0x19, "SBB", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_sbb);
    register_op(0x1A, "SBB", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_sbb);
    register_op(0x1B, "SBB", NONE, r32_rm32, r32_rm32, USE_RM, INSTR, x86_sbb);
    register_op(0x1C, "SBB", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_sbb);
    register_op(0x1D, "SBB", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_sbb);
    register_op(0x1E, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_op(0x1F, "POP", NONE, OP, OP, NO_RM, INSTR, x86_mm_pop);
    register_op(0x20, "AND", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_mm_and);
    register_op(0x21, "AND", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_mm_and);
    register_op(0x22, "AND", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_mm_and);
    register_op(0x23, "AND", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_mm_and);
    register_op(0x24, "AND", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_mm_and);
    register_op(0x25, "AND", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_mm_and);

    register_op(0x27, "DAA", NONE, OP, OP, NO_RM, INSTR, x86_daa);
    register_op(0x28, "SUB", NONE, rm8_r8, rm8_r8, NO_RM, INSTR, x86_sub);
    register_op(0x29, "SUB", NONE, rm32_r32, rm16_r16, NO_RM, INSTR, x86_sub);
    register_op(0x2A, "SUB", NONE, r8_rm8, r8_rm8, NO_RM, INSTR, x86_sub);
    register_op(0x2B, "SUB", NONE, r32_rm32, r16_rm16, NO_RM, INSTR, x86_sub);
    register_op(0x2C, "SUB", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_sub);
    register_op(0x2D, "SUB", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_sub);

    register_op(0x2F, "DAS", NONE, OP, OP, NO_RM, INSTR, x86_das);
    register_op(0x30, "XOR", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_mm_xor);
    register_op(0x31, "XOR", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_mm_xor);
    register_op(0x32, "XOR", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_mm_xor);
    register_op(0x33, "XOR", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_mm_xor);
    register_op(0x34, "XOR", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_mm_xor);
    register_op(0x35, "XOR", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_mm_xor);

    register_op(0x37, "AAA", NONE, OP, OP, NO_RM, INSTR, x86_aaa);
    register_op(0x38, "CMP", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_cmp);
    register_op(0x39, "CMP", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_cmp);
    register_op(0x3A, "CMP", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_cmp);
    register_op(0x3B, "CMP", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmp);
    register_op(0x3C, "CMP", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_cmp);
    register_op(0x3D, "CMP", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_cmp);

    register_op(0x3f, "AAS", NONE, OP, OP, NO_RM, INSTR, x86_aas);
    for (size_t i = 0; i < 8; i++)
        register_op(0x40 + i, "INC", NONE, OP, OP, NO_RM, INSTR, x86_inc);
    for (size_t i = 0; i < 8; i++)
        register_op(0x48 + i, "DEC", NONE, OP, OP, NO_RM, INSTR, x86_dec);

    for (size_t i = 0; i < 8; i++)
        register_op(0x50 + i, "PUSH", NONE, r32, r16, NO_RM, INSTR, x86_mm_push);
    for (size_t i = 0; i < 8; i++)
        register_op(0x58 + i, "POP", NONE, r32, r16, NO_RM, INSTR, x86_mm_pop);
    register_op(0x60, "PUSHA", NONE, OP, OP, NO_RM, INSTR, x86_pusha);
    register_op(0x61, "POPA", NONE, OP, OP, NO_RM, INSTR, x86_popa);
    register_op(0x62, "BOUND", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_bound);
    register_op(0x63, "ARPL", NONE, rm16_r16, rm16_r16, USE_RM, INSTR, x86_arpl);

    register_op(0x66, NULL,  NONE, no_encoding, no_encoding, NO_RM, PREFIX, NULL);

    register_op(0x68, "PUSH", NONE, imm32, imm16, NO_RM, INSTR, x86_mm_push);
    register_op(0x69, "IMUL",  NONE, r32_rm32_imm32, r16_rm16_imm16, USE_RM, INSTR, x86_imul);
    register_op(0x6A, "PUSH", NONE, imm8, imm8, NO_RM, INSTR, x86_mm_push);
    register_op(0x6B, "IMUL",  NONE, r32_rm32_imm8, r16_rm16_imm8, USE_RM, INSTR, x86_imul);
    register_op(0x6C, "INS",  NONE, OP, OP, NO_RM, INSTR, x86_ins);
    register_op(0x6D, "INS",  NONE, OP, OP, NO_RM, INSTR, x86_ins);
    register_op(0x6E, "OUTS",  NONE, OP, OP, NO_RM, INSTR, x86_outs);
    register_op(0x6F, "OUTS",  NONE, OP, OP, NO_RM, INSTR, x86_outs);
    register_op(0x70, "JO",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x71, "JNO",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x72, "JB",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x73, "JAE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x74, "JE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x75, "JNE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x76, "JBE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x77, "JA",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x78, "JS",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x79, "JNS",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7A, "JP",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7B, "JNP",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7C, "JL",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7D, "JGE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7E, "JLE",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0x7F, "JG",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);

    register_op_ext(0x80, 0, "ADD", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_add);
    register_op_ext(0x80, 1, "OR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_or);
    register_op_ext(0x80, 2, "ADC", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_adc);
    register_op_ext(0x80, 3, "SBB", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sbb);
    register_op_ext(0x80, 4, "AND", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_mm_and);
    register_op_ext(0x80, 5, "SUB", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sub);
    register_op_ext(0x80, 6, "XOR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_mm_xor);
    register_op_ext(0x80, 7, "CMP", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_cmp);
    register_op_ext(0x81, 0, "ADD", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_add);
    register_op_ext(0x81, 1, "OR", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_or);
    register_op_ext(0x81, 2, "ADC", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_adc);
    register_op_ext(0x81, 3, "SBB", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_sbb);
    register_op_ext(0x81, 4, "AND", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_mm_and);
    register_op_ext(0x81, 5, "SUB", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_sub);
    register_op_ext(0x81, 6, "XOR", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_mm_xor);
    register_op_ext(0x81, 7, "CMP", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_cmp);
    register_op_ext(0x82, 0, "ADD", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_add);
    register_op_ext(0x82, 1, "OR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_or);
    register_op_ext(0x82, 2, "ADC", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_adc);
    register_op_ext(0x82, 3, "SBB", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sbb);
    register_op_ext(0x82, 4, "AND", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_mm_and);
    register_op_ext(0x82, 5, "SUB", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sub);
    register_op_ext(0x82, 6, "XOR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_mm_xor);
    register_op_ext(0x82, 7, "CMP", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_cmp);
    register_op_ext(0x83, 0, "ADD", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_add);
    register_op_ext(0x83, 1, "OR", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_or);
    register_op_ext(0x83, 2, "ADC", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_adc);
    register_op_ext(0x83, 3, "ADC", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_sbb);
    register_op_ext(0x83, 4, "AND", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_mm_and);
    register_op_ext(0x83, 5, "SUB", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_sub);
    register_op_ext(0x83, 6, "XOR", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_mm_xor);
    register_op_ext(0x83, 7, "CMP", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_cmp);
    register_op(0x84, "TEST", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_test);
    register_op(0x85, "TEST", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_test);
    register_op(0x86, "XCHG", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_xchg);
    register_op(0x87, "XCHG", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_xchg);
    register_op(0x88, "MOV", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_mm_mov);
    register_op(0x89, "MOV", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_mm_mov);
    register_op(0x8A, "MOV", NONE, r8_rm8, r8_rm8, USE_RM, INSTR, x86_mm_mov);
    register_op(0x8B, "MOV", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_mm_mov);
    register_op(0x8C, "MOV", NONE, rm16_sreg, rm16_sreg, USE_RM, INSTR, x86_mm_mov);
    register_op(0x8D, "LEA", NONE, r32_m32, r16_m16, NO_RM, INSTR, x86_lea);
    register_op(0x8E, "MOV", NONE, sreg_rm16, sreg_rm16, USE_RM, INSTR, x86_mm_mov);
    register_op_ext(0x8F, 0, "POP", NONE, rm32, rm16, USE_RM, INSTR, x86_mm_pop);
    register_op(0x90, "NOP", NONE, OP, OP, NO_RM, INSTR, x86_mm_nop);
    for (size_t i = 0; i < 8; i++)
        register_op(0x90 + i, "XCHG", NONE, eAX_r32, AX_r16, NO_RM, INSTR, x86_xchg);
    register_op(0x98, "CBW", NONE, OP, OP, NO_RM, INSTR, x86_cbw);
    register_op(0x99, "CWQ", NONE, OP, OP, NO_RM, INSTR, x86_cwq);
    register_op(0x9A, "CALL", NONE, rela16_32, rela16_16, NO_RM, INSTR, x86_mm_call);
    register_op(0x9B, "WAIT", NONE, OP, OP, NO_RM, INSTR, x86_wait);
    register_op(0x9C, "PUSHF", NONE, OP, OP, NO_RM, INSTR, x86_pushf);
    register_op(0x9D, "POPF", NONE, OP, OP, NO_RM, INSTR, x86_popf);
    register_op(0x9E, "SAHF", NONE, OP, OP, NO_RM, INSTR, x86_sahf);
    register_op(0x9F, "LAHF", NONE, OP, OP, NO_RM, INSTR, x86_lahf);
    register_op(0xA0, "MOV", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_mm_mov);
    register_op(0xA1, "MOV", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_mm_mov);
    register_op(0xA2, "MOV", NONE, imm8_AL, imm8_AL, NO_RM, INSTR, x86_mm_mov);
    register_op(0xA3, "MOV", NONE, imm16_AX, imm32_eAX, NO_RM, INSTR, x86_mm_mov);
    register_op(0xA4, "MOVS", NONE, OP, OP, NO_RM, INSTR, x86_movs);
    register_op(0xA5, "MOVS", NONE, OP, OP, NO_RM, INSTR, x86_movs);
    register_op(0xA6, "CMPS", NONE, OP, OP, NO_RM, INSTR, x86_cmps);
    register_op(0xA7, "CMPS", NONE, OP, OP, NO_RM, INSTR, x86_cmps);
    register_op(0xA8, "TEST", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_test);
    register_op(0xA9, "TEST", NONE, eAX_imm32, AX_imm16, NO_RM, INSTR, x86_test);
    register_op(0xAA, "STOS", NONE, m8, m8, NO_RM, INSTR, x86_stos);
    register_op(0xAB, "STOS", NONE, m32, m16, NO_RM, INSTR, x86_stos);
    register_op(0xAC, "LODS", NONE, OP, OP, NO_RM, INSTR, x86_lods);
    register_op(0xAD, "LODS", NONE, OP, OP, NO_RM, INSTR, x86_lods);
    register_op(0xAE, "SCAS", NONE, m8, m8, NO_RM, INSTR, x86_scas);
    register_op(0xAF, "SCAS", NONE, m32, m16, NO_RM, INSTR, x86_scas);
    for (size_t i = 0; i < 8; i++)
        register_op(0xB0 + i, "MOV", NONE, imm8, imm8, NO_RM, INSTR, x86_mm_mov);
    for (size_t i = 0; i < 8; i++)
        register_op(0xB8 + i, "MOV", NONE, imm32, imm16, NO_RM, INSTR, x86_mm_mov);
    register_op_ext(0xC0, 0, "ROL", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_rol);
    register_op_ext(0xC0, 1, "ROR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_ror);
    register_op_ext(0xC0, 2, "RCL", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xC0, 3, "RCR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xC0, 4, "SAL", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sal);
    register_op_ext(0xC0, 5, "SHR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_shr);
    register_op_ext(0xC0, 6, "SAL", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sal);
    register_op_ext(0xC0, 7, "SAR", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_sar);
    register_op_ext(0xC1, 0, "ROL", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_rol);
    register_op_ext(0xC1, 1, "ROR", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_ror);
    register_op_ext(0xC1, 2, "RCL", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xC1, 3, "RCR", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xC1, 4, "SAL", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_sal);
    register_op_ext(0xC1, 5, "SHR", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_shr);
    register_op_ext(0xC1, 6, "SAL", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_sal);
    register_op_ext(0xC1, 7, "SAR", NONE, rm32_imm8, rm32_imm8, USE_RM, INSTR, x86_sar);
    register_op(0xC2, "RET", NONE, imm16, imm16, NO_RM, INSTR, x86_ret);
    register_op(0xC3, "RET", NONE, OP, OP, NO_RM, INSTR, x86_ret);
    register_op(0xC4, "LFS", NONE, r32_m16_32, r16_m16_16, USE_RM, INSTR, x86_lfs);
    register_op(0xC5, "LDS", NONE, r32_m16_32, r16_m16_16, USE_RM, INSTR, x86_lds);
    register_op_ext(0xC6, 0, "MOV", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_mm_mov);
    register_op_sec(0xC6, 0xF8, "XABORT", RTM, imm8, imm8, USE_RM, INSTR, x86_xabort);
    register_op_ext(0xC7, 0, "MOV", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_mm_mov);
    register_op_sec(0xC7, 0xF8, "XBEGIN", RTM, rela32, rela16, USE_RM, INSTR, x86_xbegin);
    register_op(0xC8, "ENTER", NONE, imm16_imm8, imm16_imm8, NO_RM, INSTR, x86_enter);
    register_op(0xC9, "LEAVE", NONE, OP, OP, NO_RM, INSTR, x86_leave);
    register_op(0xCA, "RET", NONE, imm16, imm16, NO_RM, INSTR, x86_ret);
    register_op(0xCB, "RET", NONE, OP, OP, NO_RM, INSTR, x86_ret);
    register_op(0xCC, "INT3", NONE, OP, OP, NO_RM, INSTR, x86_int3);
    register_op(0xCD, "INT", NONE, imm8, imm8, NO_RM, INSTR, x86_int);
    register_op(0xCE, "INT0", NONE, OP, OP, NO_RM, INSTR, x86_int0);
    register_op(0xCF, "IRET", NONE, OP, OP, NO_RM, INSTR, x86_iret);
    register_op_ext(0xD0, 0, "ROL", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_rol);
    register_op_ext(0xD0, 1, "ROR", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_ror);
    register_op_ext(0xD0, 2, "RCL", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xD0, 3, "RCR", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xD0, 4, "SAL", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD0, 5, "SHR", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_shr);
    register_op_ext(0xD0, 6, "SAL", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD0, 7, "SAR", NONE, rm8_1, rm8_1, USE_RM, INSTR, x86_sar);
    register_op_ext(0xD1, 0, "ROL", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_rol);
    register_op_ext(0xD1, 1, "ROR", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_ror);
    register_op_ext(0xD1, 2, "RCL", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xD1, 3, "RCR", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xD1, 4, "SAL", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD1, 5, "SHR", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_shr);
    register_op_ext(0xD1, 6, "SAL", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD1, 7, "SAR", NONE, rm32_1, rm16_1, USE_RM, INSTR, x86_sar);
    register_op_ext(0xD2, 0, "ROL", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_rol);
    register_op_ext(0xD2, 1, "ROR", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_ror);
    register_op_ext(0xD2, 2, "RCL", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xD2, 3, "RCR", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xD2, 4, "SAL", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD2, 5, "SHR", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_shr);
    register_op_ext(0xD2, 6, "SAL", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD2, 7, "SAR", NONE, rm8_CL, rm8_CL, USE_RM, INSTR, x86_sar);
    register_op_ext(0xD3, 0, "ROL", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_rol);
    register_op_ext(0xD3, 1, "ROR", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_ror);
    register_op_ext(0xD3, 2, "RCL", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_rcl);
    register_op_ext(0xD3, 3, "RCR", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_rcr);
    register_op_ext(0xD3, 4, "SAL", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD3, 5, "SHR", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_shr);
    register_op_ext(0xD3, 6, "SAL", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_sal);
    register_op_ext(0xD3, 7, "SAR", NONE, rm32_CL, rm16_CL, USE_RM, INSTR, x86_sar);
    register_op(0xD4, "AAM", NONE, OP, OP, NO_RM, INSTR, x86_aam);
    register_op_sec(0xD4, 0x0A, "AAM", NONE, imm8, imm8, NO_RM, INSTR, x86_aam);
    register_op(0xD5, "AAD", NONE, imm8, imm8, NO_RM, INSTR, x86_aad);
    register_op_sec(0xD5, 0x0A, "AAD", NONE, OP, OP, NO_RM, INSTR, x86_aad);

    register_op(0xD7, "XLAT", NONE, m8, m8, NO_RM, INSTR, x86_xlat);
    register_op_ext(0xD8, 0, "FADD", NONE, OP, OP, USE_RM, INSTR, x86_fadd);
    register_op_ext(0xD8, 1, "FMUL", NONE, OP, OP, NO_RM, INSTR, x86_fmul);
    register_op_ext(0xD8, 2, "FCOM", NONE, OP, OP, USE_RM, INSTR, x86_fcom);
    register_op_ext(0xD8, 3, "FCOMP", NONE, OP, OP, USE_RM, INSTR, x86_fcomp);
    register_op_ext(0xD8, 4, "FSUB", NONE, OP, OP, USE_RM, INSTR, x86_fsub);
    register_op_ext(0xD8, 5, "FSUBR", NONE, OP, OP, USE_RM, INSTR, x86_fsubr);
    register_op_ext(0xD8, 6, "FDIV", NONE, OP, OP, NO_RM, INSTR, x86_fdiv);
    register_op_ext(0xD8, 7, "FDIVR", NONE, OP, OP, USE_RM, INSTR, x86_fdivr);
    register_op_sec(0xD8, 0xD1, "FCOM", NONE, OP, OP, NO_RM, INSTR, x86_fcom);
    register_op_sec(0xD8, 0xD9, "FCOMP", NONE, OP, OP, NO_RM, INSTR, x86_fcom);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xC0 + i, "FADD", NONE, OP, OP, NO_RM, INSTR, x86_fadd);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xC8 + i, "FMUL", NONE, OP, OP, NO_RM, INSTR, x86_fmul);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xD0 + i, "FCOM", NONE, OP, OP, NO_RM, INSTR, x86_fcom);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xD8 + i, "FCOMP", NONE, OP, OP, NO_RM, INSTR, x86_fcomp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xE0 + i, "FSUB", NONE, OP, OP, NO_RM, INSTR, x86_fsub);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xE8 + i, "FSUBR", NONE, OP, OP, NO_RM, INSTR, x86_fsubr);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xF0 + i, "FDIV", NONE, OP, OP, NO_RM, INSTR, x86_fdiv);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD8, 0xF8 + i, "FDIVR", NONE, OP, OP, NO_RM, INSTR, x86_fdivr);
    register_op_sec(0xD9, 0xE0, "FCHS", NONE, OP, OP, NO_RM, INSTR, x86_fchs);
    register_op_sec(0xD9, 0xE1, "FABS", NONE, OP, OP, NO_RM, INSTR, x86_fabs);

    register_op_ext(0xD9, 0, "FLD", NONE, OP, OP, USE_RM, INSTR, x86_fld);
    register_op_ext(0xD9, 2, "FST", NONE, OP, OP, USE_RM, INSTR, x86_fst);
    register_op_ext(0xD9, 3, "FSTP", NONE, OP, OP, USE_RM, INSTR, x86_fstp);
    register_op_ext(0xD9, 4, "FLDENV", NONE, OP, OP, USE_RM, INSTR, x86_fldenv);
    register_op_ext(0xD9, 5, "FLDCW", NONE, OP, OP, USE_RM, INSTR, x86_fldcw);
    register_op_ext(0xD9, 6, "FNSTENV", NONE, OP, OP, USE_RM, INSTR, x86_fnstenv);
    register_op_prefix_ext(0x9B, 0xD9, 7, "FSTENV", NONE, OP, OP, USE_RM, INSTR, x86_fstenv);
    register_op_ext(0xD9, 7, "FNSTCW", NONE, OP, OP, USE_RM, INSTR, x86_fnstcw);
    register_op_prefix_ext(0x9B, 0xD9, 7, "FNSTCW", NONE, OP, OP, USE_RM, INSTR, x86_fnstcw);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD9, 0xC8 + i, "FXCH", NONE, OP, OP, NO_RM, INSTR, x86_fxch);
    register_op_sec(0xD9, 0xD0, "FNOP", NONE, OP, OP, NO_RM, INSTR, x86_fnop);
    register_op_sec(0xD9, 0xE4, "FTST", NONE, OP, OP, NO_RM, INSTR, x86_ftst);
    register_op_sec(0xD9, 0xE5, "FXAM", NONE, OP, OP, NO_RM, INSTR, x86_fxam);
    register_op_sec(0xD9, 0xE8, "FLD1", NONE, OP, OP, NO_RM, INSTR, x86_fld1);
    register_op_sec(0xD9, 0xE9, "FLDL2T", NONE, OP, OP, NO_RM, INSTR, x86_fldl2t);
    register_op_sec(0xD9, 0xEA, "FLDL2E", NONE, OP, OP, NO_RM, INSTR, x86_fldl2e);
    register_op_sec(0xD9, 0xEB, "FLDPI", NONE, OP, OP, NO_RM, INSTR, x86_fldpi);
    register_op_sec(0xD9, 0xEC, "FLDLG2", NONE, OP, OP, NO_RM, INSTR, x86_fldlg2);
    register_op_sec(0xD9, 0xED, "FLDLN2", NONE, OP, OP, NO_RM, INSTR, x86_fldln2);
    register_op_sec(0xD9, 0xEE, "FLDZ", NONE, OP, OP, NO_RM, INSTR, x86_fldz);
    register_op_sec(0xD9, 0xF0, "F2XM1", NONE, OP, OP, NO_RM, INSTR, x86_f2xm1);
    register_op_sec(0xD9, 0xF1, "FYL2X", NONE, OP, OP, NO_RM, INSTR, x86_fyl2x);
    register_op_sec(0xD9, 0xF2, "FPTAN", NONE, OP, OP, NO_RM, INSTR, x86_fptan);
    register_op_sec(0xD9, 0xF3, "FPATAN", NONE, OP, OP, NO_RM, INSTR, x86_fpatan);
    register_op_sec(0xD9, 0xF4, "FXTRACT", NONE, OP, OP, NO_RM, INSTR, x86_fxtract);
    register_op_sec(0xD9, 0xF5, "FPREM1", NONE, OP, OP, NO_RM, INSTR, x86_fprem1);
    register_op_sec(0xD9, 0xF6, "FDECSTP", NONE, OP, OP, NO_RM, INSTR, x86_fdecstp);
    register_op_sec(0xD9, 0xF7, "FINCSTP", NONE, OP, OP, NO_RM, INSTR, x86_fincstp);
    register_op_sec(0xD9, 0xF8, "FPREM", NONE, OP, OP, NO_RM, INSTR, x86_fprem);
    register_op_sec(0xD9, 0xF9, "FYL2XP1", NONE, OP, OP, NO_RM, INSTR, x86_fyl2xp1);
    register_op_sec(0xD9, 0xFB, "FSINCOS", NONE, OP, OP, NO_RM, INSTR, x86_fsincos);
    register_op_sec(0xD9, 0xFA, "FSQRT", NONE, OP, OP, NO_RM, INSTR, x86_fsqrt);
    register_op_sec(0xD9, 0xFC, "FRNDINT", NONE, OP, OP, NO_RM, INSTR, x86_frndint);
    register_op_sec(0xD9, 0xFD, "FSCALE", NONE, OP, OP, NO_RM, INSTR, x86_fscale);
    register_op_sec(0xD9, 0xFE, "FSIN", NONE, OP, OP, NO_RM, INSTR, x86_fsin);
    register_op_sec(0xD9, 0xFF, "FCOS", NONE, OP, OP, NO_RM, INSTR, x86_fcos);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xD9, 0xC0+i, "FLD", NONE, OP, OP, NO_RM, INSTR, x86_fld);
    register_op_ext(0xDA, 0, "FIADD", NONE, OP, OP, USE_RM, INSTR, x86_fiadd);
    register_op_ext(0xDA, 1, "FIMUL", NONE, OP, OP, USE_RM, INSTR, x86_fimul);
    register_op_ext(0xDA, 2, "FICOM", NONE, OP, OP, USE_RM, INSTR, x86_ficom);
    register_op_ext(0xDA, 3, "FICOMP", NONE, OP, OP, USE_RM, INSTR, x86_ficomp);
    register_op_ext(0xDA, 4, "FISUB", NONE, OP, OP, USE_RM, INSTR, x86_fisub);
    register_op_ext(0xDA, 5, "FISUBR", NONE, OP, OP, USE_RM, INSTR, x86_fisubr);
    register_op_ext(0xDA, 6, "FIDIV", NONE, OP, OP, USE_RM, INSTR, x86_fidiv);
    register_op_ext(0xDA, 7, "FIDIVR", NONE, OP, OP, USE_RM, INSTR, x86_fdivir);
    register_op_sec(0xDA, 0xE9, "FUCOMPP", NONE, OP, OP, USE_RM, INSTR, x86_fucompp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDA, 0xC0+i, "FCMOVB", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDA, 0xC8+i, "FCMOVE", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDA, 0xD0+i, "FCMOVBE", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDA, 0xD8+i, "FCMOVU", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);

    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xC0+i, "FCMOVNB", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xC8+i, "FCMOVNE", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xD0+i, "FCMOVNBE", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xD8+i, "FCMOVNU", NONE, OP, OP, NO_RM, INSTR, x86_fcmovcc);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xE8+i, "FUCOMI", NONE, OP, OP, NO_RM, INSTR, x86_fucomi);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDB, 0xF0+i, "FCOMI", NONE, OP, OP, NO_RM, INSTR, x86_fcomi);
    register_op_ext(0xDB, 0, "FILD", NONE, OP, OP, USE_RM, INSTR, x86_fild);
    register_op_ext(0xDB, 1, "FISTTP", NONE, OP, OP, USE_RM, INSTR, x86_fisttp);
    register_op_ext(0xDB, 2, "FIST", NONE, OP, OP, USE_RM, INSTR, x86_fist);
    register_op_ext(0xDB, 3, "FISTP", NONE, OP, OP, USE_RM, INSTR, x86_fistp);
    register_op_ext(0xDB, 5, "FLD", NONE, OP, OP, USE_RM, INSTR, x86_fld);
    register_op_ext(0xDB, 7, "FSTP", NONE, OP, OP, USE_RM, INSTR, x86_fstp);
    register_op_sec(0xDB, 0xE2, "FNCLEX", NONE, OP, OP, NO_RM, INSTR, x86_fnclex);
    register_op_prefix_sec(0x9B, 0xDB, 0xE2, "FCLEX", NONE, OP, OP, NO_RM, INSTR, x86_fclex);
    register_op_sec(0xDB, 0xE3, "FININIT", NONE, OP, OP, NO_RM, INSTR, x86_fininit);
    register_op_prefix_sec(0x9B, 0xDB, 0xE3, "FINIT", NONE, OP, OP, NO_RM, INSTR, x86_finit);

    register_op_ext(0xDC, 0, "FADD", NONE, OP, OP, USE_RM, INSTR, x86_fadd);
    register_op_ext(0xDC, 1, "FMUL", NONE, OP, OP, USE_RM, INSTR, x86_fmul);
    register_op_ext(0xDC, 2, "FCOM", NONE, OP, OP, USE_RM, INSTR, x86_fcom);
    register_op_ext(0xDC, 3, "FCOMP", NONE, OP, OP, USE_RM, INSTR, x86_fcomp);
    register_op_ext(0xDC, 4, "FSUB", NONE, OP, OP, USE_RM, INSTR, x86_fsub);
    register_op_ext(0xDC, 5, "FSUBR", NONE, OP, OP, USE_RM, INSTR, x86_fsubr);
    register_op_ext(0xDC, 6, "FDIV", NONE, OP, OP, USE_RM, INSTR, x86_fdiv);
    register_op_ext(0xDC, 7, "FDIVR", NONE, OP, OP, USE_RM, INSTR, x86_fdivr);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xC0 + i, "FADD", NONE, OP, OP, NO_RM, INSTR, x86_fadd);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xC8 + i, "FMUL", NONE, OP, OP, NO_RM, INSTR, x86_fmul);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xE0 + i, "FSUBR", NONE, OP, OP, NO_RM, INSTR, x86_fsubr);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xE8 + i, "FSUB", NONE, OP, OP, NO_RM, INSTR, x86_fsub);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xF0 + i, "FDIVR", NONE, OP, OP, NO_RM, INSTR, x86_fdivr);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDC, 0xF8 + i, "FDIV", NONE, OP, OP, NO_RM, INSTR, x86_fdiv);

    register_op_ext(0xDD, 0, "FLD", NONE, OP, OP, USE_RM, INSTR, x86_fld);
    register_op_ext(0xDD, 1, "FISTTP", NONE, OP, OP, USE_RM, INSTR, x86_fisttp);
    register_op_ext(0xDD, 2, "FST", NONE, OP, OP, USE_RM, INSTR, x86_fst);
    register_op_ext(0xDD, 3, "FSTP", NONE, OP, OP, USE_RM, INSTR, x86_fstp);
    register_op_ext(0xDD, 4, "FRSTOR", NONE, OP, OP, USE_RM, INSTR, x86_frstor);
    register_op_ext(0xDD, 6, "FNSAVE", NONE, OP, OP, USE_RM, INSTR, x86_fnsave);
    register_op_prefix_ext(0x9B, 0xDD, 6, "FSAVE", NONE, OP, OP, USE_RM, INSTR, x86_fsave);
    register_op_ext(0xDD, 7, "FNSTSW", NONE, OP, OP, USE_RM, INSTR, x86_fnstsw);
    register_op_prefix_ext(0x9B, 0xDD, 7, "FSTSW", NONE, OP, OP, USE_RM, INSTR, x86_fstsw);
    register_op_sec(0xDD, 0xE1, "FUCOM", NONE, OP, OP, NO_RM, INSTR, x86_fucom);
    register_op_sec(0xDD, 0xE9, "FUCOMP", NONE, OP, OP, NO_RM, INSTR, x86_fucomp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDD, 0xC0 + i, "FFREE", NONE, OP, OP, NO_RM, INSTR, x86_ffree);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDD, 0xD0 + i, "FST", NONE, OP, OP, NO_RM, INSTR, x86_fst);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDD, 0xD8 + i, "FSTP", NONE, OP, OP, NO_RM, INSTR, x86_fstp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDD, 0xE0 + i, "FUCOM", NONE, OP, OP, NO_RM, INSTR, x86_fucom);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDD, 0xE8 + i, "FUCOMP", NONE, OP, OP, NO_RM, INSTR, x86_fucomp);

    register_op_ext(0xDE, 0, "FIADD", NONE, OP, OP, USE_RM, INSTR, x86_fiadd);
    register_op_ext(0xDE, 1, "FIMUL", NONE, OP, OP, USE_RM, INSTR, x86_fimul);
    register_op_ext(0xDE, 2, "FICOM", NONE, OP, OP, USE_RM, INSTR, x86_ficom);
    register_op_ext(0xDE, 3, "FICOMP", NONE, OP, OP, USE_RM, INSTR, x86_ficomp);
    register_op_ext(0xDE, 4, "FISUB", NONE, OP, OP, USE_RM, INSTR, x86_fisub);
    register_op_ext(0xDE, 5, "FISUBR", NONE, OP, OP, USE_RM, INSTR, x86_fisubr);
    register_op_ext(0xDE, 6, "FIDIV", NONE, OP, OP, USE_RM, INSTR, x86_fidiv);
    register_op_ext(0xDE, 7, "FIDIVR", NONE, OP, OP, USE_RM, INSTR, x86_fdivir);
    register_op_sec(0xDE, 0xC1, "FADDP", NONE, OP, OP, NO_RM, INSTR, x86_faddp);
    register_op_sec(0xDE, 0xD9, "FCOMPP", NONE, OP, OP, NO_RM, INSTR, x86_fcompp);
    register_op_sec(0xDE, 0xE1, "FSUBRP", NONE, OP, OP, NO_RM, INSTR, x86_fsubrp);
    register_op_sec(0xDE, 0xE9, "FSUBP", NONE, OP, OP, NO_RM, INSTR, x86_fsubp);
    register_op_sec(0xDE, 0xF1, "FDIVRP", NONE, OP, OP, NO_RM, INSTR, x86_fdivrp);
    register_op_sec(0xDE, 0xF9, "FDIVP", NONE, OP, OP, NO_RM, INSTR, x86_fdivp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xC0 + i, "FADDP", NONE, OP, OP, NO_RM, INSTR, x86_faddp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xC8 + i, "FMULP", NONE, OP, OP, NO_RM, INSTR, x86_fmulp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xE0 + i, "FSUBRP", NONE, OP, OP, NO_RM, INSTR, x86_fsubrp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xE8 + i, "FSUBP", NONE, OP, OP, NO_RM, INSTR, x86_fsubp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xF0 + i, "FDIVRP", NONE, OP, OP, NO_RM, INSTR, x86_fdivrp);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDE, 0xF8 + i, "FDIVP", NONE, OP, OP, NO_RM, INSTR, x86_fdivp);

    register_op_ext(0xDF, 0, "FILD", NONE, OP, OP, USE_RM, INSTR, x86_fild);
    register_op_ext(0xDF, 1, "FISTTP", NONE, OP, OP, USE_RM, INSTR, x86_fisttp);
    register_op_ext(0xDF, 2, "FIST", NONE, OP, OP, USE_RM, INSTR, x86_fist);
    register_op_ext(0xDF, 3, "FISTP", NONE, OP, OP, USE_RM, INSTR, x86_fistp);
    register_op_ext(0xDF, 4, "FBLD", NONE, OP, OP, USE_RM, INSTR, x86_fbld);
    register_op_ext(0xDF, 5, "FILD", NONE, OP, OP, USE_RM, INSTR, x86_fild);
    register_op_ext(0xDF, 6, "FBSTP", NONE, OP, OP, USE_RM, INSTR, x86_fbstp);
    register_op_ext(0xDF, 7, "FISTP", NONE, OP, OP, USE_RM, INSTR, x86_fistp);
    register_op_sec(0xDF, 0xE0, "FNSTSW", NONE, OP, OP, NO_RM, INSTR, x86_fnstsw);
    register_op_prefix_sec(0x9B, 0xDF, 0xE0, "FSTSW", NONE, OP, OP, NO_RM, INSTR, x86_fstsw);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDF, 0xE8+i, "FUCOMIP", NONE, OP, OP, NO_RM, INSTR, x86_fucomip);
    for (size_t i = 0; i < 8; i++)
        register_op_sec(0xDF, 0xF0+i, "FCOMIP", NONE, OP, OP, NO_RM, INSTR, x86_fcomip);

    register_op(0xE0, "LOOPNE",  NONE, rela8, rela8, NO_RM, INSTR, x86_loopcc);
    register_op(0xE1, "LOOPE",  NONE, rela8, rela8, NO_RM, INSTR, x86_loopcc);
    register_op(0xE2, "LOOP",  NONE, rela8, rela8, NO_RM, INSTR, x86_loopcc);
    register_op(0xE3, "JCXZ",  NONE, rela8, rela8, NO_RM, INSTR, x86_jcc);
    register_op(0xE4, "IN", NONE, AL_imm8, AL_imm8, NO_RM, INSTR, x86_in);
    register_op(0xE5, "IN", NONE, eAX_imm8, AX_imm8, NO_RM, INSTR, x86_in);
    register_op(0xE6, "OUT", NONE, imm8_AL, imm8_AL, NO_RM, INSTR, x86_out);
    register_op(0xE7, "OUT", NONE, imm8_eAX, imm8_AX, NO_RM, INSTR, x86_out);
    register_op(0xE8, "CALL", NONE, rela32, rela16, NO_RM, INSTR, x86_mm_call);
    register_op(0xE9, "JMP", NONE, rela32, rela16, NO_RM, INSTR, x86_jmp);
    register_op(0xEA, "JMP", NONE, ptr16_32, ptr16_16, NO_RM, INSTR, x86_jmp);
    register_op(0xEB, "JMP", NONE, rela8, rela8, NO_RM, INSTR, x86_jmp);
    register_op(0xEC, "IN", NONE, OP, OP, NO_RM, INSTR, x86_in);
    register_op(0xED, "IN", NONE, OP, OP, NO_RM, INSTR, x86_in);
    register_op(0xEE, "OUT", NONE, OP, OP, NO_RM, INSTR, x86_out);
    register_op(0xEF, "OUT", NONE, OP, OP, NO_RM, INSTR, x86_out);
    register_op(0xF0, NULL, NONE, no_encoding, no_encoding, NO_RM, PREFIX, NULL);
    register_op(0xF1, "INT1", NONE, OP, OP, NO_RM, INSTR, x86_int1);
    register_op(0xF2, NULL, NONE, no_encoding, no_encoding, NO_RM, PREFIX, NULL);
    register_op(0xF3, NULL, NONE, no_encoding, no_encoding, NO_RM, PREFIX, NULL);
    register_op_sec(0xF3, 0x90, "PAUSE", NONE, OP, OP, NO_RM, INSTR, x86_pause);
    register_op(0xF4, "HLT", NONE, OP, OP, NO_RM, INSTR, x86_hlt);
    register_op(0xF5, "CMC", NONE, OP, OP, NO_RM, INSTR, x86_cmc);
    register_op_ext(0xF6, 0, "TEST", NONE, rm8_imm8, rm8_imm8, USE_RM, INSTR, x86_test);
    register_op_ext(0xF6, 2, "NOT", NONE, rm8, rm8, USE_RM, INSTR, x86_not);
    register_op_ext(0xF6, 3, "NEG", NONE, rm8, rm8, USE_RM, INSTR, x86_neg);
    register_op_ext(0xF6, 4, "MUL", NONE, rm8, rm8, USE_RM, INSTR, x86_mul);
    register_op_ext(0xF6, 5, "IMUL", NONE, rm8, rm8, USE_RM, INSTR, x86_imul);
    register_op_ext(0xF6, 6, "DIV", NONE, rm8, rm8, USE_RM, INSTR, x86_div);
    register_op_ext(0xF6, 7, "IDIV", NONE, rm8, rm8, USE_RM, INSTR, x86_idiv);
    register_op_ext(0xF7, 0, "NOT", NONE, rm32_imm32, rm16_imm16, USE_RM, INSTR, x86_test);
    register_op_ext(0xF7, 2, "NOT", NONE, rm32, rm16, USE_RM, INSTR, x86_not);
    register_op_ext(0xF7, 3, "NEG", NONE, rm32, rm16, USE_RM, INSTR, x86_neg);
    register_op_ext(0xF7, 4, "MUL", NONE, rm32, rm16, USE_RM, INSTR, x86_mul);
    register_op_ext(0xF7, 5, "IMUL", NONE, rm32, rm16, USE_RM, INSTR, x86_imul);
    register_op_ext(0xF7, 6, "DIV", NONE, rm32, rm16, USE_RM, INSTR, x86_div);
    register_op_ext(0xF7, 7, "IDIV", NONE, rm32, rm16, USE_RM, INSTR, x86_idiv);
    register_op(0xF8, "CLC", NONE, OP, OP, NO_RM, INSTR, x86_clc);
    register_op(0xF9, "STC", NONE, OP, OP, NO_RM, INSTR, x86_stc);
    register_op(0xFA, "CLI", NONE, OP, OP, NO_RM, INSTR, x86_cli);
    register_op(0xFB, "STI", NONE, OP, OP, NO_RM, INSTR, x86_sti);
    register_op(0xFC, "CLD", NONE, OP, OP, NO_RM, INSTR, x86_cld);
    register_op(0xFD, "STD", NONE, OP, OP, NO_RM, INSTR, x86_std);
    register_op_ext(0xFE, 0, "INC", NONE, rm8, rm8, USE_RM, INSTR, x86_inc);
    register_op_ext(0xFE, 1, "DEC", NONE, rm8, rm8, USE_RM, INSTR, x86_dec);
    register_op_ext(0xFF, 0, "INC", NONE, rm32, rm16, USE_RM, INSTR, x86_inc);
    register_op_ext(0xFF, 1, "DEC", NONE, rm32, rm16, USE_RM, INSTR, x86_dec);
    register_op_ext(0xFF, 2, "CALL", NONE, rm32, rm16, USE_RM, INSTR, x86_mm_call);
    register_op_ext(0xFF, 3, "CALL", NONE, m16_32, m16_16, USE_RM, INSTR, x86_mm_call);
    register_op_ext(0xFF, 4, "JMP", NONE, rm32, rm16, USE_RM, INSTR, x86_jmp);
    register_op_ext(0xFF, 5, "JMP", NONE, rm32, rm16, USE_RM, INSTR, x86_jmp);
    register_op_ext(0xFF, 6, "PUSH", NONE, rm32, rm16, USE_RM, INSTR, x86_mm_push);


    // 0f two-byte instructions

    register_0f_op_ext(0x00, 0, "SLDT", NONE, rm16, rm16, USE_RM, INSTR, x86_sldt);
    register_0f_op_ext(0x00, 1, "STR", NONE, rm16, rm16, USE_RM, INSTR, x86_str);
    register_0f_op_ext(0x00, 2, "LLDT", NONE, rm16, rm16, USE_RM, INSTR, x86_lldt);
    register_0f_op_ext(0x00, 3, "LTR", NONE, rm16, rm16, USE_RM, INSTR, x86_ltr);
    register_0f_op_ext(0x00, 4, "VERR", NONE, rm16, rm16, USE_RM, INSTR, x86_verr);
    register_0f_op_ext(0x00, 5, "VERW", NONE, rm16, rm16, USE_RM, INSTR, x86_verw);

    register_0f_op_ext(0x01, 0, "SGDT", NONE, rm32, rm32, USE_RM, INSTR, x86_sgdt);
    register_0f_op_ext(0x01, 1, "SIDT", NONE, rm32, rm32, USE_RM, INSTR, x86_sidt);
    register_0f_op_ext(0x01, 2, "LGDT", NONE, m16_32, m16_32, USE_RM, INSTR, x86_lgdt);
    register_0f_op_ext(0x01, 3, "LIDT", NONE, m16_32, m16_32, USE_RM, INSTR, x86_lidt);
    register_0f_op_ext(0x01, 4, "SMSW", NONE, r32m16, rm16, USE_RM, INSTR, x86_smsw);
    register_0f_op_ext(0x01, 6, "LMSW", NONE, rm16, rm16, USE_RM, INSTR, x86_lmsw);
    register_0f_op_ext(0x01, 7, "INVLPG", NONE, OP, OP, USE_RM, INSTR, x86_invlpg);
    register_0f_op_sec(0x01, 0xC8, "MONITOR", NONE, OP, OP, NO_RM, INSTR, x86_monitor);
    register_0f_op_sec(0x01, 0xC9, "MWAIT", NONE, OP, OP, NO_RM, INSTR, x86_mwait);
    register_0f_op_sec(0x01, 0xCA, "CLAC", SMAP, OP, OP, NO_RM, INSTR, x86_clac);
    register_0f_op_sec(0x01, 0xCB, "STAC", SMAP, OP, OP, NO_RM, INSTR, x86_stac);
    register_0f_op_sec(0x01, 0xD0, "XGETBV", NONE, OP, OP, NO_RM, INSTR, x86_xgetbv);
    register_0f_op_sec(0x01, 0xD1, "XSETBV", NONE, OP, OP, NO_RM, INSTR, x86_xsetbv);
    register_0f_op_sec(0x01, 0xD5, "XEND", RTM, OP, OP, NO_RM, INSTR, x86_xend);
    register_0f_op_sec(0x01, 0xD6, "XTEST", RTM, OP, OP, NO_RM, INSTR, x86_xtest);
    register_0f_op_sec(0x01, 0xEE, "RDPKRU", OSPKE, OP, OP, NO_RM, INSTR, x86_rdpkru);
    register_0f_op_sec(0x01, 0xEF, "WRPKRU", OSPKE, OP, OP, NO_RM, INSTR, x86_wrpkru);
    register_0f_op_sec(0x01, 0xF8, "SWAPGS", OSPKE, OP, OP, NO_RM, INSTR, x86_swapgs);
    register_0f_op_sec(0x01, 0xF9, "RDTSCP", OSPKE, OP, OP, NO_RM, INSTR, x86_rdtscp);
    register_0f_op(0x02, "LAR", NONE, r32_r32m16, r16_r16m16, USE_RM, INSTR, x86_lar);
    register_0f_op(0x03, "LSL", NONE, r32_r32m16, r16_r16m16, USE_RM, INSTR, x86_lsl);

    register_0f_op(0x05, "SYSCALL", NONE, OP, OP, NO_RM, INSTR, x86_syscall);
    register_0f_op(0x06, "CLTS", NONE, OP, OP, NO_RM, INSTR, x86_clts);
    register_0f_op(0x07, "SYSRET", NONE, OP, OP, NO_RM, INSTR, x86_sysret);

    register_0f_op(0x08, "INVD", NONE, OP, OP, NO_RM, INSTR, x86_invd);
    register_0f_op(0x09, "WBINVD", NONE, OP, OP, NO_RM, INSTR, x86_wbinvd);

    register_0f_op(0x0B, "UD2", NONE, OP, OP, NO_RM, INSTR, x86_ud2);

    register_0f_op_ext(0x0D, 1, "PREFETCHW", NONE, m8, m8, USE_RM, INSTR, x86_prefetchw);

    register_0f_op(0x10, "MOVUPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, NO_RM, INSTR, x86_movups);
    register_0f_op_prefix(0x66, 0x10, "MOVUPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movupd);
    register_0f_op_prefix(0xF2, 0x10, "MOVSD", SSE2, xmm1_m64, xmm1_xmm2, USE_RM, INSTR, x86_movsd);
    register_0f_op_prefix(0xF3, 0x10, "MOVSS", SSE, xmm1_m32, xmm1_xmm2, USE_RM, INSTR, x86_movss);
    register_0f_op(0x11, "MOVUPS", SSE, xmm2m128_xmm1, xmm2m128_xmm1, NO_RM, INSTR, x86_movups);
    register_0f_op_prefix(0x66, 0x11, "MOVUPD", SSE2, xmm2m128_xmm1, xmm2m128_xmm1, USE_RM, INSTR, x86_movupd);
    register_0f_op_prefix(0xF2, 0x11, "MOVSD", SSE, xmm1m64_xmm2, xmm1m64_xmm2, USE_RM, INSTR, x86_movsd);
    register_0f_op_prefix(0xF3, 0x11, "MOVSS", SSE, xmm2m32_xmm1, xmm2m32_xmm1, USE_RM, INSTR, x86_movss);
    register_0f_op_prefix(0x66, 0x12, "MOVLPD", SSE2, xmm1_m64, xmm1_m64, USE_RM, INSTR, x86_movlpd);
    register_0f_op(0x12, "MOVHLPS", SSE, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_movhlps);
    register_0f_op_prefix(0xF2, 0x12, "MOVDDUP", SSE3, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_movddup);
    register_0f_op_prefix(0xF3, 0x12, "MOVSLDUP", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movsldup);
    register_0f_op(0x13, "MOVLPS", SSE, m64_xmm1, m64_xmm1, USE_RM, INSTR, x86_movlps);
    register_0f_op_prefix(0x66, 0x13, "MOVLPD", SSE2, m64_xmm1, m64_xmm1, USE_RM, INSTR, x86_movlpd);
    register_0f_op(0x14, "UNPCKLPS", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_unpcklps);
    register_0f_op_prefix(0x66, 0x14, "UNPCKLPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_unpcklpd);
    register_0f_op(0x15, "UNPCKHPS", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_unpckhps);
    register_0f_op_prefix(0x66, 0x15, "UNPCKHPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_unpckhpd);
    register_0f_op(0x16, "MOVHPS", SSE, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_movhps);
    register_0f_op_prefix(0x66, 0x16, "MOVHPD", SSE2, xmm1_m64, xmm1_m64, USE_RM, INSTR, x86_movhpd);
    register_0f_op_prefix(0xF3, 0x16, "MOVSHDUP", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movshdup);

    register_0f_op_ext(0x18, 0, "PREFETCHNTA", NONE, m8, m8, USE_RM, INSTR, x86_prefetchnta);
    register_0f_op_ext(0x18, 1, "PREFETCHT0", NONE, m8, m8, USE_RM, INSTR, x86_prefetcht0);
    register_0f_op_ext(0x18, 2, "PREFETCHT1", NONE, m8, m8, USE_RM, INSTR, x86_prefetcht1);
    register_0f_op_ext(0x18, 3, "PREFETCHT2", NONE, m8, m8, USE_RM, INSTR, x86_prefetcht2);
    register_0f_op_prefix(0x66, 0x17, "MOVHPD", NONE, m64_xmm1, m64_xmm1, USE_RM, INSTR, x86_movhpd);
    register_0f_op(0x1A, "BNDLDX", MPX, bnd_sib, bnd_sib, NO_RM, INSTR, x86_bndldx);
    register_0f_op_prefix(0x66, 0x1A, "BNDMOV", MPX, bnd1_bnd2m64, bnd1_bnd2m64, USE_RM, INSTR, x86_bndmov);
    register_0f_op_prefix(0xF2, 0x1A, "BNDCU", MPX, bnd_rm32, bnd_rm32, USE_RM, INSTR, x86_bndcu);
    register_0f_op_prefix(0xF3, 0x1A, "BNDCL", MPX, bnd_rm32, bnd_rm32, USE_RM, INSTR, x86_bndcl);
    register_0f_op(0x1B, "BNDSTX", MPX, sib_bnd, sib_bnd, NO_RM, INSTR, x86_bndstx);
    register_0f_op_prefix(0x66, 0x1B, "BNDMOV", MPX, bnd1m64_bnd2, bnd1m64_bnd2, USE_RM, INSTR, x86_bndmov);
    register_0f_op_prefix(0xF2, 0x1B, "BNDCN", MPX, bnd_rm32, bnd_rm32, USE_RM, INSTR, x86_bndcn);
    register_0f_op_prefix(0xF3, 0x1B, "BNDMK", MPX, bnd_rm32, bnd_rm32, USE_RM, INSTR, x86_bndmk);

    register_0f_op_prefix_sec(0xF3, 0x1E, 0xfB, "ENDBR32", NONE, OP, OP, USE_RM, INSTR, x86_mm_endbr32);
    register_0f_op_ext(0x1F, 0, "NOP", NONE, rm32, rm16, NO_RM, INSTR, x86_mm_nop);

    register_0f_op(0x20, "MOV", NONE, rm32_r32, rm32_r32, NO_RM, INSTR, x86_mm_mov);
    register_0f_op(0x21, "MOV", NONE, rm32_r32, rm32_r32, NO_RM, INSTR, x86_mm_mov);
    register_0f_op(0x22, "MOV", NONE, r32_rm32, r32_rm32, NO_RM, INSTR, x86_mm_mov);
    register_0f_op(0x23, "MOV", NONE, r32_rm32, r32_rm32, NO_RM, INSTR, x86_mm_mov);

    register_0f_op(0x28, "MOVAPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, NO_RM, INSTR, x86_movaps);
    register_0f_op_prefix(0x66, 0x28, "MOVAPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movapd);
    register_0f_op(0x29, "MOVAPS", SSE, xmm2m128_xmm1, xmm2m128_xmm1, NO_RM, INSTR, x86_movaps);
    register_0f_op_prefix(0x66, 0x29, "MOVAPD", SSE, xmm2m128_xmm1, xmm2m128_xmm1, USE_RM, INSTR, x86_movapd);

    register_0f_op(0x2A, "CVTPI2PS", NONE, xmm1_mm1m64, xmm1_mm1m64, USE_RM, INSTR, x86_cvtpi2ps);
    register_0f_op_prefix(0x66, 0x2A, "CVTPI2PD", NONE, xmm1_mm1m64, xmm1_mm1m64, USE_RM, INSTR, x86_cvtpi2pd);
    register_0f_op_prefix(0xF2, 0x2A, "CVTSI2SD", SSE2, xmm1_r32m32, xmm1_r32m32, USE_RM, INSTR, x86_cvtsi2sd);
    register_0f_op_prefix(0xF3, 0x2A, "CVTSI2SS", SSE, xmm1_rm32, xmm1_rm32, USE_RM, INSTR, x86_cvtsi2ss);
    register_0f_op(0x2B, "MOVNTPS", SSE, m128_xmm1, m128_xmm1, USE_RM, INSTR, x86_movntps);
    register_0f_op_prefix(0x66, 0x2B, "MOVNTPD", SSE2, m128_xmm1, m128_xmm1, USE_RM, INSTR, x86_movntpd);
    register_0f_op(0x2C, "CVTTPS2PI", NONE, mm_xmm1m64, mm_xmm1m64, USE_RM, INSTR, x86_cvttps2pi);
    register_0f_op_prefix(0x66, 0x2C, "CVTTPD2PI", NONE, mm_xmm1m128, mm_xmm1m128, USE_RM, INSTR, x86_cvttpd2pi);
    register_0f_op_prefix(0xF2, 0x2C, "CVTTSD2SI", SSE2, r32_xmm1m64, r32_xmm1m64, USE_RM, INSTR, x86_cvttsd2si);
    register_0f_op_prefix(0xF3, 0x2C, "CVTTSS2SI", SSE, r32_xmm1m32, r32_xmm1m32, USE_RM, INSTR, x86_cvttss2si);
    register_0f_op(0x2D, "CVTPS2PI", NONE, mm_xmm1m64, mm_xmm1m64, USE_RM, INSTR, x86_cvtps2pi);
    register_0f_op_prefix(0x66, 0x2D, "CVTPD2PI", NONE, mm_xmm1m128, mm_xmm1m128, USE_RM, INSTR, x86_cvtpd2pi);
    register_0f_op_prefix(0xF2, 0x2D, "CVTSD2SI", SSE2, r32_xmm1m64, r32_xmm1m64, USE_RM, INSTR, x86_cvtsd2si);
    register_0f_op_prefix(0xF3, 0x2D, "CVTSS2SI", SSE, r32_xmm1m32, r32_xmm1m32, USE_RM, INSTR, x86_cvtss2si);
    register_0f_op(0x2E, "UCOMISS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_ucomiss);
    register_0f_op_prefix(0x66, 0x2E, "UCOMISD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_ucomisd);
    register_0f_op(0x2F, "COMISS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_comiss);
    register_0f_op_prefix(0x66, 0x2F, "COMISD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_comisd);
    register_0f_op(0x30, "WRMSR", NONE, OP, OP, NO_RM, INSTR, x86_wrmsr);
    register_0f_op(0x31, "RDTSC", NONE, OP, OP, NO_RM, INSTR, x86_rdtsc);
    register_0f_op(0x32, "RDMSR", NONE, OP, OP, NO_RM, INSTR, x86_rdmsr);
    register_0f_op(0x33, "RDPMC", NONE, OP, OP, NO_RM, INSTR, x86_rdpmc);
    register_0f_op(0x34, "SYSENTER", SEP, OP, OP, NO_RM, INSTR, x86_sysenter);
    register_0f_op(0x35, "SYSEXIT", SEP, OP, OP, NO_RM, INSTR, x86_sysexit);

    register_0f_op_sec(0x38, 0x00, "PSHUFB", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pshufb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x00, "PSHUFB", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pshufb);
    register_0f_op_sec(0x38, 0x01, "PHADDW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phaddw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x01, "PHADDW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phaddw);
    register_0f_op_sec(0x38, 0x02, "PHADDD", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phaddd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x02, "PHADDD", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phaddd);
    register_0f_op_sec(0x38, 0x03, "PHADDSW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phaddsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x03, "PHADDSW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phaddsw);
    register_0f_op_sec(0x38, 0x04, "PMADDUBSW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmaddubsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x04, "PMADDUBSW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaddubsw);
    register_0f_op_sec(0x38, 0x05, "PHSUBW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phsubw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x05, "PHSUBW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phsubw);
    register_0f_op_sec(0x38, 0x05, "PHSUBW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phsubw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x05, "PHSUBW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phsubw);
    register_0f_op_sec(0x38, 0x06, "PHSUBD", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phsubd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x06, "PHSUBD", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phsubd);
    register_0f_op_sec(0x38, 0x07, "PHSUBSW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_phsubsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x07, "PHSUBSW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phsubsw);
    register_0f_op_sec(0x38, 0x08, "PSIGNB", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psignb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x08, "PSIGNB", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psignb);
    register_0f_op_sec(0x38, 0x09, "PSIGNW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psignw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x09, "PSIGNW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psignw);
    register_0f_op_sec(0x38, 0x0A, "PSIGND", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psignw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x0A, "PSIGND", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psignd);
    register_0f_op_sec(0x38, 0x0B, "PMULHRSW", SSSE3, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmulhrsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x0B, "PMULHRSW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmulhrsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x10, "PBLENDVB", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pblendvb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x14, "BLENDVPS", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_blendvps);
    register_0f_op_prefix_sec(0x66, 0x38, 0x15, "BLENDVPD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_blendvpd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x17, "PTEST", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_ptest);
    register_0f_op_sec(0x38, 0x1C, "PABSB", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pabsb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x1C, "PABSB", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pabsb);
    register_0f_op_sec(0x38, 0x1D, "PABSW", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pabsw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x1D, "PABSW", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pabsw);
    register_0f_op_sec(0x38, 0x1E, "PABSD", SSSE3, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pabsd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x1E, "PABSD", SSSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pabsd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x20, "PMOVSXBW", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x21, "PMOVSXBD", SSE4_1, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x22, "PMOVSXBQ", SSE4_1, xmm1_xmm2m16, xmm1_xmm2m16, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x23, "PMOVSXWD", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x24, "PMOVSXWQ", SSE4_1, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x25, "PMOVSXDQ", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovsx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x28, "PMULDQ", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmuldq);
    register_0f_op_prefix_sec(0x66, 0x38, 0x29, "PCMPEQQ", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpeqq);
    register_0f_op_prefix_sec(0x66, 0x38, 0x2A, "MOVNTDQA", SSE4_1, xmm1_m128, xmm1_m128, USE_RM, INSTR, x86_movntdqa);
    register_0f_op_prefix_sec(0x66, 0x38, 0x2B, "PACKUSDW", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_packusdw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x30, "PMOVZXBW", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x31, "PMOVZXBD", SSE4_1, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x32, "PMOVZXBQ", SSE4_1, xmm1_xmm2m16, xmm1_xmm2m16, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x33, "PMOVZXWD", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x34, "PMOVZXWQ", SSE4_1, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x35, "PMOVZXDQ", SSE4_1, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_pmovzx);
    register_0f_op_prefix_sec(0x66, 0x38, 0x37, "PCMPGTQ", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpgtq);
    register_0f_op_prefix_sec(0x66, 0x38, 0x38, "PMINSB", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminsb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x39, "PMINSD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminsd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3A, "PMINSUW", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminuw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3B, "PMINUD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminud);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3C, "PMAXSB", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxsb);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3D, "PMAXSD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxsd);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3E, "PMAXUW", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxuw);
    register_0f_op_prefix_sec(0x66, 0x38, 0x3F, "PMAXUD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxud);
    register_0f_op_prefix_sec(0x66, 0x38, 0x40, "PMULLLD", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmulld);
    register_0f_op_prefix_sec(0x66, 0x38, 0x41, "PHMINPOSUW", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_phminposuw);
    register_0f_op_prefix_sec(0x66, 0x38, 0xC8, "SHA1NEXTE", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha1nexte);
    register_0f_op_prefix_sec(0x66, 0x38, 0xC9, "SHA1MSG1", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha1msg1);
    register_0f_op_prefix_sec(0x66, 0x38, 0xCA, "SHA1MSG2", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha1msg2);
    register_0f_op_prefix_sec(0x66, 0x38, 0xCB, "SHA256RNDS2", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha256rnds2);
    register_0f_op_prefix_sec(0x66, 0x38, 0xCC, "SHA256MSG1", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha256msg1);
    register_0f_op_prefix_sec(0x66, 0x38, 0xCD, "SHA256MSG2", SHA, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sha256msg2);
    register_0f_op_prefix_sec(0x66, 0x38, 0xDB, "AESIMC", AES, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_aesimc);
    register_0f_op_prefix_sec(0x66, 0x38, 0xDC, "AESENC", AES, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_aesenc);
    register_0f_op_prefix_sec(0x66, 0x38, 0xDD, "AESENCLAST", AES, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_aesenclast);
    register_0f_op_prefix_sec(0x66, 0x38, 0xDE, "AESDEC", AES, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_aesdec);
    register_0f_op_prefix_sec(0x66, 0x38, 0xDF, "AESDECLAST", AES, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_aesdeclast);
    register_0f_op_sec(0x38, 0xF0, "MOVBE", NONE, r32_m32, r16_m16, USE_RM, INSTR, x86_movbe);
    register_0f_op_prefix_sec(0xF2, 0x38, 0xF0, "CRC32", NONE, r32_rm8, r32_rm8, USE_RM, INSTR, x86_crc32);
    register_0f_op_sec(0x38, 0xF1, "MOVBE", NONE, r32_m32, r16_m16, USE_RM, INSTR, x86_movbe);
    register_0f_op_prefix_sec(0xF2, 0x38, 0xF1, "CRC32", NONE, r32_rm32, r32_rm16, USE_RM, INSTR, x86_crc32);
    register_0f_op_prefix_sec(0x66, 0x38, 0xF6, "ADCX", ADX, r32_rm32, r32_rm16, USE_RM, INSTR, x86_adcx);
    register_0f_op_prefix_sec(0xF3, 0x38, 0xF6, "ADOX", ADX, r32_rm32, r32_rm16, USE_RM, INSTR, x86_adox);

    register_0f_op_prefix_sec(0x66, 0x3A, 0x08, "ROUNDPS", SSE4_1, xmm1_xmm2m128_imm8,xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_roundps);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x09, "ROUNDPD", SSE4_1, xmm1_xmm2m128_imm8,xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_roundpd);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0A, "ROUNDSS", SSE4_1, xmm1_xmm2m32_imm8,xmm1_xmm2m32_imm8, USE_RM, INSTR, x86_roundss);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0B, "ROUNDSD", SSE4_1, xmm1_xmm2m64_imm8,xmm1_xmm2m64_imm8, USE_RM, INSTR, x86_roundsd);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0C, "BLENDPS", SSE4_1, xmm1_xmm2m128_imm8,xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_blendps);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0D, "BLENDPD", SSE4_1, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_blendpd);
    register_0f_op_sec(0x3A, 0x0F, "PALIGNR", SSSE3, mm1_mm2m64_imm8, mm1_mm2m64_imm8, USE_RM, INSTR, x86_palignr);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0E, "PBLENDW", SSE4_1, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pblendw);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x0F, "PALIGNR", SSSE3, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_palignr);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x14, "PEXTRB", SSE4_1, rm8_xmm2_imm8, rm8_xmm2_imm8, USE_RM, INSTR, x86_pextrb);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x15, "PEXTRW", SSE2, rm16_xmm1_imm8, rm16_xmm1_imm8, USE_RM, INSTR, x86_pextrw);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x16, "PEXTRD", SSE4_1, rm32_xmm2_imm8, rm32_xmm2_imm8, USE_RM, INSTR, x86_pextrd);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x17, "EXTRACTPS", SSE4_1, rm32_xmm1_imm8, rm32_xmm1_imm8, USE_RM, INSTR, x86_extractps);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x20, "PINSRB", SSE4_1, xmm1_r32m8_imm8, xmm1_r32m8_imm8, USE_RM, INSTR, x86_pinsrb);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x22, "PINSRD", SSE4_1, xmm1_r32m8_imm8, xmm1_r32m8_imm8, USE_RM, INSTR, x86_pinsrd);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x21, "INSERTPS", SSE4_1, xmm1_xmm2m32_imm8, xmm1_xmm2m32_imm8, USE_RM, INSTR, x86_insertps);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x40, "DPPS", SSE4_1, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_dpps);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x41, "DPPD", SSE4_1, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_dppd);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x42, "MPSADBW", SSE4_1, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_mpsadbw);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x44, "PCLMULQDQ", PCLMULQDQ, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pclmulqdq);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x60, "PCMPESTRM", SSE4_2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pcmpestrm);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x61, "PCMPESTRI", SSE4_2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pcmpestri);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x62, "PCMPISTRM", SSE4_2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pcmpistrm);
    register_0f_op_prefix_sec(0x66, 0x3A, 0x63, "PCMPISTRI", SSE4_2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pcmpistri);
    register_0f_op_prefix_sec(0x66, 0x3A, 0xCC, "SHA1RNDS4", SHA, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_sha1rnds4);
    register_0f_op_prefix_sec(0x66, 0x3A, 0xDF, "AESKEYGENASSIST", AES, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_aeskeygenassist);

    register_0f_op(0x40, "CMOVO", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x41, "CMOVNO", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x42, "CMOVB", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x43, "CMOVAE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x44, "CMOVE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x45, "CMOVNE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x46, "CMOVBE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x47, "CMOVA", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x48, "CMOVPO", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x49, "CMOVNS", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4A, "CMOVP", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4B, "CMOVNP", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4C, "CMOVL", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4D, "CMOVGE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4E, "CMOVLE", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x4F, "CMOVG", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_cmovcc);
    register_0f_op(0x50, "MOVMSKPS", SSE, r32_xmm, r32_xmm, USE_RM, INSTR, x86_movmskps);
    register_0f_op_prefix(0x66, 0x50, "MOVMSKPD", SSE2, r32_xmm, r32_xmm, USE_RM, INSTR, x86_movmskpd);
    register_0f_op(0x51, "SQRTPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sqrtps);
    register_0f_op_prefix(0x66, 0x51, "SQRTPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_sqrtpd);
    register_0f_op_prefix(0xF2, 0x51, "SQRTSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_sqrtsd);
    register_0f_op_prefix(0xF3, 0x51, "SQRTSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_sqrtss);
    register_0f_op(0x52, "RSQRTPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_rsqrtps);
    register_0f_op_prefix(0xF3, 0x52, "RSQRTSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_rsqrtss);
    register_0f_op(0x53, "RCPPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_rcpps);
    register_0f_op_prefix(0xF3, 0x53, "RCPSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_rcpss);
    register_0f_op(0x54, "ANDPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_andps);
    register_0f_op_prefix(0x66, 0x54, "ANDPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_andpd);
    register_0f_op(0x55, "ANDNPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_andnps);
    register_0f_op_prefix(0x66, 0x55, "ANDNPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_andnpd);
    register_0f_op(0x56, "ORPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_orps);
    register_0f_op_prefix(0x66, 0x56, "ORPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_orpd);
    register_0f_op(0x57, "XORPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_xorps);
    register_0f_op_prefix(0x66, 0x57, "XORPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_xorpd);
    register_0f_op(0x58, "ADDPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_addps);
    register_0f_op_prefix(0x66, 0x58, "ADDPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_addpd);
    register_0f_op_prefix(0xF2, 0x58, "ADDSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_addsd);
    register_0f_op_prefix(0xF3, 0x58, "ADDSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_addss);
    register_0f_op(0x59, "MULPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_mulps);
    register_0f_op_prefix(0x66, 0x59, "MULPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_mulpd);
    register_0f_op_prefix(0xF2, 0x59, "MULSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_mulsd);
    register_0f_op_prefix(0xF3, 0x59, "MULSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_mulss);
    register_0f_op_prefix(0x66, 0x5A, "CVTPS2PD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_cvtps2pd);
    register_0f_op_prefix(0xF2, 0x5A, "CVTSD2SS", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_cvtsd2ss);
    register_0f_op_prefix(0xF3, 0x5A, "CVTSS2SD", SSE2, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_cvtss2sd);
    register_0f_op(0x5B, "CVTDQ2PS", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvtdq2ps);
    register_0f_op_prefix(0x66, 0x5A, "CVTPD2PS", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvtpd2ps);
    register_0f_op_prefix(0x66, 0x5B, "CVTPS2DQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvtps2dq);
    register_0f_op_prefix(0xF3, 0x5B, "CVTTPS2DQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvttps2dq);
    register_0f_op(0x5C, "SUBPD", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_subps);
    register_0f_op_prefix(0x66, 0x5C, "SUBPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_subpd);
    register_0f_op_prefix(0xF2, 0x5C, "SUBSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_subsd);
    register_0f_op_prefix(0xF3, 0x5C, "SUBSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_subss);
    register_0f_op(0x5D, "MINPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_minps);
    register_0f_op_prefix(0x66, 0x5D, "MINPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_minpd);
    register_0f_op_prefix(0xF2, 0x5D, "MINSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_minsd);
    register_0f_op_prefix(0xF3, 0x5D, "MINSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_minss);
    register_0f_op(0x5E, "DIVPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_divps);
    register_0f_op_prefix(0x66, 0x5E, "DIVPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_divpd);
    register_0f_op_prefix(0xF2, 0x5E, "DIVSD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_divsd);
    register_0f_op_prefix(0xF3, 0x5E, "DIVSS", SSE, xmm1_xmm2m32, xmm1_xmm2m32, USE_RM, INSTR, x86_divss);
    register_0f_op(0x5F, "MAXPS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_maxps);
    register_0f_op_prefix(0x66, 0x5F, "MAXPD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_maxpd);
    register_0f_op_prefix(0xF2, 0x5F, "MAXSD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_maxsd);
    register_0f_op_prefix(0xF3, 0x5F, "MAXSS", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_maxss);
    register_0f_op(0x60, "PUNPCKLBW", MMX, mm1_mm2m32, mm1_mm2m32, USE_RM, INSTR, x86_punpcklbw);
    register_0f_op_prefix(0x66, 0x60, "PUNPCKLBW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpcklbw);
    register_0f_op(0x61, "PUNPCKLWD", MMX, mm1_mm2m32, mm1_mm2m32, USE_RM, INSTR, x86_punpcklwd);
    register_0f_op_prefix(0x66, 0x61, "PUNPCKLWD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpcklwd);
    register_0f_op(0x62, "PUNPCKLDQ", MMX, mm1_mm2m32, mm1_mm2m32, USE_RM, INSTR, x86_punpckldq);
    register_0f_op_prefix(0x66, 0x62, "PUNPCKLDQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpckldq);
    register_0f_op(0x63, "PACKSSWB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_packsswb);
    register_0f_op_prefix(0x66, 0x63, "PACKSSWB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_packsswb);
    register_0f_op(0x64, "PCMPGTB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpgtb);
    register_0f_op_prefix(0x66, 0x64, "PACKSSWB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpgtb);
    register_0f_op(0x65, "PCMPGTB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpgtw);
    register_0f_op_prefix(0x66, 0x65, "PACKSSWB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpgtw);
    register_0f_op(0x66, "PCMPGTB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpgtd);
    register_0f_op_prefix(0x66, 0x66, "PACKSSWB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpgtd);
    register_0f_op(0x67, "PACKUSWB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_packuswb);
    register_0f_op_prefix(0x66, 0x67, "PACKUSWB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_packuswb);
    register_0f_op(0x68, "PUNPCKHBW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_punpckhbw);
    register_0f_op_prefix(0x66, 0x68, "PUNPCKHBW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpckhbw);
    register_0f_op(0x69, "PUNPCKHWD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_punpckhwd);
    register_0f_op_prefix(0x66, 0x69, "PUNPCKHWD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpckhwd);
    register_0f_op(0x6A, "PUNPCKHDQ", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_punpckhdq);
    register_0f_op_prefix(0x66, 0x6A, "PUNPCKHDQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpckhdq);
    register_0f_op(0x6B, "PACKSSDW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_packssdw);
    register_0f_op_prefix(0x66, 0x6B, "PACKSSDW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_packssdw);
    register_0f_op_prefix(0x66, 0x6C, "PUNPCKLQDQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpcklqdq);
    register_0f_op_prefix(0x66, 0x6D, "PUNPCKHQDQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_punpckhqdq);
    register_0f_op(0x6E, "MOVD", MMX, mm_rm32, mm_rm32, USE_RM, INSTR, x86_movd);
    register_0f_op_prefix(0x66, 0x6E, "MOVD", SSE2, xmm_rm32, xmm_rm32, USE_RM, INSTR, x86_movd);
    register_0f_op(0x6F, "MOVQ", MMX, mm_m64, mm_m64, USE_RM, INSTR, x86_movq);
    register_0f_op_prefix(0x66, 0x6F, "MOVDQA", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movdqa);
    register_0f_op_prefix(0xF3, 0x6F, "MOVDQU", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_movdqu);
    register_0f_op(0x70, "PSHUFW", NONE, mm1_mm2m64_imm8, mm1_mm2m64_imm8, USE_RM, INSTR, x86_pshufw);
    register_0f_op_prefix(0x66, 0x70, "PSHUFD", SSE2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pshufd);
    register_0f_op_prefix(0xF2, 0x70, "PSHUFLW", SSE2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pshuflw);
    register_0f_op_prefix(0xF3, 0x70, "PSHUFHW", SSE2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_pshufhw);
    register_0f_op(0x71, "PSLLW", MMX, mm1_imm8, mm1_imm8, USE_RM, INSTR, x86_psllw);
    register_0f_op_prefix(0x66, 0x71, "PSLLW", SSE2, xmm1_imm8, xmm1_imm8, USE_RM, INSTR, x86_psllw);
    register_0f_op(0x72, "PSLLD", MMX, mm_imm8, mm_imm8, USE_RM, INSTR, x86_pslld);
    register_0f_op_prefix(0x66, 0x72, "PSLLD", SSE2, xmm1_imm8, xmm1_imm8, USE_RM, INSTR, x86_pslld);
    register_0f_op(0x73, "PSLLQ", MMX, mm_imm8, mm_imm8, USE_RM, INSTR, x86_psllq);
    register_0f_op_prefix(0x66, 0x73, "PSLLQ", SSE2, xmm1_imm8, xmm1_imm8, USE_RM, INSTR, x86_psllq);
    register_0f_op(0x74, "PCMPEQB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpeqb);
    register_0f_op_prefix(0x66, 0x74, "PCMPEQB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpeqb);
    register_0f_op(0x75, "PCMPEQW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpeqw);
    register_0f_op_prefix(0x66, 0x75, "PCMPEQW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpeqw);
    register_0f_op(0x76, "PCMPEQD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pcmpeqd);
    register_0f_op_prefix(0x66, 0x76, "PCMPEQD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pcmpeqd);
    register_0f_op(0x77, "EMMS", NONE, OP, OP, NO_RM, INSTR, x86_emms);

    register_0f_op_prefix(0x66, 0x7C, "HADDPD", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_haddpd);
    register_0f_op_prefix(0xF2, 0x7C, "HADDPS", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_haddps);
    register_0f_op_prefix(0x66, 0x7D, "HSUBPD", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_hsubpd);
    register_0f_op_prefix(0xF2, 0x7D, "HSUBPS", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_hsubps);
    register_0f_op(0x7E, "MOVD", MMX, rm32_mm, rm32_mm, USE_RM, INSTR, x86_movd);
    register_0f_op_prefix(0x66, 0x7E, "MOVD", SSE2, rm32_xmm, rm32_xmm, USE_RM, INSTR, x86_movd);
    register_0f_op_prefix(0xF3, 0x7E, "MOVQ", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_movq);
    register_0f_op(0x7F, "MOVQ", MMX, m64_mm, m64_mm, USE_RM, INSTR, x86_movq);
    register_0f_op_prefix(0x66, 0x7F, "MOVDQA", NONE, xmm2m128_xmm1, xmm2m128_xmm1, USE_RM, INSTR, x86_movdqa);
    register_0f_op_prefix(0xF3, 0x7F, "MOVDQU", NONE, xmm2m128_xmm1, xmm2m128_xmm1, USE_RM, INSTR, x86_movdqu);
    register_0f_op(0x80, "JO",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x81, "JNO",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x82, "JB",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x83, "JAE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x84, "JE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x85, "JNE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x86, "JBE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x87, "JA",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x88, "JS",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x89, "JNS",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8A, "JP",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8B, "JNP",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8C, "JL",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8D, "JGE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8E, "JLE",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x8F, "JG",  NONE, rela32, rela16, NO_RM, INSTR, x86_jcc);
    register_0f_op(0x90, "SETO", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x91, "SETNO", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x92, "SETB", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x93, "SETAE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x94, "SETE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x95, "SETNE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x96, "SETBE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x97, "SETA", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x98, "SETS", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x99, "SETNS", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9A, "SETP", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9B, "SETNP", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9C, "SETL", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9D, "SETGE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9E, "SETLE", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0x9F, "SETG", NONE, rm8, rm8, USE_RM, INSTR, x86_setcc);
    register_0f_op(0xA0, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_0f_op(0xA1, "POP", NONE, OP, OP, NO_RM, INSTR, x86_mm_pop);
    register_0f_op(0xA2, "CPUID", NONE, OP, OP, NO_RM, INSTR, x86_cpuid);
    register_0f_op(0xA3, "BT", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_bt);
    register_0f_op(0xA4, "SHLD", NONE, rm32_r32_imm8, rm16_r16_imm8, USE_RM, INSTR, x86_shld);
    register_0f_op(0xA5, "SHLD", NONE, rm32_r32_CL, rm16_r16_CL, USE_RM, INSTR, x86_shld);

    register_0f_op(0xA8, "PUSH", NONE, OP, OP, NO_RM, INSTR, x86_mm_push);
    register_0f_op(0xA9, "POP", NONE, OP, OP, NO_RM, INSTR, x86_mm_pop);
    register_0f_op(0xAA, "RSM", NONE, OP, OP, NO_RM, INSTR, x86_rsm);
    register_0f_op(0xAB, "BTS", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_bts);
    register_0f_op(0xAC, "SHLD", NONE, rm32_r32_imm8, rm16_r16_imm8, USE_RM, INSTR, x86_shrd);
    register_0f_op(0xAD, "SHLD", NONE, rm32_r32_CL, rm16_r16_CL, USE_RM, INSTR, x86_shrd);
    register_0f_op_ext(0xAE, 0, "FXSAVE", NONE, OP, OP, USE_RM, INSTR, x86_fxsave);
    register_0f_op_ext(0xAE, 1, "FXRSTOR", NONE, OP, OP, USE_RM, INSTR, x86_fxrstor);
    register_0f_op_ext(0xAE, 2, "LDMXCSR", SSE, m32, m32, USE_RM, INSTR, x86_ldmxcsr);
    register_0f_op_ext(0xAE, 7, "CLFLUSH", CLFSH, m8, m8, USE_RM, INSTR, x86_clflush);
    register_0f_op_sec(0xAE, 0xE8, "LFENCE", SSE, OP, OP, NO_RM, INSTR, x86_lfence);
    register_0f_op_sec(0xAE, 0xF0, "MFENCE", SSE, OP, OP, NO_RM, INSTR, x86_mfence);
    register_0f_op_sec(0xAE, 0xF8, "SFENCE", NONE, OP, OP, NO_RM, INSTR, x86_sfence);
    register_0f_op(0xAF, "IMUL", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_imul);
    register_0f_op(0xB0, "CMPXCHG", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_cmpxchg);
    register_0f_op(0xB1, "CMPXCHG", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_cmpxchg);
    register_0f_op(0xB2, "LSS", NONE, r32_m16_32, r16_m16_16, USE_RM, INSTR, x86_lss);
    register_0f_op(0xB3, "BTR", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_btr);
    register_0f_op(0xB4, "LFS", NONE, r32_m16_32, r16_m16_16, USE_RM, INSTR, x86_lfs);
    register_0f_op(0xB5, "LGS", NONE, r32_m16_32, r16_m16_16, USE_RM, INSTR, x86_lgs);
    register_0f_op(0xB6, "MOVZX", NONE, r32_rm8, r16_rm8, USE_RM, INSTR, x86_movzx);
    register_0f_op(0xB7, "MOVZX", NONE, r32_rm16, r32_rm16, USE_RM, INSTR, x86_movzx);
    register_0f_op_prefix(0xF3, 0xB8, "POPCNT", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_popcnt);
    register_0f_op(0xB9, "UD1", NONE, r32_rm32, r32_rm32, USE_RM, INSTR, x86_ud1);
    register_0f_op_ext(0xBA, 4, "BT", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_bt);
    register_0f_op_ext(0xBA, 5, "BTS", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_bts);
    register_0f_op_ext(0xBA, 6, "BTR", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_btr);
    register_0f_op_ext(0xBA, 7, "BTC", NONE, rm32_imm8, rm16_imm8, USE_RM, INSTR, x86_btc);
    register_0f_op(0xBB, "BTC", NONE, rm32_r32, rm16_r16, USE_RM, INSTR, x86_btc);
    register_0f_op(0xBC, "BSF", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_bsf);
    register_0f_op_prefix(0xF3, 0xBC, "TZCNT", BMI1, r32_rm32, r16_rm16, USE_RM, INSTR, x86_tzcnt);
    register_0f_op(0xBD, "BSR", NONE, r32_rm32, r16_rm16, USE_RM, INSTR, x86_bsr);
    register_0f_op_prefix(0xF3, 0xBD, "LZCNT", LZCNT, r32_rm32, r16_rm16, USE_RM, INSTR, x86_lzcnt);
    register_0f_op(0xBE, "MOVSX", NONE, r32_rm8, r16_rm8, USE_RM, INSTR, x86_movsx);
    register_0f_op(0xBF, "MOVSX", NONE, r32_rm16, r32_rm16, USE_RM, INSTR, x86_movsx);
    register_0f_op(0xC0, "XADD", NONE, rm8_r8, rm8_r8, USE_RM, INSTR, x86_xadd);
    register_0f_op(0xC1, "XADD", NONE, rm32_r32, rm32_r32, USE_RM, INSTR, x86_xadd);
    register_0f_op(0xC2, "CMPPS", SSE, xmm1_xmm2m128_imm8, xmm1_xmm2m128, USE_RM, INSTR, x86_cmpps);
    register_0f_op_prefix(0x66, 0xC2, "CMPPD", SSE2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_cmppd);
    register_0f_op_prefix(0xF2, 0xC2, "CMPSD", SSE2, xmm1_xmm2m64_imm8, xmm1_xmm2m64_imm8, USE_RM, INSTR, x86_cmpsd);
    register_0f_op_prefix(0xF3, 0xC2, "CMPSS", SSE, xmm1_xmm2m32_imm8, xmm1_xmm2m32_imm8, USE_RM, INSTR, x86_cmpss);
    register_0f_op(0xC3, "MOVNTI", NONE, m32_r32, m32_r32, USE_RM, INSTR, x86_movnti);
    register_0f_op(0xC4, "PINSRW", SSE, mm_r32m16_imm8, mm_r32m16_imm8, USE_RM, INSTR, x86_pinsrw);
    register_0f_op_prefix(0x66, 0xC4, "PINSRW", SSE, xmm_r32m16_imm8, xmm_r32m16_imm8, USE_RM, INSTR, x86_pinsrw);
    register_0f_op(0xC5, "PEXTRW", SSE, r32_mm_imm8, r32_mm_imm8, USE_RM, INSTR, x86_pextrw);
    register_0f_op_prefix(0x66, 0xC5, "PEXTRW", SSE2, r32_xmm_imm8, r32_xmm_imm8, USE_RM, INSTR, x86_pextrw);
    register_0f_op(0xC6, "SHUFPS", SSE, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_shufpd);
    register_0f_op_prefix(0x66, 0xC6, "SHUFPD", SSE2, xmm1_xmm2m128_imm8, xmm1_xmm2m128_imm8, USE_RM, INSTR, x86_shufpd);
    register_0f_op_ext(0xC7, 1, "CMPXCHG8B", NONE, m64, m64, USE_RM, INSTR, x86_cmpxchg8b);
    for (size_t i = 0; i < 8; i++)
        register_0f_op(0xC8 + i, "BSWAP", NONE, OP, OP, NO_RM, INSTR, x86_bswap);
    register_0f_op_prefix(0x66, 0xD0, "ADDSUBPD", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_addsubpd);
    register_0f_op_prefix(0xF2, 0xD0, "ADDSUBPS", SSE3, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_addsubps);

    register_0f_op(0xD4, "PADDQ", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddq);
    register_0f_op_prefix(0x66, 0xD4, "PADDQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddq);
    register_0f_op(0xD5, "PMULLW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmullw);
    register_0f_op_prefix(0x66, 0xD5, "PMULLW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmullw);
    register_0f_op_prefix(0x66, 0xD6, "MOVQ", SSE2, xmm2m64_xmm1, xmm2m64_xmm1, USE_RM, INSTR, x86_movq);
    register_0f_op_prefix(0xF2, 0xD6, "MOVDQ2Q", NONE, mm_xmm, mm_xmm, USE_RM, INSTR, x86_movdq2q);
    register_0f_op_prefix(0xF3, 0xD6, "MOVQ2DQ", NONE, xmm_mm, xmm_mm, USE_RM, INSTR, x86_movq2dq);
    register_0f_op(0xD7, "PMOVMSKB", SSE, r32_mm, r32_mm, USE_RM, INSTR, x86_pmovmskb);
    register_0f_op_prefix(0x66, 0xD7, "PMOVMSKB", SSE2, r32_xmm, r32_xmm, USE_RM, INSTR, x86_pmovmskb);
    register_0f_op(0xD8, "PSUBUSB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubusb);
    register_0f_op_prefix(0x66, 0xD8, "PSUBUSB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubusb);
    register_0f_op(0xD9, "PSUBUSW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubusw);
    register_0f_op_prefix(0x66, 0xD9, "PSUBUSW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubusw);
    register_0f_op(0xDA, "PMINUB", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pminub);
    register_0f_op_prefix(0x66, 0xDA, "PMINUB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminub);
    register_0f_op(0xDB, "PAND", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pand);
    register_0f_op_prefix(0x66, 0xDB, "PAND", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pand);
    register_0f_op(0xDC, "PADDUSB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddusb);
    register_0f_op_prefix(0x66, 0xDC, "PADDUSB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddusb);
    register_0f_op(0xDD, "PADDUSW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddusw);
    register_0f_op_prefix(0x66, 0xDD, "PADDUSW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddusw);
    register_0f_op(0xDE, "PMAXUB", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmaxub);
    register_0f_op_prefix(0x66, 0xDE, "PMAXUB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxub);
    register_0f_op(0xDF, "PANDN", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pandn);
    register_0f_op_prefix(0x66, 0xDF, "PANDN", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pandn);

    register_0f_op(0xE0, "PAVGB", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pavgb);
    register_0f_op_prefix(0x66, 0xE0, "PAVGB", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pavgb);

    register_0f_op(0xE3, "PAVGW", SSE2, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pavgw);
    register_0f_op_prefix(0x66, 0xE3, "PAVGW", SSE, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pavgw);
    register_0f_op(0xE4, "PMULHUW", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmulhuw);
    register_0f_op_prefix(0x66, 0xE4, "PMULHUW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmulhuw);
    register_0f_op(0xE5, "PMULHW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmulhw);
    register_0f_op_prefix(0x66, 0xE5, "PMULHW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmulhw);
    register_0f_op_prefix(0x66, 0xE6, "CVTTPD2DQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvttpd2dq);
    register_0f_op_prefix(0xF2, 0xE6, "CVTPD2DQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_cvtpd2qd);
    register_0f_op_prefix(0xF3, 0xE6, "CVTDQ2PD", SSE2, xmm1_xmm2m64, xmm1_xmm2m64, USE_RM, INSTR, x86_cvtdq2pd);
    register_0f_op(0xE7, "MOVNTQ", NONE, m64_mm, m64_mm, USE_RM, INSTR, x86_movntq);
    register_0f_op_prefix(0x66, 0xE7, "MOVNTDQ", SSE2, m128_xmm1, m128_xmm1, USE_RM, INSTR, x86_movntdq);
    register_0f_op(0xE8, "PSUBSB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubsb);
    register_0f_op_prefix(0x66, 0xE8, "PSUBSB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubsb);
    register_0f_op(0xE9, "PSUBSW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubsw);
    register_0f_op_prefix(0x66, 0xE9, "PSUBSW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubsw);
    register_0f_op(0xEA, "PMINSW", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pminsw);
    register_0f_op_prefix(0x66, 0xEA, "PMINSW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pminsw);
    register_0f_op(0xEB, "POR", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_por);
    register_0f_op_prefix(0x66, 0xEB, "POR", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_por);
    register_0f_op(0xEC, "PADDSB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddsb);
    register_0f_op_prefix(0x66, 0xEC, "PADDSB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddsb);
    register_0f_op(0xED, "PADDSW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddsw);
    register_0f_op_prefix(0x66, 0xED, "PADDSW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddsw);
    register_0f_op(0xEE, "PMAXSW", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmaxsw);
    register_0f_op_prefix(0x66, 0xEE, "PMAXSW", SSE4_1, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pmaxsw);
    register_0f_op(0xEF, "PXOR", SSE2, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pxor);
    register_0f_op_prefix(0x66, 0xEF, "PXOR", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pxor);
    register_0f_op_prefix(0xF2, 0xF0, "LDDQU", SSE3, xmm1_mm, xmm1_mm, USE_RM, INSTR, x86_lddqu);
    register_0f_op(0xF1, "PSLLW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psllw);
    register_0f_op_prefix(0x66, 0xF1, "PSLLW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psllw);
    register_0f_op(0xF2, "PSLLD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pslld);
    register_0f_op_prefix(0x66, 0xF2, "PSLLD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_pslld);
    register_0f_op(0xF3, "PSLLQ", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psllq);
    register_0f_op_prefix(0x66, 0xF3, "PSLLQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psllq);
    register_0f_op(0xF4, "PMULUDQ", SSE2, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmuludq);
    register_0f_op_prefix(0x66, 0xF4, "PMULUDQ", SSE2, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_pmuludq);
    register_0f_op(0xF5, "PMADDWD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_pmaddwd);
    register_0f_op_prefix(0x66, 0xF5, "PMADDWD", SSE2, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_pmaddwd);
    register_0f_op(0xF6, "PSADBW", SSE, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psadbw);
    register_0f_op_prefix(0x66, 0xF6, "PSADBW", SSE2, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_psadbw);
    register_0f_op(0xF7, "MASKMOVQ", NONE, mm1_mm2, mm1_mm2, USE_RM, INSTR, x86_maskmovq);
    register_0f_op_prefix(0x66, 0xF7, "MASKMOVDQU", SSE2, xmm1_xmm2, xmm1_xmm2, USE_RM, INSTR, x86_maskmovdqu);
    register_0f_op(0xF8, "PSUBB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubb);
    register_0f_op_prefix(0x66, 0xF8, "PSUBB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubb);
    register_0f_op(0xF9, "PSUBW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubw);
    register_0f_op_prefix(0x66, 0xF9, "PSUBW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubw);
    register_0f_op(0xFA, "PSUBD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubd);
    register_0f_op_prefix(0x66, 0xFA, "PSUBD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubd);
    register_0f_op(0xFB, "PSUBQ", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_psubq);
    register_0f_op_prefix(0x66, 0xFB, "PSUBQ", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_psubq);
    register_0f_op(0xFC, "PADDB", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddb);
    register_0f_op_prefix(0x66, 0xFC, "PADDB", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddb);
    register_0f_op(0xFD, "PADDW", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddw);
    register_0f_op_prefix(0x66, 0xFD, "PADDW", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddw);
    register_0f_op(0xFE, "PADDD", MMX, mm1_mm2m64, mm1_mm2m64, USE_RM, INSTR, x86_paddd);
    register_0f_op_prefix(0x66, 0xFE, "PADDD", SSE2, xmm1_xmm2m128, xmm1_xmm2m128, USE_RM, INSTR, x86_paddd);
    register_0f_op(0xFF, "UD0", NONE, r32_rm32, r32_rm32, USE_RM, INSTR, x86_ud0);

}
