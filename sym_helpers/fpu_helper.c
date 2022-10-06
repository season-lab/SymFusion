/*
 *  x86 FPU, MMX/3DNow!/SSE/SSE2/SSE3/SSSE3/SSE4/PNI helpers
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* HYBRID */
#include "qemu-isolate-build.h"
/* HYBRID */

#include "qemu/osdep.h"
#include <math.h>
#include "cpu.h"
#include "exec/helper-proto.h"
#include "qemu/host-utils.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "fpu/softfloat.h"

/* HYBRID */
#include "sym_helpers.h"

// rename helpers to avoid conflicts with QEMU
// NOTE: other functions must be static!

#define helper_psrlw_mmx SYM(helper_psrlw_mmx)
#define helper_psraw_mmx SYM(helper_psraw_mmx)
#define helper_psllw_mmx SYM(helper_psllw_mmx)
#define helper_psrld_mmx SYM(helper_psrld_mmx)
#define helper_psrad_mmx SYM(helper_psrad_mmx)
#define helper_pslld_mmx SYM(helper_pslld_mmx)
#define helper_psrlq_mmx SYM(helper_psrlq_mmx)
#define helper_psllq_mmx SYM(helper_psllq_mmx)
#define helper_paddb_mmx SYM(helper_paddb_mmx)
#define helper_paddw_mmx SYM(helper_paddw_mmx)
#define helper_paddl_mmx SYM(helper_paddl_mmx)
#define helper_paddq_mmx SYM(helper_paddq_mmx)
#define helper_psubb_mmx SYM(helper_psubb_mmx)
#define helper_psubw_mmx SYM(helper_psubw_mmx)
#define helper_psubl_mmx SYM(helper_psubl_mmx)
#define helper_psubq_mmx SYM(helper_psubq_mmx)
#define helper_paddusb_mmx SYM(helper_paddusb_mmx)
#define helper_paddsb_mmx SYM(helper_paddsb_mmx)
#define helper_psubusb_mmx SYM(helper_psubusb_mmx)
#define helper_psubsb_mmx SYM(helper_psubsb_mmx)
#define helper_paddusw_mmx SYM(helper_paddusw_mmx)
#define helper_paddsw_mmx SYM(helper_paddsw_mmx)
#define helper_psubusw_mmx SYM(helper_psubusw_mmx)
#define helper_psubsw_mmx SYM(helper_psubsw_mmx)
#define helper_pminub_mmx SYM(helper_pminub_mmx)
#define helper_pmaxub_mmx SYM(helper_pmaxub_mmx)
#define helper_pminsw_mmx SYM(helper_pminsw_mmx)
#define helper_pmaxsw_mmx SYM(helper_pmaxsw_mmx)
#define helper_pand_mmx SYM(helper_pand_mmx)
#define helper_pandn_mmx SYM(helper_pandn_mmx)
#define helper_por_mmx SYM(helper_por_mmx)
#define helper_pxor_mmx SYM(helper_pxor_mmx)
#define helper_pcmpgtb_mmx SYM(helper_pcmpgtb_mmx)
#define helper_pcmpgtw_mmx SYM(helper_pcmpgtw_mmx)
#define helper_pcmpgtl_mmx SYM(helper_pcmpgtl_mmx)
#define helper_pcmpeqb_mmx SYM(helper_pcmpeqb_mmx)
#define helper_pcmpeqw_mmx SYM(helper_pcmpeqw_mmx)
#define helper_pcmpeql_mmx SYM(helper_pcmpeql_mmx)
#define helper_pmullw_mmx SYM(helper_pmullw_mmx)
#define helper_pmulhrw_mmx SYM(helper_pmulhrw_mmx)
#define helper_pmulhuw_mmx SYM(helper_pmulhuw_mmx)
#define helper_pmulhw_mmx SYM(helper_pmulhw_mmx)
#define helper_pavgb_mmx SYM(helper_pavgb_mmx)
#define helper_pavgw_mmx SYM(helper_pavgw_mmx)
#define helper_pmuludq_mmx SYM(helper_pmuludq_mmx)
#define helper_pmaddwd_mmx SYM(helper_pmaddwd_mmx)
#define helper_psadbw_mmx SYM(helper_psadbw_mmx)
#define helper_maskmov_mmx SYM(helper_maskmov_mmx)
#define helper_movl_mm_T0_mmx SYM(helper_movl_mm_T0_mmx)
#define helper_movq_mm_T0_mmx SYM(helper_movq_mm_T0_mmx)
#define helper_pshufw_mmx SYM(helper_pshufw_mmx)
#define helper_pmovmskb_mmx SYM(helper_pmovmskb_mmx)
#define helper_packsswb_mmx SYM(helper_packsswb_mmx)
#define helper_packuswb_mmx SYM(helper_packuswb_mmx)
#define helper_packssdw_mmx SYM(helper_packssdw_mmx)
#define helper_punpcklbw_mmx SYM(helper_punpcklbw_mmx)
#define helper_punpcklwd_mmx SYM(helper_punpcklwd_mmx)
#define helper_punpckldq_mmx SYM(helper_punpckldq_mmx)
#define helper_punpckhbw_mmx SYM(helper_punpckhbw_mmx)
#define helper_punpckhwd_mmx SYM(helper_punpckhwd_mmx)
#define helper_punpckhdq_mmx SYM(helper_punpckhdq_mmx)
#define helper_pi2fd SYM(helper_pi2fd)
#define helper_pi2fw SYM(helper_pi2fw)
#define helper_pf2id SYM(helper_pf2id)
#define helper_pf2iw SYM(helper_pf2iw)
#define helper_pfacc SYM(helper_pfacc)
#define helper_pfadd SYM(helper_pfadd)
#define helper_pfcmpeq SYM(helper_pfcmpeq)
#define helper_pfcmpge SYM(helper_pfcmpge)
#define helper_pfcmpgt SYM(helper_pfcmpgt)
#define helper_pfmax SYM(helper_pfmax)
#define helper_pfmin SYM(helper_pfmin)
#define helper_pfmul SYM(helper_pfmul)
#define helper_pfnacc SYM(helper_pfnacc)
#define helper_pfpnacc SYM(helper_pfpnacc)
#define helper_pfrcp SYM(helper_pfrcp)
#define helper_pfrsqrt SYM(helper_pfrsqrt)
#define helper_pfsub SYM(helper_pfsub)
#define helper_pfsubr SYM(helper_pfsubr)
#define helper_pswapd SYM(helper_pswapd)
#define helper_pshufb_mmx SYM(helper_pshufb_mmx)
#define helper_phaddw_mmx SYM(helper_phaddw_mmx)
#define helper_phaddd_mmx SYM(helper_phaddd_mmx)
#define helper_phaddsw_mmx SYM(helper_phaddsw_mmx)
#define helper_pmaddubsw_mmx SYM(helper_pmaddubsw_mmx)
#define helper_phsubw_mmx SYM(helper_phsubw_mmx)
#define helper_phsubd_mmx SYM(helper_phsubd_mmx)
#define helper_phsubsw_mmx SYM(helper_phsubsw_mmx)
#define helper_pabsb_mmx SYM(helper_pabsb_mmx)
#define helper_pabsw_mmx SYM(helper_pabsw_mmx)
#define helper_pabsd_mmx SYM(helper_pabsd_mmx)
#define helper_pmulhrsw_mmx SYM(helper_pmulhrsw_mmx)
#define helper_psignb_mmx SYM(helper_psignb_mmx)
#define helper_psignw_mmx SYM(helper_psignw_mmx)
#define helper_psignd_mmx SYM(helper_psignd_mmx)
#define helper_palignr_mmx SYM(helper_palignr_mmx)
#define helper_psrlw_xmm SYM(helper_psrlw_xmm)
#define helper_psraw_xmm SYM(helper_psraw_xmm)
#define helper_psllw_xmm SYM(helper_psllw_xmm)
#define helper_psrld_xmm SYM(helper_psrld_xmm)
#define helper_psrad_xmm SYM(helper_psrad_xmm)
#define helper_pslld_xmm SYM(helper_pslld_xmm)
#define helper_psrlq_xmm SYM(helper_psrlq_xmm)
#define helper_psllq_xmm SYM(helper_psllq_xmm)
#define helper_psrldq_xmm SYM(helper_psrldq_xmm)
#define helper_pslldq_xmm SYM(helper_pslldq_xmm)
#define helper_paddb_xmm SYM(helper_paddb_xmm)
#define helper_paddw_xmm SYM(helper_paddw_xmm)
#define helper_paddl_xmm SYM(helper_paddl_xmm)
#define helper_paddq_xmm SYM(helper_paddq_xmm)
#define helper_psubb_xmm SYM(helper_psubb_xmm)
#define helper_psubw_xmm SYM(helper_psubw_xmm)
#define helper_psubl_xmm SYM(helper_psubl_xmm)
#define helper_psubq_xmm SYM(helper_psubq_xmm)
#define helper_paddusb_xmm SYM(helper_paddusb_xmm)
#define helper_paddsb_xmm SYM(helper_paddsb_xmm)
#define helper_psubusb_xmm SYM(helper_psubusb_xmm)
#define helper_psubsb_xmm SYM(helper_psubsb_xmm)
#define helper_paddusw_xmm SYM(helper_paddusw_xmm)
#define helper_paddsw_xmm SYM(helper_paddsw_xmm)
#define helper_psubusw_xmm SYM(helper_psubusw_xmm)
#define helper_psubsw_xmm SYM(helper_psubsw_xmm)
#define helper_pminub_xmm SYM(helper_pminub_xmm)
#define helper_pmaxub_xmm SYM(helper_pmaxub_xmm)
#define helper_pminsw_xmm SYM(helper_pminsw_xmm)
#define helper_pmaxsw_xmm SYM(helper_pmaxsw_xmm)
#define helper_pand_xmm SYM(helper_pand_xmm)
#define helper_pandn_xmm SYM(helper_pandn_xmm)
#define helper_por_xmm SYM(helper_por_xmm)
#define helper_pxor_xmm SYM(helper_pxor_xmm)
#define helper_pcmpgtb_xmm SYM(helper_pcmpgtb_xmm)
#define helper_pcmpgtw_xmm SYM(helper_pcmpgtw_xmm)
#define helper_pcmpgtl_xmm SYM(helper_pcmpgtl_xmm)
#define helper_pcmpeqb_xmm SYM(helper_pcmpeqb_xmm)
#define helper_pcmpeqw_xmm SYM(helper_pcmpeqw_xmm)
#define helper_pcmpeql_xmm SYM(helper_pcmpeql_xmm)
#define helper_pmullw_xmm SYM(helper_pmullw_xmm)
#define helper_pmulhuw_xmm SYM(helper_pmulhuw_xmm)
#define helper_pmulhw_xmm SYM(helper_pmulhw_xmm)
#define helper_pavgb_xmm SYM(helper_pavgb_xmm)
#define helper_pavgw_xmm SYM(helper_pavgw_xmm)
#define helper_pmuludq_xmm SYM(helper_pmuludq_xmm)
#define helper_pmaddwd_xmm SYM(helper_pmaddwd_xmm)
#define helper_psadbw_xmm SYM(helper_psadbw_xmm)
#define helper_maskmov_xmm SYM(helper_maskmov_xmm)
#define helper_movl_mm_T0_xmm SYM(helper_movl_mm_T0_xmm)
#define helper_movq_mm_T0_xmm SYM(helper_movq_mm_T0_xmm)
#define helper_shufps SYM(helper_shufps)
#define helper_shufpd SYM(helper_shufpd)
#define helper_pshufd_xmm SYM(helper_pshufd_xmm)
#define helper_pshuflw_xmm SYM(helper_pshuflw_xmm)
#define helper_pshufhw_xmm SYM(helper_pshufhw_xmm)
#define helper_addps SYM(helper_addps)
#define helper_addss SYM(helper_addss)
#define helper_addpd SYM(helper_addpd)
#define helper_addsd SYM(helper_addsd)
#define helper_subps SYM(helper_subps)
#define helper_subss SYM(helper_subss)
#define helper_subpd SYM(helper_subpd)
#define helper_subsd SYM(helper_subsd)
#define helper_mulps SYM(helper_mulps)
#define helper_mulss SYM(helper_mulss)
#define helper_mulpd SYM(helper_mulpd)
#define helper_mulsd SYM(helper_mulsd)
#define helper_divps SYM(helper_divps)
#define helper_divss SYM(helper_divss)
#define helper_divpd SYM(helper_divpd)
#define helper_divsd SYM(helper_divsd)
#define helper_minps SYM(helper_minps)
#define helper_minss SYM(helper_minss)
#define helper_minpd SYM(helper_minpd)
#define helper_minsd SYM(helper_minsd)
#define helper_maxps SYM(helper_maxps)
#define helper_maxss SYM(helper_maxss)
#define helper_maxpd SYM(helper_maxpd)
#define helper_maxsd SYM(helper_maxsd)
#define helper_sqrtps SYM(helper_sqrtps)
#define helper_sqrtss SYM(helper_sqrtss)
#define helper_sqrtpd SYM(helper_sqrtpd)
#define helper_sqrtsd SYM(helper_sqrtsd)
#define helper_cvtps2pd SYM(helper_cvtps2pd)
#define helper_cvtpd2ps SYM(helper_cvtpd2ps)
#define helper_cvtss2sd SYM(helper_cvtss2sd)
#define helper_cvtsd2ss SYM(helper_cvtsd2ss)
#define helper_cvtdq2ps SYM(helper_cvtdq2ps)
#define helper_cvtdq2pd SYM(helper_cvtdq2pd)
#define helper_cvtpi2ps SYM(helper_cvtpi2ps)
#define helper_cvtpi2pd SYM(helper_cvtpi2pd)
#define helper_cvtsi2ss SYM(helper_cvtsi2ss)
#define helper_cvtsi2sd SYM(helper_cvtsi2sd)
#define helper_cvtsq2ss SYM(helper_cvtsq2ss)
#define helper_cvtsq2sd SYM(helper_cvtsq2sd)
#define helper_cvtps2dq SYM(helper_cvtps2dq)
#define helper_cvtpd2dq SYM(helper_cvtpd2dq)
#define helper_cvtps2pi SYM(helper_cvtps2pi)
#define helper_cvtpd2pi SYM(helper_cvtpd2pi)
#define helper_cvtss2si SYM(helper_cvtss2si)
#define helper_cvtsd2si SYM(helper_cvtsd2si)
#define helper_cvtss2sq SYM(helper_cvtss2sq)
#define helper_cvtsd2sq SYM(helper_cvtsd2sq)
#define helper_cvttps2dq SYM(helper_cvttps2dq)
#define helper_cvttpd2dq SYM(helper_cvttpd2dq)
#define helper_cvttps2pi SYM(helper_cvttps2pi)
#define helper_cvttpd2pi SYM(helper_cvttpd2pi)
#define helper_cvttss2si SYM(helper_cvttss2si)
#define helper_cvttsd2si SYM(helper_cvttsd2si)
#define helper_cvttss2sq SYM(helper_cvttss2sq)
#define helper_cvttsd2sq SYM(helper_cvttsd2sq)
#define helper_rsqrtps SYM(helper_rsqrtps)
#define helper_rsqrtss SYM(helper_rsqrtss)
#define helper_rcpps SYM(helper_rcpps)
#define helper_rcpss SYM(helper_rcpss)
#define helper_extrq_r SYM(helper_extrq_r)
#define helper_extrq SYM(helper_extrq)
#define helper_extrq_i SYM(helper_extrq_i)
#define helper_insertq_r SYM(helper_insertq_r)
#define helper_insertq SYM(helper_insertq)
#define helper_insertq_i SYM(helper_insertq_i)
#define helper_haddps SYM(helper_haddps)
#define helper_haddpd SYM(helper_haddpd)
#define helper_hsubps SYM(helper_hsubps)
#define helper_hsubpd SYM(helper_hsubpd)
#define helper_addsubps SYM(helper_addsubps)
#define helper_addsubpd SYM(helper_addsubpd)
#define helper_cmpeqps SYM(helper_cmpeqps)
#define helper_cmpeqss SYM(helper_cmpeqss)
#define helper_cmpeqpd SYM(helper_cmpeqpd)
#define helper_cmpeqsd SYM(helper_cmpeqsd)
#define helper_cmpltps SYM(helper_cmpltps)
#define helper_cmpltss SYM(helper_cmpltss)
#define helper_cmpltpd SYM(helper_cmpltpd)
#define helper_cmpltsd SYM(helper_cmpltsd)
#define helper_cmpleps SYM(helper_cmpleps)
#define helper_cmpless SYM(helper_cmpless)
#define helper_cmplepd SYM(helper_cmplepd)
#define helper_cmplesd SYM(helper_cmplesd)
#define helper_cmpunordps SYM(helper_cmpunordps)
#define helper_cmpunordss SYM(helper_cmpunordss)
#define helper_cmpunordpd SYM(helper_cmpunordpd)
#define helper_cmpunordsd SYM(helper_cmpunordsd)
#define helper_cmpneqps SYM(helper_cmpneqps)
#define helper_cmpneqss SYM(helper_cmpneqss)
#define helper_cmpneqpd SYM(helper_cmpneqpd)
#define helper_cmpneqsd SYM(helper_cmpneqsd)
#define helper_cmpnltps SYM(helper_cmpnltps)
#define helper_cmpnltss SYM(helper_cmpnltss)
#define helper_cmpnltpd SYM(helper_cmpnltpd)
#define helper_cmpnltsd SYM(helper_cmpnltsd)
#define helper_cmpnleps SYM(helper_cmpnleps)
#define helper_cmpnless SYM(helper_cmpnless)
#define helper_cmpnlepd SYM(helper_cmpnlepd)
#define helper_cmpnlesd SYM(helper_cmpnlesd)
#define helper_cmpordps SYM(helper_cmpordps)
#define helper_cmpordss SYM(helper_cmpordss)
#define helper_cmpordpd SYM(helper_cmpordpd)
#define helper_cmpordsd SYM(helper_cmpordsd)
#define helper_ucomiss SYM(helper_ucomiss)
#define helper_comiss SYM(helper_comiss)
#define helper_ucomisd SYM(helper_ucomisd)
#define helper_comisd SYM(helper_comisd)
#define helper_movmskps SYM(helper_movmskps)
#define helper_movmskpd SYM(helper_movmskpd)
#define helper_pmovmskb_xmm SYM(helper_pmovmskb_xmm)
#define helper_packsswb_xmm SYM(helper_packsswb_xmm)
#define helper_packuswb_xmm SYM(helper_packuswb_xmm)
#define helper_packssdw_xmm SYM(helper_packssdw_xmm)
#define helper_punpcklbw_xmm SYM(helper_punpcklbw_xmm)
#define helper_punpcklwd_xmm SYM(helper_punpcklwd_xmm)
#define helper_punpckldq_xmm SYM(helper_punpckldq_xmm)
#define helper_punpcklqdq_xmm SYM(helper_punpcklqdq_xmm)
#define helper_punpckhbw_xmm SYM(helper_punpckhbw_xmm)
#define helper_punpckhwd_xmm SYM(helper_punpckhwd_xmm)
#define helper_punpckhdq_xmm SYM(helper_punpckhdq_xmm)
#define helper_punpckhqdq_xmm SYM(helper_punpckhqdq_xmm)
#define helper_pshufb_xmm SYM(helper_pshufb_xmm)
#define helper_phaddw_xmm SYM(helper_phaddw_xmm)
#define helper_phaddd_xmm SYM(helper_phaddd_xmm)
#define helper_phaddsw_xmm SYM(helper_phaddsw_xmm)
#define helper_pmaddubsw_xmm SYM(helper_pmaddubsw_xmm)
#define helper_phsubw_xmm SYM(helper_phsubw_xmm)
#define helper_phsubd_xmm SYM(helper_phsubd_xmm)
#define helper_phsubsw_xmm SYM(helper_phsubsw_xmm)
#define helper_pabsb_xmm SYM(helper_pabsb_xmm)
#define helper_pabsw_xmm SYM(helper_pabsw_xmm)
#define helper_pabsd_xmm SYM(helper_pabsd_xmm)
#define helper_pmulhrsw_xmm SYM(helper_pmulhrsw_xmm)
#define helper_psignb_xmm SYM(helper_psignb_xmm)
#define helper_psignw_xmm SYM(helper_psignw_xmm)
#define helper_psignd_xmm SYM(helper_psignd_xmm)
#define helper_palignr_xmm SYM(helper_palignr_xmm)
#define helper_pblendvb_xmm SYM(helper_pblendvb_xmm)
#define helper_blendvps_xmm SYM(helper_blendvps_xmm)
#define helper_blendvpd_xmm SYM(helper_blendvpd_xmm)
#define helper_ptest_xmm SYM(helper_ptest_xmm)
#define helper_pmovsxbw_xmm SYM(helper_pmovsxbw_xmm)
#define helper_pmovsxbd_xmm SYM(helper_pmovsxbd_xmm)
#define helper_pmovsxbq_xmm SYM(helper_pmovsxbq_xmm)
#define helper_pmovsxwd_xmm SYM(helper_pmovsxwd_xmm)
#define helper_pmovsxwq_xmm SYM(helper_pmovsxwq_xmm)
#define helper_pmovsxdq_xmm SYM(helper_pmovsxdq_xmm)
#define helper_pmovzxbw_xmm SYM(helper_pmovzxbw_xmm)
#define helper_pmovzxbd_xmm SYM(helper_pmovzxbd_xmm)
#define helper_pmovzxbq_xmm SYM(helper_pmovzxbq_xmm)
#define helper_pmovzxwd_xmm SYM(helper_pmovzxwd_xmm)
#define helper_pmovzxwq_xmm SYM(helper_pmovzxwq_xmm)
#define helper_pmovzxdq_xmm SYM(helper_pmovzxdq_xmm)
#define helper_pmuldq_xmm SYM(helper_pmuldq_xmm)
#define helper_pcmpeqq_xmm SYM(helper_pcmpeqq_xmm)
#define helper_packusdw_xmm SYM(helper_packusdw_xmm)
#define helper_pminsb_xmm SYM(helper_pminsb_xmm)
#define helper_pminsd_xmm SYM(helper_pminsd_xmm)
#define helper_pminuw_xmm SYM(helper_pminuw_xmm)
#define helper_pminud_xmm SYM(helper_pminud_xmm)
#define helper_pmaxsb_xmm SYM(helper_pmaxsb_xmm)
#define helper_pmaxsd_xmm SYM(helper_pmaxsd_xmm)
#define helper_pmaxuw_xmm SYM(helper_pmaxuw_xmm)
#define helper_pmaxud_xmm SYM(helper_pmaxud_xmm)
#define helper_pmulld_xmm SYM(helper_pmulld_xmm)
#define helper_phminposuw_xmm SYM(helper_phminposuw_xmm)
#define helper_roundps_xmm SYM(helper_roundps_xmm)
#define helper_roundpd_xmm SYM(helper_roundpd_xmm)
#define helper_roundss_xmm SYM(helper_roundss_xmm)
#define helper_roundsd_xmm SYM(helper_roundsd_xmm)
#define helper_blendps_xmm SYM(helper_blendps_xmm)
#define helper_blendpd_xmm SYM(helper_blendpd_xmm)
#define helper_pblendw_xmm SYM(helper_pblendw_xmm)
#define helper_dpps_xmm SYM(helper_dpps_xmm)
#define helper_dppd_xmm SYM(helper_dppd_xmm)
#define helper_mpsadbw_xmm SYM(helper_mpsadbw_xmm)
#define helper_pcmpgtq_xmm SYM(helper_pcmpgtq_xmm)
#define helper_pcmpestri_xmm SYM(helper_pcmpestri_xmm)
#define helper_pcmpestrm_xmm SYM(helper_pcmpestrm_xmm)
#define helper_pcmpistri_xmm SYM(helper_pcmpistri_xmm)
#define helper_pcmpistrm_xmm SYM(helper_pcmpistrm_xmm)
#define helper_crc32 SYM(helper_crc32)
#define helper_pclmulqdq_xmm SYM(helper_pclmulqdq_xmm)
#define helper_aesdec_xmm SYM(helper_aesdec_xmm)
#define helper_aesdeclast_xmm SYM(helper_aesdeclast_xmm)
#define helper_aesenc_xmm SYM(helper_aesenc_xmm)
#define helper_aesenclast_xmm SYM(helper_aesenclast_xmm)
#define helper_aesimc_xmm SYM(helper_aesimc_xmm)
#define helper_aeskeygenassist_xmm SYM(helper_aeskeygenassist_xmm)
#define set_helper_retaddr SYM(set_helper_retaddr)
#define clear_helper_retaddr SYM(clear_helper_retaddr)

