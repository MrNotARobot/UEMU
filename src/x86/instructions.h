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

#ifndef X86_INSTR_H
#define X86_INSTR_H

#include <stddef.h>
#include <stdint.h>

#include "../system.h"

#define TABLE_0F_PREFIX_MASK 5

struct exec_data {
    uint8_t bytes;
    uint8_t ext;    // extension used to get this instruction
    uint8_t sec;    // secondary opcode used
    uint8_t opc;    // opcode used
    uint8_t modrm;
    uint8_t sib;

    uint32_t moffset;
    uint32_t imm1;
    uint32_t imm2;

    uint8_t oprsz_pfx : 1;  // operand-size prefix
    uint8_t adrsz_pfx : 1;  // address-size prefix
    uint8_t lock : 1;    // lock
    uint8_t repnz : 1;
    uint8_t rep : 1;
    uint8_t segovr : 3;     // segment override
};

typedef void (*d_x86_instruction_handler)(void *, struct exec_data);

struct instruction {
    const char *name;
    d_x86_instruction_handler handler;
    moffset32_t eip;
    int encoding;
    struct exec_data data;
    uint8_t fail_to_fetch;
    uint8_t fail_byte;
    uint8_t size;
};

struct opcode {
    const char *o_name;
    d_x86_instruction_handler o_handler;
    struct opcode *o_extensions;
    struct opcode *o_prefix;
    struct opcode *o_sec_table;
    size_t o_sec_tablesz;
    int o_class;
    int o_encoding;             // encoding in the 32-bit addressing mode
    int o_encoding16bit;   // encoding in the 16-bit addressing mode
    uint8_t o_opcode;
    uint8_t o_use_rm : 1;
    uint8_t o_is_prefix : 1;
    uint8_t o_use_op_extension : 1;
};

enum {
    PFX_LOCK = 0xF0,
    PFX_REPNZ = 0xF2,
    PFX_REP = 0xF3,
    PFX_BND = 0xF2,
    PFX_CS = 0x2E,
    PFX_SS = 0x36,
    PFX_DS = 0x3E,
    PFX_ES = 0x26,
    PFX_FS = 0x64,
    PFX_GS = 0x65,
    PFX_OPRSZ = 0x66,
    PFX_ADDRSZ = 0x67
};

enum {
    SEG_CS,
    SEG_SS,
    SEG_DS,
    SEG_ES,
    SEG_FS,
    SEG_GS,
};

struct opcode x86_opcode_table[0xFF + 1];
struct opcode x86_opcode_0f_table[0xFF + 1];

enum x86CPUIDFeatureFlags {
    NONE,
    ADX,
    AES,
    BMI1,
    CLFSH,
    LZCNT,
    MMX,
    MPX,
    OSPKE,
    PCLMULQDQ,
    RTM,
    SEP,
    SHA,
    SMAP,
    SSE,
    SSE2,
    SSE3,
    SSSE3,
    SSE4_1,
    SSE4_2
};

