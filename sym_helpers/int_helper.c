/*
 *  x86 integer helpers
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
#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"
#include "exec/helper-proto.h"
#include "qapi/error.h"
#include "qemu/guest-random.h"

/* HYBRID */
#include "sym_helpers.h"

#include "qemu/log.h"
#include "qemu/log-for-trace.h"

// rename helpers to avoid conflicts with QEMU
// NOTE: other functions must be static!

#define cpu_cc_compute_all SYM(cpu_cc_compute_all)
uint32_t SYM(cpu_cc_compute_all)(CPUX86State *env, int op);

#define helper_divb_AL SYM(helper_divb_AL) 
#define helper_idivb_AL SYM(helper_idivb_AL) 
#define helper_divw_AX SYM(helper_divw_AX) 
#define helper_idivw_AX SYM(helper_idivw_AX) 
#define helper_divl_EAX SYM(helper_divl_EAX) 
#define helper_idivl_EAX SYM(helper_idivl_EAX) 
#define helper_divq_EAX SYM(helper_divq_EAX) 
#define helper_idivq_EAX SYM(helper_idivq_EAX) 

#define helper_aam SYM(helper_aam) 
#define helper_aad SYM(helper_aad) 
#define helper_aaa SYM(helper_aaa) 
#define helper_aas SYM(helper_aas) 
#define helper_daa SYM(helper_daa) 
#define helper_das SYM(helper_das) 

#define helper_pdep SYM(helper_pdep) 
#define helper_pext SYM(helper_pext) 

#define helper_rclb SYM(helper_rclb) 
#define helper_rcrb SYM(helper_rcrb) 
#define helper_rclw SYM(helper_rclw) 
#define helper_rcrw SYM(helper_rcrw) 
#define helper_rcll SYM(helper_rcll) 
#define helper_rcrl SYM(helper_rcrl) 
#define helper_rclq SYM(helper_rclq) 
#define helper_rcrq SYM(helper_rcrq) 

#define helper_rdrand SYM(helper_rdrand) 

// FIXME
// #define qemu_log_mask(a, b, c)
// #define error_free(e)

extern void QEMU_NORETURN raise_exception_ra(CPUX86State *env, int exception_index,
                                      uintptr_t retaddr);

/* HYBRID */

//#define DEBUG_MULDIV

/* modulo 9 table */
static const uint8_t rclb_table[32] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 0, 1, 2, 3, 4,
};

/* modulo 17 table */
static const uint8_t rclw_table[32] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14,
};

/* division, flags are undefined */

void helper_divb_AL(CPUX86State *env, target_ulong t0)
{
    unsigned int num, den, q, r;

    num = (env->regs[R_EAX] & 0xffff);
    den = (t0 & 0xff);
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    if (q > 0xff) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q &= 0xff;
    r = (num % den) & 0xff;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | (r << 8) | q;
}

void helper_idivb_AL(CPUX86State *env, target_ulong t0)
{
    int num, den, q, r;

    num = (int16_t)env->regs[R_EAX];
    den = (int8_t)t0;
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    if (q != (int8_t)q) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q &= 0xff;
    r = (num % den) & 0xff;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | (r << 8) | q;
}

void helper_divw_AX(CPUX86State *env, target_ulong t0)
{
    unsigned int num, den, q, r;

    num = (env->regs[R_EAX] & 0xffff) | ((env->regs[R_EDX] & 0xffff) << 16);
    den = (t0 & 0xffff);
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    if (q > 0xffff) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q &= 0xffff;
    r = (num % den) & 0xffff;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | q;
    env->regs[R_EDX] = (env->regs[R_EDX] & ~0xffff) | r;
}

void helper_idivw_AX(CPUX86State *env, target_ulong t0)
{
    int num, den, q, r;

    num = (env->regs[R_EAX] & 0xffff) | ((env->regs[R_EDX] & 0xffff) << 16);
    den = (int16_t)t0;
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    if (q != (int16_t)q) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q &= 0xffff;
    r = (num % den) & 0xffff;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | q;
    env->regs[R_EDX] = (env->regs[R_EDX] & ~0xffff) | r;
}

void helper_divl_EAX(CPUX86State *env, target_ulong t0)
{
    unsigned int den, r;
    uint64_t num, q;

    num = ((uint32_t)env->regs[R_EAX]) | ((uint64_t)((uint32_t)env->regs[R_EDX]) << 32);
    den = t0;
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    r = (num % den);
    if (q > 0xffffffff) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    env->regs[R_EAX] = (uint32_t)q;
    env->regs[R_EDX] = (uint32_t)r;
}