/* HYBRID */

#if 0

#define FPU_RC_MASK         0xc00
#define FPU_RC_NEAR         0x000
#define FPU_RC_DOWN         0x400
#define FPU_RC_UP           0x800
#define FPU_RC_CHOP         0xc00

#define MAXTAN 9223372036854775808.0

/* the following deal with x86 long double-precision numbers */
#define MAXEXPD 0x7fff
#define EXPBIAS 16383
#define EXPD(fp)        (fp.l.upper & 0x7fff)
#define SIGND(fp)       ((fp.l.upper) & 0x8000)
#define MANTD(fp)       (fp.l.lower)
#define BIASEXPONENT(fp) fp.l.upper = (fp.l.upper & ~(0x7fff)) | EXPBIAS

#define FPUS_IE (1 << 0)
#define FPUS_DE (1 << 1)
#define FPUS_ZE (1 << 2)
#define FPUS_OE (1 << 3)
#define FPUS_UE (1 << 4)
#define FPUS_PE (1 << 5)
#define FPUS_SF (1 << 6)
#define FPUS_SE (1 << 7)
#define FPUS_B  (1 << 15)

#define FPUC_EM 0x3f

#define floatx80_lg2 make_floatx80(0x3ffd, 0x9a209a84fbcff799LL)
#define floatx80_l2e make_floatx80(0x3fff, 0xb8aa3b295c17f0bcLL)
#define floatx80_l2t make_floatx80(0x4000, 0xd49a784bcd1b8afeLL)