enum x86OpcodeEncoding {
    no_encoding,
    AL_imm8,
    AX_imm8,
    AX_imm16,
    AX_r16,
    bnd_rm32,
    bnd_sib,
    bnd1m64_bnd2,
    bnd1_bnd2m64,
    eAX_imm8,
    eAX_imm32,
    eAX_r32,
    imm8,
    imm8_AL,
    imm8_AX,
    imm8_eAX,
    imm16,
    imm16_AX,
    imm16_imm8,
    imm32,
    imm32_eAX,
    m8,
    m16,
    m16_16,
    m16_32,
    m32,
    m32_r32,
    m64,
    m64_mm,
    m64_xmm1,
    m128_xmm1,
    mm_imm8,
    mm_m64,
    mm_rm32,
    mm_r32m16_imm8,
    mm_xmm,
    mm_xmm1m64,
    mm_xmm1m128,
    mm1_imm8,
    mm1_mm2,
    mm1_mm2m32,
    mm1_mm2m64,
    mm1_mm2m64_imm8,
    rm8_imm8,
    rm8,
    rm8_1,
    rm8_CL,
    rm8_r8,
    rm8_xmm2_imm8,
    rm16,
    rm16_1,
    rm16_CL,
    rm16_imm8,
    rm16_imm16,
    rm16_r16,
    rm16_r16_CL,
    rm16_r16_imm8,
    rm16_sreg,
    rm16_xmm1_imm8,
    rm32,
    rm32_1,
    rm32_CL,
    rm32_mm,
    rm32_r32,
    rm32_r32_CL,
    rm32_r32_imm8,
    rm32_imm8,
    rm32_imm32,
    rm32_xmm,
    rm32_xmm1_imm8,
    rm32_xmm2_imm8,
    r8_rm8,
    r16,
    r16_m16,
    r16_m16_16,
    r16_rm8,
    r16_rm16,
    r16_r16m16,
    r16_rm16_imm8,
    r16_rm16_imm16,
    r32,
    r32m16,
    r32_mm,
    r32_m32,
    r32_m16_32,
    r32_rm32,
    r32_rm8,
    r32_rm16,
    r32_r32m16,
    r32_rm32_imm8,
    r32_rm32_imm32,
    r32_mm_imm8,
    r32_xmm_imm8,
    r32_xmm,
    r32_xmm1m32,
    r32_xmm1m64,
    rela8,
    rela16,
    rela32,
    rela16_16,
    rela16_32,
    sib_bnd,
    sreg_rm16,
    OP,
    ptr16_16,
    ptr16_32,
    xmm_mm,
    xmm_rm32,
    xmm_r32m16_imm8,
    xmm1_imm8,
    xmm1_mm,
    xmm1_m32,
    xmm1_m64,
    xmm1_m128,
    xmm1_mm1m64,
    xmm1_rm32,
    xmm1_r32m8_imm8,
    xmm1_r32m32,
    xmm1_xmm2,
    xmm1_xmm2m16,
    xmm1_xmm2m32,
    xmm1_xmm2m32_imm8,
    xmm1_xmm2m64,
    xmm1_xmm2m64_imm8,
    xmm1_xmm2m128,
    xmm1_xmm2m128_imm8,
    xmm1m64_xmm2,
    xmm2m32_xmm1,
    xmm2m64_xmm1,
    xmm2m128_xmm1
};

enum x86OpcodeFlags {
    NO_RM = 0,
    USE_RM = 1,
    INSTR = 0,
    PREFIX = 1,
    NO_EXT = 0,
    OP_EXT = 1,
};

void x86_init_opcode_table(void);
void x86_free_opcode_table(void);

_Bool x86_byteispfx(uint8_t);
uint8_t byte2segovr(uint8_t);

