DEF_HELPER_FLAGS_1(sym_dbg, 0, void, env)

DEF_HELPER_FLAGS_2(sym_init_args_2_void, 0, void, ptr, ptr)
DEF_HELPER_FLAGS_3(sym_init_args_3_void, 0, void, ptr, ptr, ptr)

DEF_HELPER_FLAGS_2(sym_init_args_2, TCG_CALL_NO_RWG_SE, ptr, ptr, ptr)
DEF_HELPER_FLAGS_4(sym_init_args_4, TCG_CALL_NO_RWG_SE, ptr, ptr, ptr, ptr, ptr)
DEF_HELPER_FLAGS_1(sym_set_return_value, TCG_CALL_NO_RWG_SE, ptr, ptr)

DEF_HELPER_FLAGS_2(sym_load_mem_reg, 0, ptr, env, i64)
DEF_HELPER_FLAGS_3(sym_store_mem_reg, TCG_CALL_NO_RWG, void, env, ptr, i64)

// cc_helper.c
DEF_HELPER_FLAGS_4(cc_compute_c_symbolized, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl, int)
DEF_HELPER_FLAGS_4(cc_compute_all_symbolized, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl, int)

// int_helper.c
DEF_HELPER_2(divb_AL_symbolized, void, env, tl)
DEF_HELPER_2(divw_AX_symbolized, void, env, tl)
DEF_HELPER_2(divl_EAX_symbolized, void, env, tl)
DEF_HELPER_2(divq_EAX_symbolized, void, env, tl)
DEF_HELPER_2(idivb_AL_symbolized, void, env, tl)
DEF_HELPER_2(idivw_AX_symbolized, void, env, tl)
DEF_HELPER_2(idivl_EAX_symbolized, void, env, tl)
DEF_HELPER_2(idivq_EAX_symbolized, void, env, tl)

// fpu_helper.c and op_sse.h
#define SUFFIX _xmm_symbolized
#define Reg ZMMReg

DEF_HELPER_3(glue(psrlw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psraw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psllw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psrld, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psrad, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pslld, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psrlq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psllq, SUFFIX), void, env, Reg, Reg)

DEF_HELPER_3(glue(psrldq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pslldq, SUFFIX), void, env, Reg, Reg)

#define SSE_HELPER_B(name, F)\
    DEF_HELPER_3(glue(name, SUFFIX), void, env, Reg, Reg)

#define SSE_HELPER_W(name, F)\
    DEF_HELPER_3(glue(name, SUFFIX), void, env, Reg, Reg)

#define SSE_HELPER_L(name, F)\
    DEF_HELPER_3(glue(name, SUFFIX), void, env, Reg, Reg)

#define SSE_HELPER_Q(name, F)\
    DEF_HELPER_3(glue(name, SUFFIX), void, env, Reg, Reg)

// DEF_HELPER_3(glue(paddq, SUFFIX), void, env, Reg, Reg) 

SSE_HELPER_B(paddb, FADD)
SSE_HELPER_W(paddw, FADD)
SSE_HELPER_L(paddl, FADD)
SSE_HELPER_Q(paddq, FADD)

SSE_HELPER_B(psubb, FSUB)
SSE_HELPER_W(psubw, FSUB)
SSE_HELPER_L(psubl, FSUB)
SSE_HELPER_Q(psubq, FSUB)

SSE_HELPER_B(paddusb, FADDUB)
SSE_HELPER_B(paddsb, FADDSB)
SSE_HELPER_B(psubusb, FSUBUB)
SSE_HELPER_B(psubsb, FSUBSB)

SSE_HELPER_W(paddusw, FADDUW)
SSE_HELPER_W(paddsw, FADDSW)
SSE_HELPER_W(psubusw, FSUBUW)
SSE_HELPER_W(psubsw, FSUBSW)

SSE_HELPER_B(pminub, FMINUB)
SSE_HELPER_B(pmaxub, FMAXUB)

SSE_HELPER_W(pminsw, FMINSW)
SSE_HELPER_W(pmaxsw, FMAXSW)