static inline void fpush(CPUX86State *env)
{
    env->fpstt = (env->fpstt - 1) & 7;
    env->fptags[env->fpstt] = 0; /* validate stack entry */
}

static inline void fpop(CPUX86State *env)
{
    env->fptags[env->fpstt] = 1; /* invalidate stack entry */
    env->fpstt = (env->fpstt + 1) & 7;
}

static inline floatx80 helper_fldt(CPUX86State *env, target_ulong ptr,
                                   uintptr_t retaddr)
{
    CPU_LDoubleU temp;

    temp.l.lower = cpu_ldq_data_ra(env, ptr, retaddr);
    temp.l.upper = cpu_lduw_data_ra(env, ptr + 8, retaddr);
    return temp.d;
}

static inline void helper_fstt(CPUX86State *env, floatx80 f, target_ulong ptr,
                               uintptr_t retaddr)
{
    CPU_LDoubleU temp;

    temp.d = f;
    cpu_stq_data_ra(env, ptr, temp.l.lower, retaddr);
    cpu_stw_data_ra(env, ptr + 8, temp.l.upper, retaddr);
}

/* x87 FPU helpers */

static inline double floatx80_to_double(CPUX86State *env, floatx80 a)
{
    union {
        float64 f64;
        double d;
    } u;

    u.f64 = floatx80_to_float64(a, &env->fp_status);
    return u.d;
}

static inline floatx80 double_to_floatx80(CPUX86State *env, double a)
{
    union {
        float64 f64;
        double d;
    } u;

    u.d = a;
    return float64_to_floatx80(u.f64, &env->fp_status);
}

static void fpu_set_exception(CPUX86State *env, int mask)
{
    env->fpus |= mask;
    if (env->fpus & (~env->fpuc & FPUC_EM)) {
        env->fpus |= FPUS_SE | FPUS_B;
    }
}