void helper_idivl_EAX(CPUX86State *env, target_ulong t0)
{
    int den, r;
    int64_t num, q;

    num = ((uint32_t)env->regs[R_EAX]) | ((uint64_t)((uint32_t)env->regs[R_EDX]) << 32);
    den = t0;
    if (den == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    q = (num / den);
    r = (num % den);
    if (q != (int32_t)q) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    env->regs[R_EAX] = (uint32_t)q;
    env->regs[R_EDX] = (uint32_t)r;
}

/* bcd */

/* XXX: exception */
void helper_aam(CPUX86State *env, int base)
{
    int al, ah;

    al = env->regs[R_EAX] & 0xff;
    ah = al / base;
    al = al % base;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | al | (ah << 8);
    CC_DST = al;
}

void helper_aad(CPUX86State *env, int base)
{
    int al, ah;

    al = env->regs[R_EAX] & 0xff;
    ah = (env->regs[R_EAX] >> 8) & 0xff;
    al = ((ah * base) + al) & 0xff;
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | al;
    CC_DST = al;
}

void helper_aaa(CPUX86State *env)
{
    int icarry;
    int al, ah, af;
    int eflags;

    eflags = cpu_cc_compute_all(env, CC_OP);
    af = eflags & CC_A;
    al = env->regs[R_EAX] & 0xff;
    ah = (env->regs[R_EAX] >> 8) & 0xff;

    icarry = (al > 0xf9);
    if (((al & 0x0f) > 9) || af) {
        al = (al + 6) & 0x0f;
        ah = (ah + 1 + icarry) & 0xff;
        eflags |= CC_C | CC_A;
    } else {
        eflags &= ~(CC_C | CC_A);
        al &= 0x0f;
    }
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | al | (ah << 8);
    CC_SRC = eflags;
}

void helper_aas(CPUX86State *env)
{
    int icarry;
    int al, ah, af;
    int eflags;

    eflags = cpu_cc_compute_all(env, CC_OP);
    af = eflags & CC_A;
    al = env->regs[R_EAX] & 0xff;
    ah = (env->regs[R_EAX] >> 8) & 0xff;

    icarry = (al < 6);
    if (((al & 0x0f) > 9) || af) {
        al = (al - 6) & 0x0f;
        ah = (ah - 1 - icarry) & 0xff;
        eflags |= CC_C | CC_A;
    } else {
        eflags &= ~(CC_C | CC_A);
        al &= 0x0f;
    }
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xffff) | al | (ah << 8);
    CC_SRC = eflags;
}

void helper_daa(CPUX86State *env)
{
    int old_al, al, af, cf;
    int eflags;

    eflags = cpu_cc_compute_all(env, CC_OP);
    cf = eflags & CC_C;
    af = eflags & CC_A;
    old_al = al = env->regs[R_EAX] & 0xff;

    eflags = 0;
    if (((al & 0x0f) > 9) || af) {
        al = (al + 6) & 0xff;
        eflags |= CC_A;
    }
    if ((old_al > 0x99) || cf) {
        al = (al + 0x60) & 0xff;
        eflags |= CC_C;
    }
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xff) | al;
    /* well, speed is not an issue here, so we compute the flags by hand */
    eflags |= (al == 0) << 6; /* zf */
    eflags |= parity_table[al]; /* pf */
    eflags |= (al & 0x80); /* sf */
    CC_SRC = eflags;
}

void helper_das(CPUX86State *env)
{
    int al, al1, af, cf;
    int eflags;

    eflags = cpu_cc_compute_all(env, CC_OP);
    cf = eflags & CC_C;
    af = eflags & CC_A;
    al = env->regs[R_EAX] & 0xff;

    eflags = 0;
    al1 = al;
    if (((al & 0x0f) > 9) || af) {
        eflags |= CC_A;
        if (al < 6 || cf) {
            eflags |= CC_C;
        }
        al = (al - 6) & 0xff;
    }
    if ((al1 > 0x99) || cf) {
        al = (al - 0x60) & 0xff;
        eflags |= CC_C;
    }
    env->regs[R_EAX] = (env->regs[R_EAX] & ~0xff) | al;
    /* well, speed is not an issue here, so we compute the flags by hand */
    eflags |= (al == 0) << 6; /* zf */
    eflags |= parity_table[al]; /* pf */
    eflags |= (al & 0x80); /* sf */
    CC_SRC = eflags;
}

#ifdef TARGET_X86_64
static void add128(uint64_t *plow, uint64_t *phigh, uint64_t a, uint64_t b)
{
    *plow += a;
    /* carry test */
    if (*plow < a) {
        (*phigh)++;
    }
    *phigh += b;
}

static void neg128(uint64_t *plow, uint64_t *phigh)
{
    *plow = ~*plow;
    *phigh = ~*phigh;
    add128(plow, phigh, 1, 0);
}