SSE_HELPER_Q(pand, FAND)
SSE_HELPER_Q(pandn, FANDN)
SSE_HELPER_Q(por, FOR)
SSE_HELPER_Q(pxor, FXOR)

SSE_HELPER_B(pcmpgtb, FCMPGTB)
SSE_HELPER_W(pcmpgtw, FCMPGTW)
SSE_HELPER_L(pcmpgtl, FCMPGTL)

SSE_HELPER_B(pcmpeqb, FCMPEQ)
SSE_HELPER_W(pcmpeqw, FCMPEQ)
SSE_HELPER_L(pcmpeql, FCMPEQ)

SSE_HELPER_W(pmullw, FMULLW)
// SSE_HELPER_W(pmulhrw, FMULHRW)
SSE_HELPER_W(pmulhuw, FMULHUW)
SSE_HELPER_W(pmulhw, FMULHW)

SSE_HELPER_B(pavgb, FAVG)
SSE_HELPER_W(pavgw, FAVG)

DEF_HELPER_3(glue(pmuludq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaddwd, SUFFIX), void, env, Reg, Reg)

DEF_HELPER_3(glue(psadbw, SUFFIX), void, env, Reg, Reg)
// DEF_HELPER_4(glue(maskmov, SUFFIX), void, env, Reg, Reg, tl)
DEF_HELPER_2(glue(movl_mm_T0, SUFFIX), void, Reg, i32)
DEF_HELPER_2(glue(movq_mm_T0, SUFFIX), void, Reg, i64)

// DEF_HELPER_3(glue(pshufw, SUFFIX), void, Reg, Reg, int)
DEF_HELPER_3(glue(pshufd, SUFFIX), void, Reg, Reg, int)
DEF_HELPER_3(glue(pshuflw, SUFFIX), void, Reg, Reg, int)
DEF_HELPER_3(glue(pshufhw, SUFFIX), void, Reg, Reg, int)

DEF_HELPER_2(glue(pmovmskb, SUFFIX), i32, env, Reg)
DEF_HELPER_3(glue(packsswb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(packuswb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(packssdw, SUFFIX), void, env, Reg, Reg)
#define UNPCK_OP(base_name, base)                                       \
    DEF_HELPER_3(glue(punpck ## base_name ## bw, SUFFIX), void, env, Reg, Reg) \
    DEF_HELPER_3(glue(punpck ## base_name ## wd, SUFFIX), void, env, Reg, Reg) \
    DEF_HELPER_3(glue(punpck ## base_name ## dq, SUFFIX), void, env, Reg, Reg)

UNPCK_OP(l, 0)
UNPCK_OP(h, 1)

DEF_HELPER_3(glue(punpcklqdq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(punpckhqdq, SUFFIX), void, env, Reg, Reg)

DEF_HELPER_3(glue(phaddw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phaddd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phaddsw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phsubw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phsubd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phsubsw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pabsb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pabsw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pabsd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaddubsw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmulhrsw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pshufb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psignb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psignw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(psignd, SUFFIX), void, env, Reg, Reg)
// DEF_HELPER_4(glue(palignr, SUFFIX), void, env, Reg, Reg, s32)

DEF_HELPER_3(glue(pblendvb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(blendvps, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(blendvpd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(ptest, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxbw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxbd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxbq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxwd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxwq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovsxdq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxbw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxbd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxbq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxwd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxwq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmovzxdq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmuldq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pcmpeqq, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(packusdw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pminsb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pminsd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pminuw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pminud, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaxsb, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaxsd, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaxuw, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmaxud, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(pmulld, SUFFIX), void, env, Reg, Reg)
DEF_HELPER_3(glue(phminposuw, SUFFIX), void, env, Reg, Reg)

// DEF_HELPER_4(glue(mpsadbw, SUFFIX), void, env, Reg, Reg, i32)

#undef UNPCK_OP
#undef SSE_HELPER_B
#undef SSE_HELPER_W
#undef SSE_HELPER_L
#undef SSE_HELPER_Q
#undef SUFFIX
#undef Reg