static inline floatx80 helper_fdiv(CPUX86State *env, floatx80 a, floatx80 b)
{
    if (floatx80_is_zero(b)) {
        fpu_set_exception(env, FPUS_ZE);
    }
    return floatx80_div(a, b, &env->fp_status);
}

static void fpu_raise_exception(CPUX86State *env, uintptr_t retaddr)
{
    if (env->cr[0] & CR0_NE_MASK) {
        raise_exception_ra(env, EXCP10_COPR, retaddr);
    }
#if !defined(CONFIG_USER_ONLY)
    else {
        cpu_set_ferr(env);
    }
#endif
}

void helper_flds_FT0(CPUX86State *env, uint32_t val)
{
    union {
        float32 f;
        uint32_t i;
    } u;

    u.i = val;
    FT0 = float32_to_floatx80(u.f, &env->fp_status);
}

void helper_fldl_FT0(CPUX86State *env, uint64_t val)
{
    union {
        float64 f;
        uint64_t i;
    } u;

    u.i = val;
    FT0 = float64_to_floatx80(u.f, &env->fp_status);
}

void helper_fildl_FT0(CPUX86State *env, int32_t val)
{
    FT0 = int32_to_floatx80(val, &env->fp_status);
}

void helper_flds_ST0(CPUX86State *env, uint32_t val)
{
    int new_fpstt;
    union {
        float32 f;
        uint32_t i;
    } u;

    new_fpstt = (env->fpstt - 1) & 7;
    u.i = val;
    env->fpregs[new_fpstt].d = float32_to_floatx80(u.f, &env->fp_status);
    env->fpstt = new_fpstt;
    env->fptags[new_fpstt] = 0; /* validate stack entry */
}

void helper_fldl_ST0(CPUX86State *env, uint64_t val)
{
    int new_fpstt;
    union {
        float64 f;
        uint64_t i;
    } u;

    new_fpstt = (env->fpstt - 1) & 7;
    u.i = val;
    env->fpregs[new_fpstt].d = float64_to_floatx80(u.f, &env->fp_status);
    env->fpstt = new_fpstt;
    env->fptags[new_fpstt] = 0; /* validate stack entry */
}

void helper_fildl_ST0(CPUX86State *env, int32_t val)
{
    int new_fpstt;

    new_fpstt = (env->fpstt - 1) & 7;
    env->fpregs[new_fpstt].d = int32_to_floatx80(val, &env->fp_status);
    env->fpstt = new_fpstt;
    env->fptags[new_fpstt] = 0; /* validate stack entry */
}

void helper_fildll_ST0(CPUX86State *env, int64_t val)
{
    int new_fpstt;

    new_fpstt = (env->fpstt - 1) & 7;
    env->fpregs[new_fpstt].d = int64_to_floatx80(val, &env->fp_status);
    env->fpstt = new_fpstt;
    env->fptags[new_fpstt] = 0; /* validate stack entry */
}

uint32_t helper_fsts_ST0(CPUX86State *env)
{
    union {
        float32 f;
        uint32_t i;
    } u;

    u.f = floatx80_to_float32(ST0, &env->fp_status);
    return u.i;
}

uint64_t helper_fstl_ST0(CPUX86State *env)
{
    union {
        float64 f;
        uint64_t i;
    } u;

    u.f = floatx80_to_float64(ST0, &env->fp_status);
    return u.i;
}

int32_t helper_fist_ST0(CPUX86State *env)
{
    int32_t val;

    val = floatx80_to_int32(ST0, &env->fp_status);
    if (val != (int16_t)val) {
        val = -32768;
    }
    return val;
}

int32_t helper_fistl_ST0(CPUX86State *env)
{
    int32_t val;
    signed char old_exp_flags;

    old_exp_flags = get_float_exception_flags(&env->fp_status);
    set_float_exception_flags(0, &env->fp_status);

    val = floatx80_to_int32(ST0, &env->fp_status);
    if (get_float_exception_flags(&env->fp_status) & float_flag_invalid) {
        val = 0x80000000;
    }
    set_float_exception_flags(get_float_exception_flags(&env->fp_status)
                                | old_exp_flags, &env->fp_status);
    return val;
}

int64_t helper_fistll_ST0(CPUX86State *env)
{
    int64_t val;
    signed char old_exp_flags;

    old_exp_flags = get_float_exception_flags(&env->fp_status);
    set_float_exception_flags(0, &env->fp_status);

    val = floatx80_to_int64(ST0, &env->fp_status);
    if (get_float_exception_flags(&env->fp_status) & float_flag_invalid) {
        val = 0x8000000000000000ULL;
    }
    set_float_exception_flags(get_float_exception_flags(&env->fp_status)
                                | old_exp_flags, &env->fp_status);
    return val;
}

int32_t helper_fistt_ST0(CPUX86State *env)
{
    int32_t val;

    val = floatx80_to_int32_round_to_zero(ST0, &env->fp_status);
    if (val != (int16_t)val) {
        val = -32768;
    }
    return val;
}

int32_t helper_fisttl_ST0(CPUX86State *env)
{
    return floatx80_to_int32_round_to_zero(ST0, &env->fp_status);
}

int64_t helper_fisttll_ST0(CPUX86State *env)
{
    return floatx80_to_int64_round_to_zero(ST0, &env->fp_status);
}

void helper_fldt_ST0(CPUX86State *env, target_ulong ptr)
{
    int new_fpstt;

    new_fpstt = (env->fpstt - 1) & 7;
    env->fpregs[new_fpstt].d = helper_fldt(env, ptr, GETPC());
    env->fpstt = new_fpstt;
    env->fptags[new_fpstt] = 0; /* validate stack entry */
}

void helper_fstt_ST0(CPUX86State *env, target_ulong ptr)
{
    helper_fstt(env, ST0, ptr, GETPC());
}

void helper_fpush(CPUX86State *env)
{
    fpush(env);
}

void helper_fpop(CPUX86State *env)
{
    fpop(env);
}

void helper_fdecstp(CPUX86State *env)
{
    env->fpstt = (env->fpstt - 1) & 7;
    env->fpus &= ~0x4700;
}

void helper_fincstp(CPUX86State *env)
{
    env->fpstt = (env->fpstt + 1) & 7;
    env->fpus &= ~0x4700;
}

/* FPU move */

void helper_ffree_STN(CPUX86State *env, int st_index)
{
    env->fptags[(env->fpstt + st_index) & 7] = 1;
}

void helper_fmov_ST0_FT0(CPUX86State *env)
{
    ST0 = FT0;
}

void helper_fmov_FT0_STN(CPUX86State *env, int st_index)
{
    FT0 = ST(st_index);
}

void helper_fmov_ST0_STN(CPUX86State *env, int st_index)
{
    ST0 = ST(st_index);
}

void helper_fmov_STN_ST0(CPUX86State *env, int st_index)
{
    ST(st_index) = ST0;
}

void helper_fxchg_ST0_STN(CPUX86State *env, int st_index)
{
    floatx80 tmp;

    tmp = ST(st_index);
    ST(st_index) = ST0;
    ST0 = tmp;
}

/* FPU operations */

static const int fcom_ccval[4] = {0x0100, 0x4000, 0x0000, 0x4500};

void helper_fcom_ST0_FT0(CPUX86State *env)
{
    int ret;

    ret = floatx80_compare(ST0, FT0, &env->fp_status);
    env->fpus = (env->fpus & ~0x4500) | fcom_ccval[ret + 1];
}

void helper_fucom_ST0_FT0(CPUX86State *env)
{
    int ret;

    ret = floatx80_compare_quiet(ST0, FT0, &env->fp_status);
    env->fpus = (env->fpus & ~0x4500) | fcom_ccval[ret + 1];
}

static const int fcomi_ccval[4] = {CC_C, CC_Z, 0, CC_Z | CC_P | CC_C};

void helper_fcomi_ST0_FT0(CPUX86State *env)
{
    int eflags;
    int ret;

    ret = floatx80_compare(ST0, FT0, &env->fp_status);
    eflags = cpu_cc_compute_all(env, CC_OP);
    eflags = (eflags & ~(CC_Z | CC_P | CC_C)) | fcomi_ccval[ret + 1];
    CC_SRC = eflags;
}

void helper_fucomi_ST0_FT0(CPUX86State *env)
{
    int eflags;
    int ret;

    ret = floatx80_compare_quiet(ST0, FT0, &env->fp_status);
    eflags = cpu_cc_compute_all(env, CC_OP);
    eflags = (eflags & ~(CC_Z | CC_P | CC_C)) | fcomi_ccval[ret + 1];
    CC_SRC = eflags;
}

void helper_fadd_ST0_FT0(CPUX86State *env)
{
    ST0 = floatx80_add(ST0, FT0, &env->fp_status);
}

void helper_fmul_ST0_FT0(CPUX86State *env)
{
    ST0 = floatx80_mul(ST0, FT0, &env->fp_status);
}

void helper_fsub_ST0_FT0(CPUX86State *env)
{
    ST0 = floatx80_sub(ST0, FT0, &env->fp_status);
}

void helper_fsubr_ST0_FT0(CPUX86State *env)
{
    ST0 = floatx80_sub(FT0, ST0, &env->fp_status);
}

void helper_fdiv_ST0_FT0(CPUX86State *env)
{
    ST0 = helper_fdiv(env, ST0, FT0);
}

void helper_fdivr_ST0_FT0(CPUX86State *env)
{
    ST0 = helper_fdiv(env, FT0, ST0);
}

/* fp operations between STN and ST0 */