/* return TRUE if overflow */
static int div64(uint64_t *plow, uint64_t *phigh, uint64_t b)
{
    uint64_t q, r, a1, a0;
    int i, qb, ab;

    a0 = *plow;
    a1 = *phigh;
    if (a1 == 0) {
        q = a0 / b;
        r = a0 % b;
        *plow = q;
        *phigh = r;
    } else {
        if (a1 >= b) {
            return 1;
        }
        /* XXX: use a better algorithm */
        for (i = 0; i < 64; i++) {
            ab = a1 >> 63;
            a1 = (a1 << 1) | (a0 >> 63);
            if (ab || a1 >= b) {
                a1 -= b;
                qb = 1;
            } else {
                qb = 0;
            }
            a0 = (a0 << 1) | qb;
        }
#if defined(DEBUG_MULDIV)
        printf("div: 0x%016" PRIx64 "%016" PRIx64 " / 0x%016" PRIx64
               ": q=0x%016" PRIx64 " r=0x%016" PRIx64 "\n",
               *phigh, *plow, b, a0, a1);
#endif
        *plow = a0;
        *phigh = a1;
    }
    return 0;
}

/* return TRUE if overflow */
static int idiv64(uint64_t *plow, uint64_t *phigh, int64_t b)
{
    int sa, sb;

    sa = ((int64_t)*phigh < 0);
    if (sa) {
        neg128(plow, phigh);
    }
    sb = (b < 0);
    if (sb) {
        b = -b;
    }
    if (div64(plow, phigh, b) != 0) {
        return 1;
    }
    if (sa ^ sb) {
        if (*plow > (1ULL << 63)) {
            return 1;
        }
        *plow = -*plow;
    } else {
        if (*plow >= (1ULL << 63)) {
            return 1;
        }
    }
    if (sa) {
        *phigh = -*phigh;
    }
    return 0;
}

void helper_divq_EAX(CPUX86State *env, target_ulong t0)
{
    uint64_t r0, r1;

    if (t0 == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    r0 = env->regs[R_EAX];
    r1 = env->regs[R_EDX];
    if (div64(&r0, &r1, t0)) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    env->regs[R_EAX] = r0;
    env->regs[R_EDX] = r1;
}

void helper_idivq_EAX(CPUX86State *env, target_ulong t0)
{
    uint64_t r0, r1;

    if (t0 == 0) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    r0 = env->regs[R_EAX];
    r1 = env->regs[R_EDX];
    if (idiv64(&r0, &r1, t0)) {
        raise_exception_ra(env, EXCP00_DIVZ, GETPC());
    }
    env->regs[R_EAX] = r0;
    env->regs[R_EDX] = r1;
}
#endif

#if TARGET_LONG_BITS == 32
# define ctztl  ctz32
# define clztl  clz32
#else
# define ctztl  ctz64
# define clztl  clz64
#endif

target_ulong helper_pdep(target_ulong src, target_ulong mask)
{
    target_ulong dest = 0;
    int i, o;

    for (i = 0; mask != 0; i++) {
        o = ctztl(mask);
        mask &= mask - 1;
        dest |= ((src >> i) & 1) << o;
    }
    return dest;
}

target_ulong helper_pext(target_ulong src, target_ulong mask)
{
    target_ulong dest = 0;
    int i, o;

    for (o = 0; mask != 0; o++) {
        i = ctztl(mask);
        mask &= mask - 1;
        dest |= ((src >> i) & 1) << o;
    }
    return dest;
}

#define SHIFT 0
#include "shift_helper_template.h"
#undef SHIFT

#define SHIFT 1
#include "shift_helper_template.h"
#undef SHIFT

#define SHIFT 2
#include "shift_helper_template.h"
#undef SHIFT

#ifdef TARGET_X86_64
#define SHIFT 3
#include "shift_helper_template.h"
#undef SHIFT
#endif

/* Test that BIT is enabled in CR4.  If not, raise an illegal opcode
   exception.  This reduces the requirements for rare CR4 bits being
   mapped into HFLAGS.  */
void helper_cr4_testbit(CPUX86State *env, uint32_t bit)
{
    if (unlikely((env->cr[4] & bit) == 0)) {
        raise_exception_ra(env, EXCP06_ILLOP, GETPC());
    }
}

target_ulong HELPER(rdrand)(CPUX86State *env)
{
    Error *err = NULL;
    target_ulong ret;

    if (qemu_guest_getrandom(&ret, sizeof(ret), &err) < 0) {
#if 0
        qemu_log_mask(LOG_UNIMP, "rdrand: Crypto failure: %s",
                      error_get_pretty(err));
        error_free(err);
#endif
        /* Failure clears CF and all other flags, and returns 0.  */
        env->cc_src = 0;
        return 0;
    }

    /* Success sets CF and clears all others.  */
    env->cc_src = CC_C;
    return ret;
}