void x86_aaa(void *, struct exec_data);
void x86_aad(void *, struct exec_data);
void x86_mm_add(void *, struct exec_data);
void x86_aam(void *, struct exec_data);
void x86_aas(void *, struct exec_data);
void x86_adc(void *, struct exec_data);
void x86_adcx(void *, struct exec_data);
void x86_addpd(void *, struct exec_data);
void x86_addps(void *, struct exec_data);
void x86_addsd(void *, struct exec_data);
void x86_addss(void *, struct exec_data);
void x86_addsubpd(void *, struct exec_data);
void x86_addsubps(void *, struct exec_data);
void x86_adox(void *, struct exec_data);
void x86_aesdec(void *, struct exec_data);
void x86_aesdeclast(void *, struct exec_data);
void x86_aesenc(void *, struct exec_data);
void x86_aesenclast(void *, struct exec_data);
void x86_aesimc(void *, struct exec_data);
void x86_aeskeygenassist(void *, struct exec_data);
void x86_mm_and(void *, struct exec_data);
void x86_andpd(void *, struct exec_data);
void x86_andps(void *, struct exec_data);
void x86_andnpd(void *, struct exec_data);
void x86_andnps(void *, struct exec_data);
void x86_arpl(void *, struct exec_data);
void x86_blendpd(void *, struct exec_data);
void x86_blendps(void *, struct exec_data);
void x86_blendvpd(void *, struct exec_data);
void x86_blendvps(void *, struct exec_data);
void x86_bndcl(void *, struct exec_data);
void x86_bndcu(void *, struct exec_data);
void x86_bndcn(void *, struct exec_data);
void x86_bndldx(void *, struct exec_data);
void x86_bndmk(void *, struct exec_data);
void x86_bndmov(void *, struct exec_data);
void x86_bndstx(void *, struct exec_data);
void x86_bound(void *, struct exec_data);
void x86_bsf(void *, struct exec_data);
void x86_bsr(void *, struct exec_data);
void x86_bswap(void *, struct exec_data);
void x86_bt(void *, struct exec_data);
void x86_btc(void *, struct exec_data);
void x86_btr(void *, struct exec_data);
void x86_bts(void *, struct exec_data);
void x86_mm_call(void *, struct exec_data);
void x86_cbw(void *, struct exec_data);
void x86_clac(void *, struct exec_data);
void x86_clc(void *, struct exec_data);
void x86_cld(void *, struct exec_data);
void x86_clflush(void *, struct exec_data);
void x86_cli(void *, struct exec_data);
void x86_clts(void *, struct exec_data);
void x86_cmc(void *, struct exec_data);
void x86_cmovcc(void *, struct exec_data);
void x86_cmp(void *, struct exec_data);
void x86_cmppd(void *, struct exec_data);
void x86_cmpps(void *, struct exec_data);
void x86_cmps(void *, struct exec_data);
void x86_cmpsd(void *, struct exec_data);
void x86_cmpss(void *, struct exec_data);
void x86_cmpxchg(void *, struct exec_data);
void x86_cmpxchg8b(void *, struct exec_data);
void x86_comisd(void *, struct exec_data);
void x86_comiss(void *, struct exec_data);
void x86_cpuid(void *, struct exec_data);
void x86_crc32(void *, struct exec_data);
void x86_cvtdq2pd(void *, struct exec_data);
void x86_cvtdq2ps(void *, struct exec_data);
void x86_cvtpd2qd(void *, struct exec_data);
void x86_cvtpd2pi(void *, struct exec_data);
void x86_cvtpd2ps(void *, struct exec_data);
void x86_cvtpi2pd(void *, struct exec_data);
void x86_cvtpi2ps(void *, struct exec_data);
void x86_cvtps2dq(void *, struct exec_data);
void x86_cvtps2pd(void *, struct exec_data);
void x86_cvtps2pi(void *, struct exec_data);
void x86_cvtsd2si(void *, struct exec_data);
void x86_cvtsd2ss(void *, struct exec_data);
void x86_cvtsi2sd(void *, struct exec_data);
void x86_cvtsi2ss(void *, struct exec_data);
void x86_cvtss2sd(void *, struct exec_data);
void x86_cvtss2si(void *, struct exec_data);
void x86_cvttpd2dq(void *, struct exec_data);
void x86_cvttpd2pi(void *, struct exec_data);
void x86_cvttps2dq(void *, struct exec_data);
void x86_cvttps2pi(void *, struct exec_data);
void x86_cvttsd2si(void *, struct exec_data);
void x86_cvttss2si(void *, struct exec_data);
void x86_cwq(void *, struct exec_data);
void x86_daa(void *, struct exec_data);
void x86_das(void *, struct exec_data);
void x86_dec(void *, struct exec_data);
void x86_div(void *, struct exec_data);
void x86_divpd(void *, struct exec_data);
void x86_divps(void *, struct exec_data);
void x86_divsd(void *, struct exec_data);
void x86_divss(void *, struct exec_data);
void x86_dppd(void *, struct exec_data);
void x86_dpps(void *, struct exec_data);
void x86_emms(void *, struct exec_data);
void x86_mm_endbr32(void *, struct exec_data);
void x86_enter(void *, struct exec_data);
void x86_extractps(void *, struct exec_data);
void x86_f2xm1(void *, struct exec_data);
void x86_fabs(void *, struct exec_data);
void x86_fadd(void *, struct exec_data);
void x86_faddp(void *, struct exec_data);
void x86_fiadd(void *, struct exec_data);
void x86_fbld(void *, struct exec_data);
void x86_fbstp(void *, struct exec_data);
void x86_fchs(void *, struct exec_data);
void x86_fnclex(void *, struct exec_data);
void x86_fcmovcc(void *, struct exec_data);
void x86_fcom(void *, struct exec_data);
void x86_fcomp(void *, struct exec_data);
void x86_fcompp(void *, struct exec_data);
void x86_fcomi(void *, struct exec_data);
void x86_fcomip(void *, struct exec_data);
void x86_fucomi(void *, struct exec_data);
void x86_fucomip(void *, struct exec_data);
void x86_fcos(void *, struct exec_data);
void x86_fdecstp(void *, struct exec_data);
void x86_fdiv(void *, struct exec_data);
void x86_fdivp(void *, struct exec_data);
void x86_fidiv(void *, struct exec_data);
void x86_fdivr(void *, struct exec_data);
void x86_fdivrp(void *, struct exec_data);
void x86_fdivir(void *, struct exec_data);
void x86_ffree(void *, struct exec_data);
void x86_ficom(void *, struct exec_data);
void x86_ficomp(void *, struct exec_data);
void x86_fild(void *, struct exec_data);
void x86_fincstp(void *, struct exec_data);
void x86_fclex(void *, struct exec_data);
void x86_finit(void *, struct exec_data);
void x86_fininit(void *, struct exec_data);
void x86_fist(void *, struct exec_data);
void x86_fistp(void *, struct exec_data);
void x86_fisttp(void *, struct exec_data);
void x86_fld(void *, struct exec_data);
void x86_fld1(void *, struct exec_data);
void x86_fldl2t(void *, struct exec_data);
void x86_fldl2e(void *, struct exec_data);
void x86_fldpi(void *, struct exec_data);
void x86_fldlg2(void *, struct exec_data);
void x86_fldln2(void *, struct exec_data);
void x86_fldz(void *, struct exec_data);
void x86_fldcw(void *, struct exec_data);
void x86_fldenv(void *, struct exec_data);
void x86_fmul(void *, struct exec_data);
void x86_fmulp(void *, struct exec_data);
void x86_fimul(void *, struct exec_data);
void x86_fnop(void *, struct exec_data);
void x86_fpatan(void *, struct exec_data);
void x86_fprem(void *, struct exec_data);
void x86_fprem1(void *, struct exec_data);
void x86_fptan(void *, struct exec_data);
void x86_frndint(void *, struct exec_data);
void x86_frstor(void *, struct exec_data);
void x86_fsave(void *, struct exec_data);
void x86_fnsave(void *, struct exec_data);
void x86_fscale(void *, struct exec_data);
void x86_fsin(void *, struct exec_data);
void x86_fsincos(void *, struct exec_data);
void x86_fsqrt(void *, struct exec_data);
void x86_fst(void *, struct exec_data);
void x86_fstp(void *, struct exec_data);
void x86_fnstcw(void *, struct exec_data);
void x86_fstenv(void *, struct exec_data);
void x86_fnstenv(void *, struct exec_data);
void x86_fstsw(void *, struct exec_data);
void x86_fnstsw(void *, struct exec_data);
void x86_fsub(void *, struct exec_data);
void x86_fsubp(void *, struct exec_data);
void x86_fisub(void *, struct exec_data);
void x86_fsubr(void *, struct exec_data);
void x86_fsubrp(void *, struct exec_data);
void x86_fisubr(void *, struct exec_data);
void x86_ftst(void *, struct exec_data);
void x86_fucom(void *, struct exec_data);
void x86_fucomp(void *, struct exec_data);
void x86_fucompp(void *, struct exec_data);
void x86_fxam(void *, struct exec_data);
void x86_fxch(void *, struct exec_data);
void x86_fxrstor(void *, struct exec_data);
void x86_fxsave(void *, struct exec_data);
void x86_fxtract(void *, struct exec_data);
void x86_fyl2x(void *, struct exec_data);
void x86_fyl2xp1(void *, struct exec_data);
void x86_haddpd(void *, struct exec_data);
void x86_haddps(void *, struct exec_data);
void x86_hlt(void *, struct exec_data);
void x86_hsubpd(void *, struct exec_data);
void x86_hsubps(void *, struct exec_data);
void x86_idiv(void *, struct exec_data);
void x86_imul(void *, struct exec_data);
void x86_in(void *, struct exec_data);
void x86_inc(void *, struct exec_data);
void x86_ins(void *, struct exec_data);
void x86_insertps(void *, struct exec_data);
void x86_int3(void *, struct exec_data);
void x86_int(void *, struct exec_data);
void x86_int0(void *, struct exec_data);
void x86_int1(void *, struct exec_data);
void x86_invd(void *, struct exec_data);
void x86_invlpg(void *, struct exec_data);
void x86_iret(void *, struct exec_data);
void x86_mm_jcc(void *, struct exec_data);
void x86_jmp(void *, struct exec_data);
void x86_lahf(void *, struct exec_data);
void x86_lar(void *, struct exec_data);
void x86_lddqu(void *, struct exec_data);
void x86_ldmxcsr(void *, struct exec_data);
void x86_lds(void *, struct exec_data);
void x86_lss(void *, struct exec_data);
void x86_les(void *, struct exec_data);
void x86_lfs(void *, struct exec_data);
void x86_lgs(void *, struct exec_data);
void x86_mm_lea(void *, struct exec_data);
void x86_leave(void *, struct exec_data);
void x86_lfence(void *, struct exec_data);
void x86_lgdt(void *, struct exec_data);
void x86_lidt(void *, struct exec_data);
void x86_lldt(void *, struct exec_data);
void x86_lmsw(void *, struct exec_data);
void x86_lods(void *, struct exec_data);
void x86_loopcc(void *, struct exec_data);
void x86_lsl(void *, struct exec_data);
void x86_ltr(void *, struct exec_data);
void x86_lzcnt(void *, struct exec_data);
void x86_maskmovdqu(void *, struct exec_data);
void x86_maskmovq(void *, struct exec_data);
void x86_maxpd(void *, struct exec_data);
void x86_maxps(void *, struct exec_data);
void x86_maxsd(void *, struct exec_data);
void x86_maxss(void *, struct exec_data);
void x86_mfence(void *, struct exec_data);
void x86_minpd(void *, struct exec_data);
void x86_minps(void *, struct exec_data);
void x86_minsd(void *, struct exec_data);
void x86_minss(void *, struct exec_data);
void x86_monitor(void *, struct exec_data);
void x86_mm_mov(void *, struct exec_data);
void x86_movapd(void *, struct exec_data);
void x86_movaps(void *, struct exec_data);
void x86_movbe(void *, struct exec_data);
void x86_movd(void *, struct exec_data);
void x86_movddup(void *, struct exec_data);
void x86_movdqa(void *, struct exec_data);
void x86_movdqu(void *, struct exec_data);
void x86_movdq2q(void *, struct exec_data);
void x86_movhlps(void *, struct exec_data);
void x86_movhpd(void *, struct exec_data);
void x86_movhps(void *, struct exec_data);
void x86_movlpd(void *, struct exec_data);
void x86_movlps(void *, struct exec_data);
void x86_movmskpd(void *, struct exec_data);
void x86_movmskps(void *, struct exec_data);
void x86_movntdqa(void *, struct exec_data);
void x86_movntdq(void *, struct exec_data);
void x86_movnti(void *, struct exec_data);
void x86_movntpd(void *, struct exec_data);
void x86_movntps(void *, struct exec_data);
void x86_movntq(void *, struct exec_data);
void x86_movq(void *, struct exec_data);
void x86_movq2dq(void *, struct exec_data);
void x86_movs(void *, struct exec_data);
void x86_movsd(void *, struct exec_data);
void x86_movshdup(void *, struct exec_data);
void x86_movsldup(void *, struct exec_data);
void x86_movss(void *, struct exec_data);
void x86_movsx(void *, struct exec_data);
void x86_movupd(void *, struct exec_data);
void x86_movups(void *, struct exec_data);
void x86_movzx(void *, struct exec_data);
void x86_mpsadbw(void *, struct exec_data);
void x86_mul(void *, struct exec_data);
void x86_mulpd(void *, struct exec_data);
void x86_mulps(void *, struct exec_data);
void x86_mulsd(void *, struct exec_data);
void x86_mulss(void *, struct exec_data);
void x86_mwait(void *, struct exec_data);
void x86_neg(void *, struct exec_data);
void x86_mm_nop(void *, struct exec_data);
void x86_not(void *, struct exec_data);
void x86_or(void *, struct exec_data);
void x86_orpd(void *, struct exec_data);
void x86_orps(void *, struct exec_data);
void x86_out(void *, struct exec_data);
void x86_outs(void *, struct exec_data);
void x86_pabsb(void *, struct exec_data);
void x86_pabsw(void *, struct exec_data);
void x86_pabsd(void *, struct exec_data);
void x86_packsswb(void *, struct exec_data);
void x86_packssdw(void *, struct exec_data);
void x86_packusdw(void *, struct exec_data);
void x86_packuswb(void *, struct exec_data);
void x86_paddb(void *, struct exec_data);
void x86_paddw(void *, struct exec_data);
void x86_paddd(void *, struct exec_data);
void x86_paddq(void *, struct exec_data);
void x86_paddsb(void *, struct exec_data);
void x86_paddsw(void *, struct exec_data);
void x86_paddusb(void *, struct exec_data);
void x86_paddusw(void *, struct exec_data);
void x86_palignr(void *, struct exec_data);
void x86_pand(void *, struct exec_data);
void x86_pandn(void *, struct exec_data);
void x86_pause(void *, struct exec_data);
void x86_pavgb(void *, struct exec_data);
void x86_pavgw(void *, struct exec_data);
void x86_pblendvb(void *, struct exec_data);
void x86_pblendw(void *, struct exec_data);
void x86_pclmulqdq(void *, struct exec_data);
void x86_pcmpeqb(void *, struct exec_data);
void x86_pcmpeqw(void *, struct exec_data);
void x86_pcmpeqd(void *, struct exec_data);
void x86_pcmpeqq(void *, struct exec_data);
void x86_pcmpestri(void *, struct exec_data);
void x86_pcmpestrm(void *, struct exec_data);
void x86_pcmpgtb(void *, struct exec_data);
void x86_pcmpgtw(void *, struct exec_data);
void x86_pcmpgtd(void *, struct exec_data);
void x86_pcmpgtq(void *, struct exec_data);
void x86_pcmpistri(void *, struct exec_data);
void x86_pcmpistrm(void *, struct exec_data);
void x86_pextrb(void *, struct exec_data);
void x86_pextrd(void *, struct exec_data);
void x86_pextrw(void *, struct exec_data);
void x86_phaddw(void *, struct exec_data);
void x86_phaddd(void *, struct exec_data);
void x86_phaddsw(void *, struct exec_data);
void x86_phminposuw(void *, struct exec_data);
void x86_phsubw(void *, struct exec_data);
void x86_phsubd(void *, struct exec_data);
void x86_phsubsw(void *, struct exec_data);
void x86_pinsrb(void *, struct exec_data);
void x86_pinsrd(void *, struct exec_data);
void x86_pinsrw(void *, struct exec_data);
void x86_pmaddubsw(void *, struct exec_data);
void x86_pmaddwd(void *, struct exec_data);
void x86_pmaxsb(void *, struct exec_data);
void x86_pmaxsw(void *, struct exec_data);
void x86_pmaxsd(void *, struct exec_data);
void x86_pmaxub(void *, struct exec_data);
void x86_pmaxuw(void *, struct exec_data);
void x86_pmaxud(void *, struct exec_data);
void x86_pminsb(void *, struct exec_data);
void x86_pminsw(void *, struct exec_data);
void x86_pminsd(void *, struct exec_data);
void x86_pminub(void *, struct exec_data);
void x86_pminuw(void *, struct exec_data);
void x86_pminud(void *, struct exec_data);
void x86_pmovmskb(void *, struct exec_data);
void x86_pmovsx(void *, struct exec_data);
void x86_pmovzx(void *, struct exec_data);
void x86_pmuldq(void *, struct exec_data);
void x86_pmulhrsw(void *, struct exec_data);
void x86_pmulhuw(void *, struct exec_data);
void x86_pmulhw(void *, struct exec_data);
void x86_pmulld(void *, struct exec_data);
void x86_pmullw(void *, struct exec_data);
void x86_pmuludq(void *, struct exec_data);
void x86_mm_pop(void *, struct exec_data);
void x86_popa(void *, struct exec_data);
void x86_popcnt(void *, struct exec_data);
void x86_popf(void *, struct exec_data);
void x86_por(void *, struct exec_data);
void x86_prefetcht0(void *, struct exec_data);
void x86_prefetcht1(void *, struct exec_data);
void x86_prefetcht2(void *, struct exec_data);
void x86_prefetchnta(void *, struct exec_data);
void x86_prefetchw(void *, struct exec_data);
void x86_psadbw(void *, struct exec_data);
void x86_pshufb(void *, struct exec_data);
void x86_pshufd(void *, struct exec_data);
void x86_pshufhw(void *, struct exec_data);
void x86_pshuflw(void *, struct exec_data);
void x86_pshufw(void *, struct exec_data);
void x86_psignb(void *, struct exec_data);
void x86_psignw(void *, struct exec_data);
void x86_psignd(void *, struct exec_data);
void x86_psllw(void *, struct exec_data);
void x86_pslld(void *, struct exec_data);
void x86_psllq(void *, struct exec_data);
void x86_psubb(void *, struct exec_data);
void x86_psubw(void *, struct exec_data);
void x86_psubd(void *, struct exec_data);
void x86_psubq(void *, struct exec_data);
void x86_psubsb(void *, struct exec_data);
void x86_psubsw(void *, struct exec_data);
void x86_psubusb(void *, struct exec_data);
void x86_psubusw(void *, struct exec_data);
void x86_ptest(void *, struct exec_data);
void x86_punpckhbw(void *, struct exec_data);
void x86_punpckhwd(void *, struct exec_data);
void x86_punpckhdq(void *, struct exec_data);
void x86_punpckhqdq(void *, struct exec_data);
void x86_punpcklbw(void *, struct exec_data);
void x86_punpcklwd(void *, struct exec_data);
void x86_punpckldq(void *, struct exec_data);
void x86_punpcklqdq(void *, struct exec_data);
void x86_mm_push(void *, struct exec_data);
void x86_pusha(void *, struct exec_data);
void x86_pushf(void *, struct exec_data);
void x86_pxor(void *, struct exec_data);
void x86_rcl(void *, struct exec_data);
void x86_rcr(void *, struct exec_data);
void x86_rol(void *, struct exec_data);
void x86_ror(void *, struct exec_data);
void x86_rcpps(void *, struct exec_data);
void x86_rcpss(void *, struct exec_data);
void x86_rdmsr(void *, struct exec_data);
void x86_rdpkru(void *, struct exec_data);
void x86_rdpmc(void *, struct exec_data);
void x86_rdtsc(void *, struct exec_data);
void x86_rdtscp(void *, struct exec_data);
void x86_mm_ret(void *, struct exec_data);
void x86_roundpd(void *, struct exec_data);
void x86_roundps(void *, struct exec_data);
void x86_roundsd(void *, struct exec_data);
void x86_roundss(void *, struct exec_data);
void x86_rsm(void *, struct exec_data);
void x86_rsqrtps(void *, struct exec_data);
void x86_rsqrtss(void *, struct exec_data);
void x86_sahf(void *, struct exec_data);
void x86_sal(void *, struct exec_data);
void x86_sar(void *, struct exec_data);
void x86_shl(void *, struct exec_data);
void x86_shr(void *, struct exec_data);
void x86_sbb(void *, struct exec_data);
void x86_scas(void *, struct exec_data);
void x86_mm_setcc(void *, struct exec_data);
void x86_sfence(void *, struct exec_data);
void x86_sgdt(void *, struct exec_data);
void x86_sha1rnds4(void *, struct exec_data);
void x86_sha1nexte(void *, struct exec_data);
void x86_sha1msg1(void *, struct exec_data);
void x86_sha1msg2(void *, struct exec_data);
void x86_sha256rnds2(void *, struct exec_data);
void x86_sha256msg1(void *, struct exec_data);
void x86_sha256msg2(void *, struct exec_data);
void x86_shld(void *, struct exec_data);
void x86_shrd(void *, struct exec_data);
void x86_shufpd(void *, struct exec_data);
void x86_sidt(void *, struct exec_data);
void x86_sldt(void *, struct exec_data);
void x86_smsw(void *, struct exec_data);
void x86_sqrtpd(void *, struct exec_data);
void x86_sqrtps(void *, struct exec_data);
void x86_sqrtsd(void *, struct exec_data);
void x86_sqrtss(void *, struct exec_data);
void x86_stac(void *, struct exec_data);
void x86_stc(void *, struct exec_data);
void x86_std(void *, struct exec_data);
void x86_sti(void *, struct exec_data);
void x86_stos(void *, struct exec_data);
void x86_str(void *, struct exec_data);
void x86_mm_sub(void *, struct exec_data);
void x86_subpd(void *, struct exec_data);
void x86_subps(void *, struct exec_data);
void x86_subsd(void *, struct exec_data);
void x86_subss(void *, struct exec_data);
void x86_swapgs(void *, struct exec_data);
void x86_syscall(void *, struct exec_data);
void x86_sysenter(void *, struct exec_data);
void x86_sysexit(void *, struct exec_data);
void x86_sysret(void *, struct exec_data);
void x86_mm_test(void *, struct exec_data);
void x86_tzcnt(void *, struct exec_data);
void x86_ucomisd(void *, struct exec_data);
void x86_ucomiss(void *, struct exec_data);
void x86_ud0(void *, struct exec_data);
void x86_ud1(void *, struct exec_data);
void x86_ud2(void *, struct exec_data);
void x86_unpckhpd(void *, struct exec_data);
void x86_unpckhps(void *, struct exec_data);
void x86_unpcklpd(void *, struct exec_data);
void x86_unpcklps(void *, struct exec_data);
void x86_verr(void *, struct exec_data);
void x86_verw(void *, struct exec_data);
void x86_wait(void *, struct exec_data);
void x86_wbinvd(void *, struct exec_data);
void x86_wrmsr(void *, struct exec_data);
void x86_wrpkru(void *, struct exec_data);
void x86_xabort(void *, struct exec_data);
void x86_xadd(void *, struct exec_data);
void x86_xbegin(void *, struct exec_data);
void x86_xchg(void *, struct exec_data);
void x86_xend(void *, struct exec_data);
void x86_xgetbv(void *, struct exec_data);
void x86_xlat(void *, struct exec_data);
void x86_mm_xor(void *, struct exec_data);
void x86_xorpd(void *, struct exec_data);
void x86_xorps(void *, struct exec_data);
void x86_xsetbv(void *, struct exec_data);
void x86_xtest(void *, struct exec_data);

#endif /* X86_INSTR_H */