void helper_fadd_STN_ST0(CPUX86State *env, int st_index)
{
    ST(st_index) = floatx80_add(ST(st_index), ST0, &env->fp_status);
}

void helper_fmul_STN_ST0(CPUX86State *env, int st_index)
{
    ST(st_index) = floatx80_mul(ST(st_index), ST0, &env->fp_status);
}

void helper_fsub_STN_ST0(CPUX86State *env, int st_index)
{
    ST(st_index) = floatx80_sub(ST(st_index), ST0, &env->fp_status);
}

void helper_fsubr_STN_ST0(CPUX86State *env, int st_index)
{
    ST(st_index) = floatx80_sub(ST0, ST(st_index), &env->fp_status);
}

void helper_fdiv_STN_ST0(CPUX86State *env, int st_index)
{
    floatx80 *p;

    p = &ST(st_index);
    *p = helper_fdiv(env, *p, ST0);
}

void helper_fdivr_STN_ST0(CPUX86State *env, int st_index)
{
    floatx80 *p;

    p = &ST(st_index);
    *p = helper_fdiv(env, ST0, *p);
}

/* misc FPU operations */
void helper_fchs_ST0(CPUX86State *env)
{
    ST0 = floatx80_chs(ST0);
}

void helper_fabs_ST0(CPUX86State *env)
{
    ST0 = floatx80_abs(ST0);
}

void helper_fld1_ST0(CPUX86State *env)
{
    ST0 = floatx80_one;
}

void helper_fldl2t_ST0(CPUX86State *env)
{
    ST0 = floatx80_l2t;
}

void helper_fldl2e_ST0(CPUX86State *env)
{
    ST0 = floatx80_l2e;
}

void helper_fldpi_ST0(CPUX86State *env)
{
    ST0 = floatx80_pi;
}

void helper_fldlg2_ST0(CPUX86State *env)
{
    ST0 = floatx80_lg2;
}

void helper_fldln2_ST0(CPUX86State *env)
{
    ST0 = floatx80_ln2;
}

void helper_fldz_ST0(CPUX86State *env)
{
    ST0 = floatx80_zero;
}

void helper_fldz_FT0(CPUX86State *env)
{
    FT0 = floatx80_zero;
}

uint32_t helper_fnstsw(CPUX86State *env)
{
    return (env->fpus & ~0x3800) | (env->fpstt & 0x7) << 11;
}

uint32_t helper_fnstcw(CPUX86State *env)
{
    return env->fpuc;
}

void update_fp_status(CPUX86State *env)
{
    int rnd_type;

    /* set rounding mode */
    switch (env->fpuc & FPU_RC_MASK) {
    default:
    case FPU_RC_NEAR:
        rnd_type = float_round_nearest_even;
        break;
    case FPU_RC_DOWN:
        rnd_type = float_round_down;
        break;
    case FPU_RC_UP:
        rnd_type = float_round_up;
        break;
    case FPU_RC_CHOP:
        rnd_type = float_round_to_zero;
        break;
    }
    set_float_rounding_mode(rnd_type, &env->fp_status);
    switch ((env->fpuc >> 8) & 3) {
    case 0:
        rnd_type = 32;
        break;
    case 2:
        rnd_type = 64;
        break;
    case 3:
    default:
        rnd_type = 80;
        break;
    }
    set_floatx80_rounding_precision(rnd_type, &env->fp_status);
}

void helper_fldcw(CPUX86State *env, uint32_t val)
{
    cpu_set_fpuc(env, val);
}

void helper_fclex(CPUX86State *env)
{
    env->fpus &= 0x7f00;
}

void helper_fwait(CPUX86State *env)
{
    if (env->fpus & FPUS_SE) {
        fpu_raise_exception(env, GETPC());
    }
}

void helper_fninit(CPUX86State *env)
{
    env->fpus = 0;
    env->fpstt = 0;
    cpu_set_fpuc(env, 0x37f);
    env->fptags[0] = 1;
    env->fptags[1] = 1;
    env->fptags[2] = 1;
    env->fptags[3] = 1;
    env->fptags[4] = 1;
    env->fptags[5] = 1;
    env->fptags[6] = 1;
    env->fptags[7] = 1;
}

/* BCD ops */

void helper_fbld_ST0(CPUX86State *env, target_ulong ptr)
{
    floatx80 tmp;
    uint64_t val;
    unsigned int v;
    int i;

    val = 0;
    for (i = 8; i >= 0; i--) {
        v = cpu_ldub_data_ra(env, ptr + i, GETPC());
        val = (val * 100) + ((v >> 4) * 10) + (v & 0xf);
    }
    tmp = int64_to_floatx80(val, &env->fp_status);
    if (cpu_ldub_data_ra(env, ptr + 9, GETPC()) & 0x80) {
        tmp = floatx80_chs(tmp);
    }
    fpush(env);
    ST0 = tmp;
}

void helper_fbst_ST0(CPUX86State *env, target_ulong ptr)
{
    int v;
    target_ulong mem_ref, mem_end;
    int64_t val;

    val = floatx80_to_int64(ST0, &env->fp_status);
    mem_ref = ptr;
    mem_end = mem_ref + 9;
    if (val < 0) {
        cpu_stb_data_ra(env, mem_end, 0x80, GETPC());
        val = -val;
    } else {
        cpu_stb_data_ra(env, mem_end, 0x00, GETPC());
    }
    while (mem_ref < mem_end) {
        if (val == 0) {
            break;
        }
        v = val % 100;
        val = val / 100;
        v = ((v / 10) << 4) | (v % 10);
        cpu_stb_data_ra(env, mem_ref++, v, GETPC());
    }
    while (mem_ref < mem_end) {
        cpu_stb_data_ra(env, mem_ref++, 0, GETPC());
    }
}

void helper_f2xm1(CPUX86State *env)
{
    double val = floatx80_to_double(env, ST0);

    val = pow(2.0, val) - 1.0;
    ST0 = double_to_floatx80(env, val);
}

void helper_fyl2x(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if (fptemp > 0.0) {
        fptemp = log(fptemp) / log(2.0); /* log2(ST) */
        fptemp *= floatx80_to_double(env, ST1);
        ST1 = double_to_floatx80(env, fptemp);
        fpop(env);
    } else {
        env->fpus &= ~0x4700;
        env->fpus |= 0x400;
    }
}

