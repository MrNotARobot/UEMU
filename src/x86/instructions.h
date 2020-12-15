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

#define MODRM_RM_BIT_MASK 0b00000111
#define MODRM_REG_BIT_MASK 0b00111000
#define MODRM_MOD_BIT_MASK 0b11000000

struct instruction {
};

typedef void (*d_x86_instruction_handler)(void *, struct instruction *);

struct opcode {
    const char *o_name;
    d_x86_instruction_handler o_handler;
    int o_class;
    int o_encoding;             // encoding in the 32-bit addressing mode
    int o_encoding16bit;   // encoding in the 16-bit addressing mode
    uint8_t o_opcode;
    unsigned int o_use_rm : 1;
    unsigned int o_is_prefix : 1;
    unsigned int o_use_op_extension : 1;
    struct opcode *o_sec_table;
    size_t o_sec_tablesz;
    struct opcode *o_extensions;
    struct opcode *o_prefix;
};

enum voidIDFeatureFlags {
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
    r16_m32_16,
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
    r32_m32_16,
    r32_rm32,
    r32_rm8,
    r32_rm16,
    r32_r32m16,
    r32_rm32_imm8,
    r32_rm32_imm32,
    r32_sib,
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

void x86_aaa(void *, struct instruction *);
void x86_aad(void *, struct instruction *);
void x86_add(void *, struct instruction *);
void x86_aam(void *, struct instruction *);
void x86_aas(void *, struct instruction *);
void x86_adc(void *, struct instruction *);
void x86_adcx(void *, struct instruction *);
void x86_addpd(void *, struct instruction *);
void x86_addps(void *, struct instruction *);
void x86_addsd(void *, struct instruction *);
void x86_addss(void *, struct instruction *);
void x86_addsubpd(void *, struct instruction *);
void x86_addsubps(void *, struct instruction *);
void x86_adox(void *, struct instruction *);
void x86_aesdec(void *, struct instruction *);
void x86_aesdeclast(void *, struct instruction *);
void x86_aesenc(void *, struct instruction *);
void x86_aesenclast(void *, struct instruction *);
void x86_aesimc(void *, struct instruction *);
void x86_aeskeygenassist(void *, struct instruction *);
void x86_and(void *, struct instruction *);
void x86_andpd(void *, struct instruction *);
void x86_andps(void *, struct instruction *);
void x86_andnpd(void *, struct instruction *);
void x86_andnps(void *, struct instruction *);
void x86_arpl(void *, struct instruction *);
void x86_blendpd(void *, struct instruction *);
void x86_blendps(void *, struct instruction *);
void x86_blendvpd(void *, struct instruction *);
void x86_blendvps(void *, struct instruction *);
void x86_bndcl(void *, struct instruction *);
void x86_bndcu(void *, struct instruction *);
void x86_bndcn(void *, struct instruction *);
void x86_bndldx(void *, struct instruction *);
void x86_bndmk(void *, struct instruction *);
void x86_bndmov(void *, struct instruction *);
void x86_bndstx(void *, struct instruction *);
void x86_bound(void *, struct instruction *);
void x86_bsf(void *, struct instruction *);
void x86_bsr(void *, struct instruction *);
void x86_bswap(void *, struct instruction *);
void x86_bt(void *, struct instruction *);
void x86_btc(void *, struct instruction *);
void x86_btr(void *, struct instruction *);
void x86_bts(void *, struct instruction *);
void x86_call(void *, struct instruction *);
void x86_cbw(void *, struct instruction *);
void x86_clac(void *, struct instruction *);
void x86_clc(void *, struct instruction *);
void x86_cld(void *, struct instruction *);
void x86_clflush(void *, struct instruction *);
void x86_cli(void *, struct instruction *);
void x86_clts(void *, struct instruction *);
void x86_cmc(void *, struct instruction *);
void x86_cmovcc(void *, struct instruction *);
void x86_cmp(void *, struct instruction *);
void x86_cmppd(void *, struct instruction *);
void x86_cmpps(void *, struct instruction *);
void x86_cmps(void *, struct instruction *);
void x86_cmpsd(void *, struct instruction *);
void x86_cmpss(void *, struct instruction *);
void x86_cmpxchg(void *, struct instruction *);
void x86_cmpxchg8b(void *, struct instruction *);
void x86_comisd(void *, struct instruction *);
void x86_comiss(void *, struct instruction *);
void x86_cpuid(void *, struct instruction *);
void x86_crc32(void *, struct instruction *);
void x86_cvtdq2pd(void *, struct instruction *);
void x86_cvtdq2ps(void *, struct instruction *);
void x86_cvtpd2qd(void *, struct instruction *);
void x86_cvtpd2pi(void *, struct instruction *);
void x86_cvtpd2ps(void *, struct instruction *);
void x86_cvtpi2pd(void *, struct instruction *);
void x86_cvtpi2ps(void *, struct instruction *);
void x86_cvtps2dq(void *, struct instruction *);
void x86_cvtps2pd(void *, struct instruction *);
void x86_cvtps2pi(void *, struct instruction *);
void x86_cvtsd2si(void *, struct instruction *);
void x86_cvtsd2ss(void *, struct instruction *);
void x86_cvtsi2sd(void *, struct instruction *);
void x86_cvtsi2ss(void *, struct instruction *);
void x86_cvtss2sd(void *, struct instruction *);
void x86_cvtss2si(void *, struct instruction *);
void x86_cvttpd2dq(void *, struct instruction *);
void x86_cvttpd2pi(void *, struct instruction *);
void x86_cvttps2dq(void *, struct instruction *);
void x86_cvttps2pi(void *, struct instruction *);
void x86_cvttsd2si(void *, struct instruction *);
void x86_cvttss2si(void *, struct instruction *);
void x86_cwq(void *, struct instruction *);
void x86_daa(void *, struct instruction *);
void x86_das(void *, struct instruction *);
void x86_dec(void *, struct instruction *);
void x86_div(void *, struct instruction *);
void x86_divpd(void *, struct instruction *);
void x86_divps(void *, struct instruction *);
void x86_divsd(void *, struct instruction *);
void x86_divss(void *, struct instruction *);
void x86_dppd(void *, struct instruction *);
void x86_dpps(void *, struct instruction *);
void x86_emms(void *, struct instruction *);
void x86_enter(void *, struct instruction *);
void x86_extractps(void *, struct instruction *);
void x86_f2xm1(void *, struct instruction *);
void x86_fabs(void *, struct instruction *);
void x86_fadd(void *, struct instruction *);
void x86_faddp(void *, struct instruction *);
void x86_fiadd(void *, struct instruction *);
void x86_fbld(void *, struct instruction *);
void x86_fbstp(void *, struct instruction *);
void x86_fchs(void *, struct instruction *);
void x86_fnclex(void *, struct instruction *);
void x86_fcmovcc(void *, struct instruction *);
void x86_fcom(void *, struct instruction *);
void x86_fcomp(void *, struct instruction *);
void x86_fcompp(void *, struct instruction *);
void x86_fcomi(void *, struct instruction *);
void x86_fcomip(void *, struct instruction *);
void x86_fucomi(void *, struct instruction *);
void x86_fucomip(void *, struct instruction *);
void x86_fcos(void *, struct instruction *);
void x86_fdecstp(void *, struct instruction *);
void x86_fdiv(void *, struct instruction *);
void x86_fdivp(void *, struct instruction *);
void x86_fidiv(void *, struct instruction *);
void x86_fdivr(void *, struct instruction *);
void x86_fdivrp(void *, struct instruction *);
void x86_fdivir(void *, struct instruction *);
void x86_ffree(void *, struct instruction *);
void x86_ficom(void *, struct instruction *);
void x86_ficomp(void *, struct instruction *);
void x86_fild(void *, struct instruction *);
void x86_fincstp(void *, struct instruction *);
void x86_fclex(void *, struct instruction *);
void x86_finit(void *, struct instruction *);
void x86_fininit(void *, struct instruction *);
void x86_fist(void *, struct instruction *);
void x86_fistp(void *, struct instruction *);
void x86_fisttp(void *, struct instruction *);
void x86_fld(void *, struct instruction *);
void x86_fld1(void *, struct instruction *);
void x86_fldl2t(void *, struct instruction *);
void x86_fldl2e(void *, struct instruction *);
void x86_fldpi(void *, struct instruction *);
void x86_fldlg2(void *, struct instruction *);
void x86_fldln2(void *, struct instruction *);
void x86_fldz(void *, struct instruction *);
void x86_fldcw(void *, struct instruction *);
void x86_fldenv(void *, struct instruction *);
void x86_fmul(void *, struct instruction *);
void x86_fmulp(void *, struct instruction *);
void x86_fimul(void *, struct instruction *);
void x86_fnop(void *, struct instruction *);
void x86_fpatan(void *, struct instruction *);
void x86_fprem(void *, struct instruction *);
void x86_fprem1(void *, struct instruction *);
void x86_fptan(void *, struct instruction *);
void x86_frndint(void *, struct instruction *);
void x86_frstor(void *, struct instruction *);
void x86_fsave(void *, struct instruction *);
void x86_fnsave(void *, struct instruction *);
void x86_fscale(void *, struct instruction *);
void x86_fsin(void *, struct instruction *);
void x86_fsincos(void *, struct instruction *);
void x86_fsqrt(void *, struct instruction *);
void x86_fst(void *, struct instruction *);
void x86_fstp(void *, struct instruction *);
void x86_fnstcw(void *, struct instruction *);
void x86_fstenv(void *, struct instruction *);
void x86_fnstenv(void *, struct instruction *);
void x86_fstsw(void *, struct instruction *);
void x86_fnstsw(void *, struct instruction *);
void x86_fsub(void *, struct instruction *);
void x86_fsubp(void *, struct instruction *);
void x86_fisub(void *, struct instruction *);
void x86_fsubr(void *, struct instruction *);
void x86_fsubrp(void *, struct instruction *);
void x86_fisubr(void *, struct instruction *);
void x86_ftst(void *, struct instruction *);
void x86_fucom(void *, struct instruction *);
void x86_fucomp(void *, struct instruction *);
void x86_fucompp(void *, struct instruction *);
void x86_fxam(void *, struct instruction *);
void x86_fxch(void *, struct instruction *);
void x86_fxrstor(void *, struct instruction *);
void x86_fxsave(void *, struct instruction *);
void x86_fxtract(void *, struct instruction *);
void x86_fyl2x(void *, struct instruction *);
void x86_fyl2xp1(void *, struct instruction *);
void x86_haddpd(void *, struct instruction *);
void x86_haddps(void *, struct instruction *);
void x86_hlt(void *, struct instruction *);
void x86_hsubpd(void *, struct instruction *);
void x86_hsubps(void *, struct instruction *);
void x86_idiv(void *, struct instruction *);
void x86_imul(void *, struct instruction *);
void x86_in(void *, struct instruction *);
void x86_inc(void *, struct instruction *);
void x86_ins(void *, struct instruction *);
void x86_insertps(void *, struct instruction *);
void x86_int3(void *, struct instruction *);
void x86_int(void *, struct instruction *);
void x86_int0(void *, struct instruction *);
void x86_int1(void *, struct instruction *);
void x86_invd(void *, struct instruction *);
void x86_invlpg(void *, struct instruction *);
void x86_iret(void *, struct instruction *);
void x86_jcc(void *, struct instruction *);
void x86_jmp(void *, struct instruction *);
void x86_lahf(void *, struct instruction *);
void x86_lar(void *, struct instruction *);
void x86_lddqu(void *, struct instruction *);
void x86_ldmxcsr(void *, struct instruction *);
void x86_lds(void *, struct instruction *);
void x86_lss(void *, struct instruction *);
void x86_les(void *, struct instruction *);
void x86_lfs(void *, struct instruction *);
void x86_lgs(void *, struct instruction *);
void x86_lea(void *, struct instruction *);
void x86_leave(void *, struct instruction *);
void x86_lfence(void *, struct instruction *);
void x86_lgdt(void *, struct instruction *);
void x86_lidt(void *, struct instruction *);
void x86_lldt(void *, struct instruction *);
void x86_lmsw(void *, struct instruction *);
void x86_lods(void *, struct instruction *);
void x86_loopcc(void *, struct instruction *);
void x86_lsl(void *, struct instruction *);
void x86_ltr(void *, struct instruction *);
void x86_lzcnt(void *, struct instruction *);
void x86_maskmovdqu(void *, struct instruction *);
void x86_maskmovq(void *, struct instruction *);
void x86_maxpd(void *, struct instruction *);
void x86_maxps(void *, struct instruction *);
void x86_maxsd(void *, struct instruction *);
void x86_maxss(void *, struct instruction *);
void x86_mfence(void *, struct instruction *);
void x86_minpd(void *, struct instruction *);
void x86_minps(void *, struct instruction *);
void x86_minsd(void *, struct instruction *);
void x86_minss(void *, struct instruction *);
void x86_monitor(void *, struct instruction *);
void x86_mov(void *, struct instruction *);
void x86_movapd(void *, struct instruction *);
void x86_movaps(void *, struct instruction *);
void x86_movbe(void *, struct instruction *);
void x86_movd(void *, struct instruction *);
void x86_movddup(void *, struct instruction *);
void x86_movdqa(void *, struct instruction *);
void x86_movdqu(void *, struct instruction *);
void x86_movdq2q(void *, struct instruction *);
void x86_movhlps(void *, struct instruction *);
void x86_movhpd(void *, struct instruction *);
void x86_movhps(void *, struct instruction *);
void x86_movlpd(void *, struct instruction *);
void x86_movlps(void *, struct instruction *);
void x86_movmskpd(void *, struct instruction *);
void x86_movmskps(void *, struct instruction *);
void x86_movntdqa(void *, struct instruction *);
void x86_movntdq(void *, struct instruction *);
void x86_movnti(void *, struct instruction *);
void x86_movntpd(void *, struct instruction *);
void x86_movntps(void *, struct instruction *);
void x86_movntq(void *, struct instruction *);
void x86_movq(void *, struct instruction *);
void x86_movq2dq(void *, struct instruction *);
void x86_movs(void *, struct instruction *);
void x86_movsd(void *, struct instruction *);
void x86_movshdup(void *, struct instruction *);
void x86_movsldup(void *, struct instruction *);
void x86_movss(void *, struct instruction *);
void x86_movsx(void *, struct instruction *);
void x86_movupd(void *, struct instruction *);
void x86_movups(void *, struct instruction *);
void x86_movzx(void *, struct instruction *);
void x86_mpsadbw(void *, struct instruction *);
void x86_mul(void *, struct instruction *);
void x86_mulpd(void *, struct instruction *);
void x86_mulps(void *, struct instruction *);
void x86_mulsd(void *, struct instruction *);
void x86_mulss(void *, struct instruction *);
void x86_mwait(void *, struct instruction *);
void x86_neg(void *, struct instruction *);
void x86_nop(void *, struct instruction *);
void x86_not(void *, struct instruction *);
void x86_or(void *, struct instruction *);
void x86_orpd(void *, struct instruction *);
void x86_orps(void *, struct instruction *);
void x86_out(void *, struct instruction *);
void x86_outs(void *, struct instruction *);
void x86_pabsb(void *, struct instruction *);
void x86_pabsw(void *, struct instruction *);
void x86_pabsd(void *, struct instruction *);
void x86_packsswb(void *, struct instruction *);
void x86_packssdw(void *, struct instruction *);
void x86_packusdw(void *, struct instruction *);
void x86_packuswb(void *, struct instruction *);
void x86_paddb(void *, struct instruction *);
void x86_paddw(void *, struct instruction *);
void x86_paddd(void *, struct instruction *);
void x86_paddq(void *, struct instruction *);
void x86_paddsb(void *, struct instruction *);
void x86_paddsw(void *, struct instruction *);
void x86_paddusb(void *, struct instruction *);
void x86_paddusw(void *, struct instruction *);
void x86_palignr(void *, struct instruction *);
void x86_pand(void *, struct instruction *);
void x86_pandn(void *, struct instruction *);
void x86_pause(void *, struct instruction *);
void x86_pavgb(void *, struct instruction *);
void x86_pavgw(void *, struct instruction *);
void x86_pblendvb(void *, struct instruction *);
void x86_pblendw(void *, struct instruction *);
void x86_pclmulqdq(void *, struct instruction *);
void x86_pcmpeqb(void *, struct instruction *);
void x86_pcmpeqw(void *, struct instruction *);
void x86_pcmpeqd(void *, struct instruction *);
void x86_pcmpeqq(void *, struct instruction *);
void x86_pcmpestri(void *, struct instruction *);
void x86_pcmpestrm(void *, struct instruction *);
void x86_pcmpgtb(void *, struct instruction *);
void x86_pcmpgtw(void *, struct instruction *);
void x86_pcmpgtd(void *, struct instruction *);
void x86_pcmpgtq(void *, struct instruction *);
void x86_pcmpistri(void *, struct instruction *);
void x86_pcmpistrm(void *, struct instruction *);
void x86_pextrb(void *, struct instruction *);
void x86_pextrd(void *, struct instruction *);
void x86_pextrw(void *, struct instruction *);
void x86_phaddw(void *, struct instruction *);
void x86_phaddd(void *, struct instruction *);
void x86_phaddsw(void *, struct instruction *);
void x86_phminposuw(void *, struct instruction *);
void x86_phsubw(void *, struct instruction *);
void x86_phsubd(void *, struct instruction *);
void x86_phsubsw(void *, struct instruction *);
void x86_pinsrb(void *, struct instruction *);
void x86_pinsrd(void *, struct instruction *);
void x86_pinsrw(void *, struct instruction *);
void x86_pmaddubsw(void *, struct instruction *);
void x86_pmaddwd(void *, struct instruction *);
void x86_pmaxsb(void *, struct instruction *);
void x86_pmaxsw(void *, struct instruction *);
void x86_pmaxsd(void *, struct instruction *);
void x86_pmaxub(void *, struct instruction *);
void x86_pmaxuw(void *, struct instruction *);
void x86_pmaxud(void *, struct instruction *);
void x86_pminsb(void *, struct instruction *);
void x86_pminsw(void *, struct instruction *);
void x86_pminsd(void *, struct instruction *);
void x86_pminub(void *, struct instruction *);
void x86_pminuw(void *, struct instruction *);
void x86_pminud(void *, struct instruction *);
void x86_pmovmskb(void *, struct instruction *);
void x86_pmovsx(void *, struct instruction *);
void x86_pmovzx(void *, struct instruction *);
void x86_pmuldq(void *, struct instruction *);
void x86_pmulhrsw(void *, struct instruction *);
void x86_pmulhuw(void *, struct instruction *);
void x86_pmulhw(void *, struct instruction *);
void x86_pmulld(void *, struct instruction *);
void x86_pmullw(void *, struct instruction *);
void x86_pmuludq(void *, struct instruction *);
void x86_pop(void *, struct instruction *);
void x86_popa(void *, struct instruction *);
void x86_popcnt(void *, struct instruction *);
void x86_popf(void *, struct instruction *);
void x86_por(void *, struct instruction *);
void x86_prefetcht0(void *, struct instruction *);
void x86_prefetcht1(void *, struct instruction *);
void x86_prefetcht2(void *, struct instruction *);
void x86_prefetchnta(void *, struct instruction *);
void x86_prefetchw(void *, struct instruction *);
void x86_psadbw(void *, struct instruction *);
void x86_pshufb(void *, struct instruction *);
void x86_pshufd(void *, struct instruction *);
void x86_pshufhw(void *, struct instruction *);
void x86_pshuflw(void *, struct instruction *);
void x86_pshufw(void *, struct instruction *);
void x86_psignb(void *, struct instruction *);
void x86_psignw(void *, struct instruction *);
void x86_psignd(void *, struct instruction *);
void x86_psllw(void *, struct instruction *);
void x86_pslld(void *, struct instruction *);
void x86_psllq(void *, struct instruction *);
void x86_psubb(void *, struct instruction *);
void x86_psubw(void *, struct instruction *);
void x86_psubd(void *, struct instruction *);
void x86_psubq(void *, struct instruction *);
void x86_psubsb(void *, struct instruction *);
void x86_psubsw(void *, struct instruction *);
void x86_psubusb(void *, struct instruction *);
void x86_psubusw(void *, struct instruction *);
void x86_ptest(void *, struct instruction *);
void x86_punpckhbw(void *, struct instruction *);
void x86_punpckhwd(void *, struct instruction *);
void x86_punpckhdq(void *, struct instruction *);
void x86_punpckhqdq(void *, struct instruction *);
void x86_punpcklbw(void *, struct instruction *);
void x86_punpcklwd(void *, struct instruction *);
void x86_punpckldq(void *, struct instruction *);
void x86_punpcklqdq(void *, struct instruction *);
void x86_push(void *, struct instruction *);
void x86_pusha(void *, struct instruction *);
void x86_pushf(void *, struct instruction *);
void x86_pxor(void *, struct instruction *);
void x86_rcl(void *, struct instruction *);
void x86_rcr(void *, struct instruction *);
void x86_rol(void *, struct instruction *);
void x86_ror(void *, struct instruction *);
void x86_rcpps(void *, struct instruction *);
void x86_rcpss(void *, struct instruction *);
void x86_rdmsr(void *, struct instruction *);
void x86_rdpkru(void *, struct instruction *);
void x86_rdpmc(void *, struct instruction *);
void x86_rdtsc(void *, struct instruction *);
void x86_rdtscp(void *, struct instruction *);
void x86_ret(void *, struct instruction *);
void x86_roundpd(void *, struct instruction *);
void x86_roundps(void *, struct instruction *);
void x86_roundsd(void *, struct instruction *);
void x86_roundss(void *, struct instruction *);
void x86_rsm(void *, struct instruction *);
void x86_rsqrtps(void *, struct instruction *);
void x86_rsqrtss(void *, struct instruction *);
void x86_sahf(void *, struct instruction *);
void x86_sal(void *, struct instruction *);
void x86_sar(void *, struct instruction *);
void x86_shl(void *, struct instruction *);
void x86_shr(void *, struct instruction *);
void x86_sbb(void *, struct instruction *);
void x86_scas(void *, struct instruction *);
void x86_setcc(void *, struct instruction *);
void x86_sfence(void *, struct instruction *);
void x86_sgdt(void *, struct instruction *);
void x86_sha1rnds4(void *, struct instruction *);
void x86_sha1nexte(void *, struct instruction *);
void x86_sha1msg1(void *, struct instruction *);
void x86_sha1msg2(void *, struct instruction *);
void x86_sha256rnds2(void *, struct instruction *);
void x86_sha256msg1(void *, struct instruction *);
void x86_sha256msg2(void *, struct instruction *);
void x86_shld(void *, struct instruction *);
void x86_shrd(void *, struct instruction *);
void x86_shufpd(void *, struct instruction *);
void x86_sidt(void *, struct instruction *);
void x86_sldt(void *, struct instruction *);
void x86_smsw(void *, struct instruction *);
void x86_sqrtpd(void *, struct instruction *);
void x86_sqrtps(void *, struct instruction *);
void x86_sqrtsd(void *, struct instruction *);
void x86_sqrtss(void *, struct instruction *);
void x86_stac(void *, struct instruction *);
void x86_stc(void *, struct instruction *);
void x86_std(void *, struct instruction *);
void x86_sti(void *, struct instruction *);
void x86_stos(void *, struct instruction *);
void x86_str(void *, struct instruction *);
void x86_sub(void *, struct instruction *);
void x86_subpd(void *, struct instruction *);
void x86_subps(void *, struct instruction *);
void x86_subsd(void *, struct instruction *);
void x86_subss(void *, struct instruction *);
void x86_swapgs(void *, struct instruction *);
void x86_syscall(void *, struct instruction *);
void x86_sysenter(void *, struct instruction *);
void x86_sysexit(void *, struct instruction *);
void x86_sysret(void *, struct instruction *);
void x86_test(void *, struct instruction *);
void x86_tzcnt(void *, struct instruction *);
void x86_ucomisd(void *, struct instruction *);
void x86_ucomiss(void *, struct instruction *);
void x86_ud0(void *, struct instruction *);
void x86_ud1(void *, struct instruction *);
void x86_ud2(void *, struct instruction *);
void x86_unpckhpd(void *, struct instruction *);
void x86_unpckhps(void *, struct instruction *);
void x86_unpcklpd(void *, struct instruction *);
void x86_unpcklps(void *, struct instruction *);
void x86_verr(void *, struct instruction *);
void x86_verw(void *, struct instruction *);
void x86_wait(void *, struct instruction *);
void x86_wbinvd(void *, struct instruction *);
void x86_wrmsr(void *, struct instruction *);
void x86_wrpkru(void *, struct instruction *);
void x86_xabort(void *, struct instruction *);
void x86_xadd(void *, struct instruction *);
void x86_xbegin(void *, struct instruction *);
void x86_xchg(void *, struct instruction *);
void x86_xend(void *, struct instruction *);
void x86_xgetbv(void *, struct instruction *);
void x86_xlat(void *, struct instruction *);
void x86_xor(void *, struct instruction *);
void x86_xorpd(void *, struct instruction *);
void x86_xorps(void *, struct instruction *);
void x86_xsetbv(void *, struct instruction *);
void x86_xtest(void *, struct instruction *);

#endif /* X86_INSTR_H */