void helper_fptan(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if ((fptemp > MAXTAN) || (fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        fptemp = tan(fptemp);
        ST0 = double_to_floatx80(env, fptemp);
        fpush(env);
        ST0 = floatx80_one;
        env->fpus &= ~0x400; /* C2 <-- 0 */
        /* the above code is for |arg| < 2**52 only */
    }
}

void helper_fpatan(CPUX86State *env)
{
    double fptemp, fpsrcop;

    fpsrcop = floatx80_to_double(env, ST1);
    fptemp = floatx80_to_double(env, ST0);
    ST1 = double_to_floatx80(env, atan2(fpsrcop, fptemp));
    fpop(env);
}

void helper_fxtract(CPUX86State *env)
{
    CPU_LDoubleU temp;

    temp.d = ST0;

    if (floatx80_is_zero(ST0)) {
        /* Easy way to generate -inf and raising division by 0 exception */
        ST0 = floatx80_div(floatx80_chs(floatx80_one), floatx80_zero,
                           &env->fp_status);
        fpush(env);
        ST0 = temp.d;
    } else {
        int expdif;

        expdif = EXPD(temp) - EXPBIAS;
        /* DP exponent bias */
        ST0 = int32_to_floatx80(expdif, &env->fp_status);
        fpush(env);
        BIASEXPONENT(temp);
        ST0 = temp.d;
    }
}

void helper_fprem1(CPUX86State *env)
{
    double st0, st1, dblq, fpsrcop, fptemp;
    CPU_LDoubleU fpsrcop1, fptemp1;
    int expdif;
    signed long long int q;

    st0 = floatx80_to_double(env, ST0);
    st1 = floatx80_to_double(env, ST1);

    if (isinf(st0) || isnan(st0) || isnan(st1) || (st1 == 0.0)) {
        ST0 = double_to_floatx80(env, 0.0 / 0.0); /* NaN */
        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        return;
    }

    fpsrcop = st0;
    fptemp = st1;
    fpsrcop1.d = ST0;
    fptemp1.d = ST1;
    expdif = EXPD(fpsrcop1) - EXPD(fptemp1);

    if (expdif < 0) {
        /* optimisation? taken from the AMD docs */
        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        /* ST0 is unchanged */
        return;
    }

    if (expdif < 53) {
        dblq = fpsrcop / fptemp;
        /* round dblq towards nearest integer */
        dblq = rint(dblq);
        st0 = fpsrcop - fptemp * dblq;

        /* convert dblq to q by truncating towards zero */
        if (dblq < 0.0) {
            q = (signed long long int)(-dblq);
        } else {
            q = (signed long long int)dblq;
        }

        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        /* (C0,C3,C1) <-- (q2,q1,q0) */
        env->fpus |= (q & 0x4) << (8 - 2);  /* (C0) <-- q2 */
        env->fpus |= (q & 0x2) << (14 - 1); /* (C3) <-- q1 */
        env->fpus |= (q & 0x1) << (9 - 0);  /* (C1) <-- q0 */
    } else {
        env->fpus |= 0x400;  /* C2 <-- 1 */
        fptemp = pow(2.0, expdif - 50);
        fpsrcop = (st0 / st1) / fptemp;
        /* fpsrcop = integer obtained by chopping */
        fpsrcop = (fpsrcop < 0.0) ?
                  -(floor(fabs(fpsrcop))) : floor(fpsrcop);
        st0 -= (st1 * fpsrcop * fptemp);
    }
    ST0 = double_to_floatx80(env, st0);
}

void helper_fprem(CPUX86State *env)
{
    double st0, st1, dblq, fpsrcop, fptemp;
    CPU_LDoubleU fpsrcop1, fptemp1;
    int expdif;
    signed long long int q;

    st0 = floatx80_to_double(env, ST0);
    st1 = floatx80_to_double(env, ST1);

    if (isinf(st0) || isnan(st0) || isnan(st1) || (st1 == 0.0)) {
        ST0 = double_to_floatx80(env, 0.0 / 0.0); /* NaN */
        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        return;
    }

    fpsrcop = st0;
    fptemp = st1;
    fpsrcop1.d = ST0;
    fptemp1.d = ST1;
    expdif = EXPD(fpsrcop1) - EXPD(fptemp1);

    if (expdif < 0) {
        /* optimisation? taken from the AMD docs */
        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        /* ST0 is unchanged */
        return;
    }

    if (expdif < 53) {
        dblq = fpsrcop / fptemp; /* ST0 / ST1 */
        /* round dblq towards zero */
        dblq = (dblq < 0.0) ? ceil(dblq) : floor(dblq);
        st0 = fpsrcop - fptemp * dblq; /* fpsrcop is ST0 */

        /* convert dblq to q by truncating towards zero */
        if (dblq < 0.0) {
            q = (signed long long int)(-dblq);
        } else {
            q = (signed long long int)dblq;
        }

        env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
        /* (C0,C3,C1) <-- (q2,q1,q0) */
        env->fpus |= (q & 0x4) << (8 - 2);  /* (C0) <-- q2 */
        env->fpus |= (q & 0x2) << (14 - 1); /* (C3) <-- q1 */
        env->fpus |= (q & 0x1) << (9 - 0);  /* (C1) <-- q0 */
    } else {
        int N = 32 + (expdif % 32); /* as per AMD docs */

        env->fpus |= 0x400;  /* C2 <-- 1 */
        fptemp = pow(2.0, (double)(expdif - N));
        fpsrcop = (st0 / st1) / fptemp;
        /* fpsrcop = integer obtained by chopping */
        fpsrcop = (fpsrcop < 0.0) ?
                  -(floor(fabs(fpsrcop))) : floor(fpsrcop);
        st0 -= (st1 * fpsrcop * fptemp);
    }
    ST0 = double_to_floatx80(env, st0);
}

void helper_fyl2xp1(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if ((fptemp + 1.0) > 0.0) {
        fptemp = log(fptemp + 1.0) / log(2.0); /* log2(ST + 1.0) */
        fptemp *= floatx80_to_double(env, ST1);
        ST1 = double_to_floatx80(env, fptemp);
        fpop(env);
    } else {
        env->fpus &= ~0x4700;
        env->fpus |= 0x400;
    }
}

void helper_fsqrt(CPUX86State *env)
{
    if (floatx80_is_neg(ST0)) {
        env->fpus &= ~0x4700;  /* (C3,C2,C1,C0) <-- 0000 */
        env->fpus |= 0x400;
    }
    ST0 = floatx80_sqrt(ST0, &env->fp_status);
}

void helper_fsincos(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if ((fptemp > MAXTAN) || (fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = double_to_floatx80(env, sin(fptemp));
        fpush(env);
        ST0 = double_to_floatx80(env, cos(fptemp));
        env->fpus &= ~0x400;  /* C2 <-- 0 */
        /* the above code is for |arg| < 2**63 only */
    }
}

void helper_frndint(CPUX86State *env)
{
    ST0 = floatx80_round_to_int(ST0, &env->fp_status);
}

void helper_fscale(CPUX86State *env)
{
    if (floatx80_is_any_nan(ST1)) {
        ST0 = ST1;
    } else {
        int n = floatx80_to_int32_round_to_zero(ST1, &env->fp_status);
        ST0 = floatx80_scalbn(ST0, n, &env->fp_status);
    }
}

void helper_fsin(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if ((fptemp > MAXTAN) || (fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = double_to_floatx80(env, sin(fptemp));
        env->fpus &= ~0x400;  /* C2 <-- 0 */
        /* the above code is for |arg| < 2**53 only */
    }
}

void helper_fcos(CPUX86State *env)
{
    double fptemp = floatx80_to_double(env, ST0);

    if ((fptemp > MAXTAN) || (fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = double_to_floatx80(env, cos(fptemp));
        env->fpus &= ~0x400;  /* C2 <-- 0 */
        /* the above code is for |arg| < 2**63 only */
    }
}

void helper_fxam_ST0(CPUX86State *env)
{
    CPU_LDoubleU temp;
    int expdif;

    temp.d = ST0;

    env->fpus &= ~0x4700; /* (C3,C2,C1,C0) <-- 0000 */
    if (SIGND(temp)) {
        env->fpus |= 0x200; /* C1 <-- 1 */
    }

    /* XXX: test fptags too */
    expdif = EXPD(temp);
    if (expdif == MAXEXPD) {
        if (MANTD(temp) == 0x8000000000000000ULL) {
            env->fpus |= 0x500; /* Infinity */
        } else {
            env->fpus |= 0x100; /* NaN */
        }
    } else if (expdif == 0) {
        if (MANTD(temp) == 0) {
            env->fpus |=  0x4000; /* Zero */
        } else {
            env->fpus |= 0x4400; /* Denormal */
        }
    } else {
        env->fpus |= 0x400;
    }
}

static void do_fstenv(CPUX86State *env, target_ulong ptr, int data32,
                      uintptr_t retaddr)
{
    int fpus, fptag, exp, i;
    uint64_t mant;
    CPU_LDoubleU tmp;

    fpus = (env->fpus & ~0x3800) | (env->fpstt & 0x7) << 11;
    fptag = 0;
    for (i = 7; i >= 0; i--) {
        fptag <<= 2;
        if (env->fptags[i]) {
            fptag |= 3;
        } else {
            tmp.d = env->fpregs[i].d;
            exp = EXPD(tmp);
            mant = MANTD(tmp);
            if (exp == 0 && mant == 0) {
                /* zero */
                fptag |= 1;
            } else if (exp == 0 || exp == MAXEXPD
                       || (mant & (1LL << 63)) == 0) {
                /* NaNs, infinity, denormal */
                fptag |= 2;
            }
        }
    }
    if (data32) {
        /* 32 bit */
        cpu_stl_data_ra(env, ptr, env->fpuc, retaddr);
        cpu_stl_data_ra(env, ptr + 4, fpus, retaddr);
        cpu_stl_data_ra(env, ptr + 8, fptag, retaddr);
        cpu_stl_data_ra(env, ptr + 12, 0, retaddr); /* fpip */
        cpu_stl_data_ra(env, ptr + 16, 0, retaddr); /* fpcs */
        cpu_stl_data_ra(env, ptr + 20, 0, retaddr); /* fpoo */
        cpu_stl_data_ra(env, ptr + 24, 0, retaddr); /* fpos */
    } else {
        /* 16 bit */
        cpu_stw_data_ra(env, ptr, env->fpuc, retaddr);
        cpu_stw_data_ra(env, ptr + 2, fpus, retaddr);
        cpu_stw_data_ra(env, ptr + 4, fptag, retaddr);
        cpu_stw_data_ra(env, ptr + 6, 0, retaddr);
        cpu_stw_data_ra(env, ptr + 8, 0, retaddr);
        cpu_stw_data_ra(env, ptr + 10, 0, retaddr);
        cpu_stw_data_ra(env, ptr + 12, 0, retaddr);
    }
}

void helper_fstenv(CPUX86State *env, target_ulong ptr, int data32)
{
    do_fstenv(env, ptr, data32, GETPC());
}

static void do_fldenv(CPUX86State *env, target_ulong ptr, int data32,
                      uintptr_t retaddr)
{
    int i, fpus, fptag;

    if (data32) {
        cpu_set_fpuc(env, cpu_lduw_data_ra(env, ptr, retaddr));
        fpus = cpu_lduw_data_ra(env, ptr + 4, retaddr);
        fptag = cpu_lduw_data_ra(env, ptr + 8, retaddr);
    } else {
        cpu_set_fpuc(env, cpu_lduw_data_ra(env, ptr, retaddr));
        fpus = cpu_lduw_data_ra(env, ptr + 2, retaddr);
        fptag = cpu_lduw_data_ra(env, ptr + 4, retaddr);
    }
    env->fpstt = (fpus >> 11) & 7;
    env->fpus = fpus & ~0x3800;
    for (i = 0; i < 8; i++) {
        env->fptags[i] = ((fptag & 3) == 3);
        fptag >>= 2;
    }
}

void helper_fldenv(CPUX86State *env, target_ulong ptr, int data32)
{
    do_fldenv(env, ptr, data32, GETPC());
}

void helper_fsave(CPUX86State *env, target_ulong ptr, int data32)
{
    floatx80 tmp;
    int i;

    do_fstenv(env, ptr, data32, GETPC());

    ptr += (14 << data32);
    for (i = 0; i < 8; i++) {
        tmp = ST(i);
        helper_fstt(env, tmp, ptr, GETPC());
        ptr += 10;
    }

    /* fninit */
    env->fpus = 0;
    env->fpstt = 0;
    cpu_set_fpuc(env, 0x37f);
    env->fptags[0] = 1;
    env->fptags[1] = 1;
    env->fptags[2] = 1;
    env->fptags[3] = 1;
    env->fptags[4] = 1;
    env->fptags[5] = 1;
    env->fptags[6] = 1;
    env->fptags[7] = 1;
}

void helper_frstor(CPUX86State *env, target_ulong ptr, int data32)
{
    floatx80 tmp;
    int i;

    do_fldenv(env, ptr, data32, GETPC());
    ptr += (14 << data32);

    for (i = 0; i < 8; i++) {
        tmp = helper_fldt(env, ptr, GETPC());
        ST(i) = tmp;
        ptr += 10;
    }
}

#if defined(CONFIG_USER_ONLY)
void cpu_x86_fsave(CPUX86State *env, target_ulong ptr, int data32)
{
    helper_fsave(env, ptr, data32);
}

void cpu_x86_frstor(CPUX86State *env, target_ulong ptr, int data32)
{
    helper_frstor(env, ptr, data32);
}
#endif

#define XO(X)  offsetof(X86XSaveArea, X)

static void do_xsave_fpu(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    int fpus, fptag, i;
    target_ulong addr;

    fpus = (env->fpus & ~0x3800) | (env->fpstt & 0x7) << 11;
    fptag = 0;
    for (i = 0; i < 8; i++) {
        fptag |= (env->fptags[i] << i);
    }

    cpu_stw_data_ra(env, ptr + XO(legacy.fcw), env->fpuc, ra);
    cpu_stw_data_ra(env, ptr + XO(legacy.fsw), fpus, ra);
    cpu_stw_data_ra(env, ptr + XO(legacy.ftw), fptag ^ 0xff, ra);

    /* In 32-bit mode this is eip, sel, dp, sel.
       In 64-bit mode this is rip, rdp.
       But in either case we don't write actual data, just zeros.  */
    cpu_stq_data_ra(env, ptr + XO(legacy.fpip), 0, ra); /* eip+sel; rip */
    cpu_stq_data_ra(env, ptr + XO(legacy.fpdp), 0, ra); /* edp+sel; rdp */

    addr = ptr + XO(legacy.fpregs);
    for (i = 0; i < 8; i++) {
        floatx80 tmp = ST(i);
        helper_fstt(env, tmp, addr, ra);
        addr += 16;
    }
}

static void do_xsave_mxcsr(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    cpu_stl_data_ra(env, ptr + XO(legacy.mxcsr), env->mxcsr, ra);
    cpu_stl_data_ra(env, ptr + XO(legacy.mxcsr_mask), 0x0000ffff, ra);
}

static void do_xsave_sse(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    int i, nb_xmm_regs;
    target_ulong addr;

    if (env->hflags & HF_CS64_MASK) {
        nb_xmm_regs = 16;
    } else {
        nb_xmm_regs = 8;
    }

    addr = ptr + XO(legacy.xmm_regs);
    for (i = 0; i < nb_xmm_regs; i++) {
        cpu_stq_data_ra(env, addr, env->xmm_regs[i].ZMM_Q(0), ra);
        cpu_stq_data_ra(env, addr + 8, env->xmm_regs[i].ZMM_Q(1), ra);
        addr += 16;
    }
}

static void do_xsave_bndregs(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    target_ulong addr = ptr + offsetof(XSaveBNDREG, bnd_regs);
    int i;

    for (i = 0; i < 4; i++, addr += 16) {
        cpu_stq_data_ra(env, addr, env->bnd_regs[i].lb, ra);
        cpu_stq_data_ra(env, addr + 8, env->bnd_regs[i].ub, ra);
    }
}

static void do_xsave_bndcsr(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    cpu_stq_data_ra(env, ptr + offsetof(XSaveBNDCSR, bndcsr.cfgu),
                    env->bndcs_regs.cfgu, ra);
    cpu_stq_data_ra(env, ptr + offsetof(XSaveBNDCSR, bndcsr.sts),
                    env->bndcs_regs.sts, ra);
}

static void do_xsave_pkru(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    cpu_stq_data_ra(env, ptr, env->pkru, ra);
}

void helper_fxsave(CPUX86State *env, target_ulong ptr)
{
    uintptr_t ra = GETPC();

    /* The operand must be 16 byte aligned */
    if (ptr & 0xf) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    do_xsave_fpu(env, ptr, ra);

    if (env->cr[4] & CR4_OSFXSR_MASK) {
        do_xsave_mxcsr(env, ptr, ra);
        /* Fast FXSAVE leaves out the XMM registers */
        if (!(env->efer & MSR_EFER_FFXSR)
            || (env->hflags & HF_CPL_MASK)
            || !(env->hflags & HF_LMA_MASK)) {
            do_xsave_sse(env, ptr, ra);
        }
    }
}

static uint64_t get_xinuse(CPUX86State *env)
{
    uint64_t inuse = -1;

    /* For the most part, we don't track XINUSE.  We could calculate it
       here for all components, but it's probably less work to simply
       indicate in use.  That said, the state of BNDREGS is important
       enough to track in HFLAGS, so we might as well use that here.  */
    if ((env->hflags & HF_MPX_IU_MASK) == 0) {
       inuse &= ~XSTATE_BNDREGS_MASK;
    }
    return inuse;
}

static void do_xsave(CPUX86State *env, target_ulong ptr, uint64_t rfbm,
                     uint64_t inuse, uint64_t opt, uintptr_t ra)
{
    uint64_t old_bv, new_bv;

    /* The OS must have enabled XSAVE.  */
    if (!(env->cr[4] & CR4_OSXSAVE_MASK)) {
        raise_exception_ra(env, EXCP06_ILLOP, ra);
    }

    /* The operand must be 64 byte aligned.  */
    if (ptr & 63) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    /* Never save anything not enabled by XCR0.  */
    rfbm &= env->xcr0;
    opt &= rfbm;

    if (opt & XSTATE_FP_MASK) {
        do_xsave_fpu(env, ptr, ra);
    }
    if (rfbm & XSTATE_SSE_MASK) {
        /* Note that saving MXCSR is not suppressed by XSAVEOPT.  */
        do_xsave_mxcsr(env, ptr, ra);
    }
    if (opt & XSTATE_SSE_MASK) {
        do_xsave_sse(env, ptr, ra);
    }
    if (opt & XSTATE_BNDREGS_MASK) {
        do_xsave_bndregs(env, ptr + XO(bndreg_state), ra);
    }
    if (opt & XSTATE_BNDCSR_MASK) {
        do_xsave_bndcsr(env, ptr + XO(bndcsr_state), ra);
    }
    if (opt & XSTATE_PKRU_MASK) {
        do_xsave_pkru(env, ptr + XO(pkru_state), ra);
    }

    /* Update the XSTATE_BV field.  */
    old_bv = cpu_ldq_data_ra(env, ptr + XO(header.xstate_bv), ra);
    new_bv = (old_bv & ~rfbm) | (inuse & rfbm);
    cpu_stq_data_ra(env, ptr + XO(header.xstate_bv), new_bv, ra);
}

void helper_xsave(CPUX86State *env, target_ulong ptr, uint64_t rfbm)
{
    do_xsave(env, ptr, rfbm, get_xinuse(env), -1, GETPC());
}

void helper_xsaveopt(CPUX86State *env, target_ulong ptr, uint64_t rfbm)
{
    uint64_t inuse = get_xinuse(env);
    do_xsave(env, ptr, rfbm, inuse, inuse, GETPC());
}

static void do_xrstor_fpu(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    int i, fpuc, fpus, fptag;
    target_ulong addr;

    fpuc = cpu_lduw_data_ra(env, ptr + XO(legacy.fcw), ra);
    fpus = cpu_lduw_data_ra(env, ptr + XO(legacy.fsw), ra);
    fptag = cpu_lduw_data_ra(env, ptr + XO(legacy.ftw), ra);
    cpu_set_fpuc(env, fpuc);
    env->fpstt = (fpus >> 11) & 7;
    env->fpus = fpus & ~0x3800;
    fptag ^= 0xff;
    for (i = 0; i < 8; i++) {
        env->fptags[i] = ((fptag >> i) & 1);
    }

    addr = ptr + XO(legacy.fpregs);
    for (i = 0; i < 8; i++) {
        floatx80 tmp = helper_fldt(env, addr, ra);
        ST(i) = tmp;
        addr += 16;
    }
}

static void do_xrstor_mxcsr(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    cpu_set_mxcsr(env, cpu_ldl_data_ra(env, ptr + XO(legacy.mxcsr), ra));
}

static void do_xrstor_sse(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    int i, nb_xmm_regs;
    target_ulong addr;

    if (env->hflags & HF_CS64_MASK) {
        nb_xmm_regs = 16;
    } else {
        nb_xmm_regs = 8;
    }

    addr = ptr + XO(legacy.xmm_regs);
    for (i = 0; i < nb_xmm_regs; i++) {
        env->xmm_regs[i].ZMM_Q(0) = cpu_ldq_data_ra(env, addr, ra);
        env->xmm_regs[i].ZMM_Q(1) = cpu_ldq_data_ra(env, addr + 8, ra);
        addr += 16;
    }
}

static void do_xrstor_bndregs(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    target_ulong addr = ptr + offsetof(XSaveBNDREG, bnd_regs);
    int i;

    for (i = 0; i < 4; i++, addr += 16) {
        env->bnd_regs[i].lb = cpu_ldq_data_ra(env, addr, ra);
        env->bnd_regs[i].ub = cpu_ldq_data_ra(env, addr + 8, ra);
    }
}

static void do_xrstor_bndcsr(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    /* FIXME: Extend highest implemented bit of linear address.  */
    env->bndcs_regs.cfgu
        = cpu_ldq_data_ra(env, ptr + offsetof(XSaveBNDCSR, bndcsr.cfgu), ra);
    env->bndcs_regs.sts
        = cpu_ldq_data_ra(env, ptr + offsetof(XSaveBNDCSR, bndcsr.sts), ra);
}

static void do_xrstor_pkru(CPUX86State *env, target_ulong ptr, uintptr_t ra)
{
    env->pkru = cpu_ldq_data_ra(env, ptr, ra);
}

void helper_fxrstor(CPUX86State *env, target_ulong ptr)
{
    uintptr_t ra = GETPC();

    /* The operand must be 16 byte aligned */
    if (ptr & 0xf) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    do_xrstor_fpu(env, ptr, ra);

    if (env->cr[4] & CR4_OSFXSR_MASK) {
        do_xrstor_mxcsr(env, ptr, ra);
        /* Fast FXRSTOR leaves out the XMM registers */
        if (!(env->efer & MSR_EFER_FFXSR)
            || (env->hflags & HF_CPL_MASK)
            || !(env->hflags & HF_LMA_MASK)) {
            do_xrstor_sse(env, ptr, ra);
        }
    }
}

#if defined(CONFIG_USER_ONLY)
void cpu_x86_fxsave(CPUX86State *env, target_ulong ptr)
{
    helper_fxsave(env, ptr);
}

void cpu_x86_fxrstor(CPUX86State *env, target_ulong ptr)
{
    helper_fxrstor(env, ptr);
}
#endif

void helper_xrstor(CPUX86State *env, target_ulong ptr, uint64_t rfbm)
{
    uintptr_t ra = GETPC();
    uint64_t xstate_bv, xcomp_bv, reserve0;

    rfbm &= env->xcr0;

    /* The OS must have enabled XSAVE.  */
    if (!(env->cr[4] & CR4_OSXSAVE_MASK)) {
        raise_exception_ra(env, EXCP06_ILLOP, ra);
    }

    /* The operand must be 64 byte aligned.  */
    if (ptr & 63) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    xstate_bv = cpu_ldq_data_ra(env, ptr + XO(header.xstate_bv), ra);

    if ((int64_t)xstate_bv < 0) {
        /* FIXME: Compact form.  */
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    /* Standard form.  */

    /* The XSTATE_BV field must not set bits not present in XCR0.  */
    if (xstate_bv & ~env->xcr0) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    /* The XCOMP_BV field must be zero.  Note that, as of the April 2016
       revision, the description of the XSAVE Header (Vol 1, Sec 13.4.2)
       describes only XCOMP_BV, but the description of the standard form
       of XRSTOR (Vol 1, Sec 13.8.1) checks bytes 23:8 for zero, which
       includes the next 64-bit field.  */
    xcomp_bv = cpu_ldq_data_ra(env, ptr + XO(header.xcomp_bv), ra);
    reserve0 = cpu_ldq_data_ra(env, ptr + XO(header.reserve0), ra);
    if (xcomp_bv || reserve0) {
        raise_exception_ra(env, EXCP0D_GPF, ra);
    }

    if (rfbm & XSTATE_FP_MASK) {
        if (xstate_bv & XSTATE_FP_MASK) {
            do_xrstor_fpu(env, ptr, ra);
        } else {
            helper_fninit(env);
            memset(env->fpregs, 0, sizeof(env->fpregs));
        }
    }
    if (rfbm & XSTATE_SSE_MASK) {
        /* Note that the standard form of XRSTOR loads MXCSR from memory
           whether or not the XSTATE_BV bit is set.  */
        do_xrstor_mxcsr(env, ptr, ra);
        if (xstate_bv & XSTATE_SSE_MASK) {
            do_xrstor_sse(env, ptr, ra);
        } else {
            /* ??? When AVX is implemented, we may have to be more
               selective in the clearing.  */
            memset(env->xmm_regs, 0, sizeof(env->xmm_regs));
        }
    }
    if (rfbm & XSTATE_BNDREGS_MASK) {
        if (xstate_bv & XSTATE_BNDREGS_MASK) {
            do_xrstor_bndregs(env, ptr + XO(bndreg_state), ra);
            env->hflags |= HF_MPX_IU_MASK;
        } else {
            memset(env->bnd_regs, 0, sizeof(env->bnd_regs));
            env->hflags &= ~HF_MPX_IU_MASK;
        }
    }
    if (rfbm & XSTATE_BNDCSR_MASK) {
        if (xstate_bv & XSTATE_BNDCSR_MASK) {
            do_xrstor_bndcsr(env, ptr + XO(bndcsr_state), ra);
        } else {
            memset(&env->bndcs_regs, 0, sizeof(env->bndcs_regs));
        }
        cpu_sync_bndcs_hflags(env);
    }
    if (rfbm & XSTATE_PKRU_MASK) {
        uint64_t old_pkru = env->pkru;
        if (xstate_bv & XSTATE_PKRU_MASK) {
            do_xrstor_pkru(env, ptr + XO(pkru_state), ra);
        } else {
            env->pkru = 0;
        }
        if (env->pkru != old_pkru) {
            CPUState *cs = env_cpu(env);
            tlb_flush(cs);
        }
    }
}

#undef XO

uint64_t helper_xgetbv(CPUX86State *env, uint32_t ecx)
{
    /* The OS must have enabled XSAVE.  */
    if (!(env->cr[4] & CR4_OSXSAVE_MASK)) {
        raise_exception_ra(env, EXCP06_ILLOP, GETPC());
    }

    switch (ecx) {
    case 0:
        return env->xcr0;
    case 1:
        if (env->features[FEAT_XSAVE] & CPUID_XSAVE_XGETBV1) {
            return env->xcr0 & get_xinuse(env);
        }
        break;
    }
    raise_exception_ra(env, EXCP0D_GPF, GETPC());
}

void helper_xsetbv(CPUX86State *env, uint32_t ecx, uint64_t mask)
{
    uint32_t dummy, ena_lo, ena_hi;
    uint64_t ena;

    /* The OS must have enabled XSAVE.  */
    if (!(env->cr[4] & CR4_OSXSAVE_MASK)) {
        raise_exception_ra(env, EXCP06_ILLOP, GETPC());
    }

    /* Only XCR0 is defined at present; the FPU may not be disabled.  */
    if (ecx != 0 || (mask & XSTATE_FP_MASK) == 0) {
        goto do_gpf;
    }

    /* Disallow enabling unimplemented features.  */
    cpu_x86_cpuid(env, 0x0d, 0, &ena_lo, &dummy, &dummy, &ena_hi);
    ena = ((uint64_t)ena_hi << 32) | ena_lo;
    if (mask & ~ena) {
        goto do_gpf;
    }

    /* Disallow enabling only half of MPX.  */
    if ((mask ^ (mask * (XSTATE_BNDCSR_MASK / XSTATE_BNDREGS_MASK)))
        & XSTATE_BNDCSR_MASK) {
        goto do_gpf;
    }

    env->xcr0 = mask;
    cpu_sync_bndcs_hflags(env);
    return;

 do_gpf:
    raise_exception_ra(env, EXCP0D_GPF, GETPC());
}

/* MMX/SSE */
/* XXX: optimize by storing fptt and fptags in the static cpu state */

#define SSE_DAZ             0x0040
#define SSE_RC_MASK         0x6000
#define SSE_RC_NEAR         0x0000
#define SSE_RC_DOWN         0x2000
#define SSE_RC_UP           0x4000
#define SSE_RC_CHOP         0x6000
#define SSE_FZ              0x8000

void update_mxcsr_status(CPUX86State *env)
{
    uint32_t mxcsr = env->mxcsr;
    int rnd_type;

    /* set rounding mode */
    switch (mxcsr & SSE_RC_MASK) {
    default:
    case SSE_RC_NEAR:
        rnd_type = float_round_nearest_even;
        break;
    case SSE_RC_DOWN:
        rnd_type = float_round_down;
        break;
    case SSE_RC_UP:
        rnd_type = float_round_up;
        break;
    case SSE_RC_CHOP:
        rnd_type = float_round_to_zero;
        break;
    }
    set_float_rounding_mode(rnd_type, &env->sse_status);

    /* set denormals are zero */
    set_flush_inputs_to_zero((mxcsr & SSE_DAZ) ? 1 : 0, &env->sse_status);

    /* set flush to zero */
    set_flush_to_zero((mxcsr & SSE_FZ) ? 1 : 0, &env->fp_status);
}

void helper_ldmxcsr(CPUX86State *env, uint32_t val)
{
    cpu_set_mxcsr(env, val);
}

void helper_enter_mmx(CPUX86State *env)
{
    env->fpstt = 0;
    *(uint32_t *)(env->fptags) = 0;
    *(uint32_t *)(env->fptags + 4) = 0;
}

void helper_emms(CPUX86State *env)
{
    /* set to empty state */
    *(uint32_t *)(env->fptags) = 0x01010101;
    *(uint32_t *)(env->fptags + 4) = 0x01010101;
}

/* XXX: suppress */
void helper_movq(CPUX86State *env, void *d, void *s)
{
    *(uint64_t *)d = *(uint64_t *)s;
}

#endif

#define SHIFT 0
#include "ops_sse.h"

#define SHIFT 1
#include "ops_sse.h"
