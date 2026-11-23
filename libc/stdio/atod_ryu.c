// Copyright 2019 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

#include "local-stdio.h"
#include "ryu/generic_128.h"

/*
 * Convert a decimal value n * 10^e10 to binary, as Ryu's s2d does, but
 * for any n of up to *_RYU_DIG digits, for every type, and leaving the
 * rounding to the caller.
 *
 * Like s2d, this picks a binary exponent e2 from the lengths of n and
 * 5^e10 so that m2 = floor(n * 10^e10 / 2^e2) has mant_dig + 2 or
 * mant_dig + 3 bits, then computes m2 as the top bits of the product
 * of n and an approximation of 5^e10 (or 5^-e10): the 125-bit values
 * from ryu_table.c for float and double, the 251-bit values from
 * ryu_table_128.c for long double, or, for double-double, those from
 * ryu_table_atod.c.
 *
 * The approximation is low (5^e10) or high (5^-e10) by less than
 * pow5_err units, so the product is off by less than n * pow5_err
 * units. Where a multiple of 2^shift lies within that distance, the
 * value is exact; s2d instead checks whether n is a multiple of a
 * power of two or five, which needs more work for long n.
 * mk-ryu-table-128.py checks, for every decimal exponent and every
 * length of n up to *_RYU_DIG digits, that no inexact value comes that
 * close to a multiple of 2^shift, so this decides exactness correctly
 * without a fallback. The caller then rounds m2 with the sticky bit
 * in any direction.
 *
 * Values beyond the type's range, in every rounding direction, return
 * a small m2 with *sticky set and an exponent out of range for the
 * type.
 */

#ifdef ATOD_RYU_LONG_DOUBLE
// 80-bit and binary128 long double
#define ATOD_RYU_FUNC __atold_ryu
#define POW5_MAX_BITS FLOAT_128_POW5_BITCOUNT
#elif __LDBL_MANT_DIG__ == 106
// float, double and double-double
#define ATOD_RYU_FUNC __atod_ryu
#define POW5_MAX_BITS ATOD_RYU_BITCOUNT
#else
// float and double
#define ATOD_RYU_FUNC __atod_ryu
#define POW5_MAX_BITS DOUBLE_POW5_BITCOUNT
#endif

// Words of n, the power of five and their product
#define N_LIMBS    RYU_LIMBS(32 * ATOD_RYU_NWORDS)
#define POW5_LIMBS RYU_LIMBS(POW5_MAX_BITS)
#define P_LIMBS    (N_LIMBS + POW5_LIMBS)
#define Q_LIMBS    RYU_LIMBS(128)

// Converts n 64-bit words to ryu_limb
static void
pow5_limbs(const uint64_t *t, int n, ryu_limb *r)
{
    int j;

    for (j = 0; j < RYU_LIMBS(64 * n); j++)
        r[j] = (ryu_limb)(t[j * RYU_LIMB_BITS / 64] >> (j * RYU_LIMB_BITS % 64));
}

int
ATOD_RYU_FUNC(const uint32_t *n, int nn, int e10, int mant_dig, int min_exp, int max_exp,
              uint64_t *q, int *sticky)
{
    ryu_limb  m[N_LIMBS + 1], pow5[POW5_LIMBS], p[P_LIMBS], r[P_LIMBS];
    ryu_limb  qp[Q_LIMBS], qr[Q_LIMBS], low;
    ryu_dlimb carry;
    int       nm = RYU_LIMBS(32 * nn);
    int       np, pow5_bits, pow5_err;
    int       bits, e2, shift, i, exact;

    for (i = 0; i < nm; i++) {
        m[i] = n[i * RYU_LIMB_BITS / 32];
        if (RYU_LIMB_BITS == 64 && 2 * i + 1 < nn)
            m[i] |= (ryu_limb)((ryu_dlimb)n[2 * i + 1] << 32);
    }
    for (bits = nm * RYU_LIMB_BITS; !(m[nm - 1] >> ((bits - 1) % RYU_LIMB_BITS)); bits--)
        ;

    q[0] = 1;
    q[1] = 0;
    *sticky = 1;

    // Values beyond 10^max_exp or below 10^-(-min_exp + 2 * mant_dig + bits) are out of
    // range, which also keeps e10 within the range of generic_pow5bits.
    if (e10 >= max_exp)
        return max_exp + 1;
    if (-e10 >= -min_exp + 2 * mant_dig + bits)
        return min_exp - 2 * mant_dig - 4;

    // The length of n * 10^e10 in bits is:
    //   log2(n * 10^e10) = log2(n) + e10 + e10 * log2(5)
    //
    // We want to compute the mant_dig + 2 top-most bits (the significand plus a guard
    // bit). We therefore choose a binary output exponent of
    //   floor(log2(n)) + e10 + floor(log2(5^e10)) - (mant_dig + 1)
    // which leaves 2^(mant_dig + 1) <= n * 10^e10 / 2^e2 < 2^(mant_dig + 3)
    if (e10 >= 0)
        e2 = bits - 1 + e10 + (int)generic_pow5bits(e10) - 1 - (mant_dig + 1);
    else
        e2 = bits - 1 + e10 - (int)generic_pow5bits(-e10) - (mant_dig + 1);

    // Values at least 2^max_exp overflow and values below half the smallest
    // subnormal underflow in every rounding direction.
    if (e2 >= max_exp - mant_dig - 1)
        return max_exp + 1;
    if (e2 <= min_exp - 2 * mant_dig - 4)
        return min_exp - 2 * mant_dig - 4;

#ifdef ATOD_RYU_LONG_DOUBLE
    {
        uint64_t t[4];
        if (e10 >= 0)
            generic_computePow5(e10, t);
        else
            generic_computeInvPow5(-e10, t);
        pow5_limbs(t, 4, pow5);
        pow5_bits = FLOAT_128_POW5_BITCOUNT;
        pow5_err = 1;
    }
#else
#if __LDBL_MANT_DIG__ == 106
    if (mant_dig > 53) {
        __atod_ryu_pow5(e10 >= 0 ? e10 : -e10, e10 < 0, pow5);
        pow5_bits = ATOD_RYU_BITCOUNT;
        pow5_err = ATOD_RYU_ERR;
    } else
#endif
    {
        uint64_t t[2];
        if (e10 >= 0)
            __double_computePow5(e10, t);
        else
            __double_computeInvPow5(-e10, t);
        pow5_limbs(t, 2, pow5);
        pow5_bits = DOUBLE_POW5_BITCOUNT;
        pow5_err = 1;
    }
#endif
    np = nm + RYU_LIMBS(pow5_bits);
    ryu_mul(m, nm, pow5, RYU_LIMBS(pow5_bits), p);

    // r = p + n * pow5_err (5^e10 is low) or p - n * pow5_err (5^-e10 is high),
    // the other end of the range holding the exact product.
    carry = 0;
    for (i = 0; i < nm; i++) {
        carry += (ryu_dlimb)m[i] * pow5_err;
        m[i] = (ryu_limb)carry;
        carry >>= RYU_LIMB_BITS;
    }
    m[nm] = (ryu_limb)carry;
    carry = 0;
    for (i = 0; i < np; i++) {
        ryu_limb a = i <= nm ? m[i] : 0;
        if (e10 >= 0)
            carry = (ryu_dlimb)p[i] + a + carry;
        else
            carry = (ryu_dlimb)p[i] - a - carry;
        r[i] = (ryu_limb)carry;
        carry = (carry >> RYU_LIMB_BITS) & 1;
    }

    // m2 is the product shifted right by bits + pow5_bits - mant_dig - 3, which
    // leaves mant_dig + 2 or mant_dig + 3 bits.
    shift = bits + pow5_bits - mant_dig - 3;
    exact = 0;
    for (i = 0; i < Q_LIMBS; i++) {
        qp[i] = ryu_shift_limb(p, np, shift, i);
        qr[i] = ryu_shift_limb(r, np, shift, i);
        if (qp[i] != qr[i])
            exact = 1;
    }

    if (exact) {
        // A multiple of 2^shift lies between p and r: the value is exact, and for
        // e10 >= 0 the larger of the two.
        if (e10 >= 0)
            memcpy(qp, qr, sizeof(qp));
        *sticky = 0;
    } else if (e10 >= 0) {
        // The approximation of 5^e10 may also be exact
        low = p[shift / RYU_LIMB_BITS] << (RYU_LIMB_BITS - 1 - shift % RYU_LIMB_BITS) << 1;
        for (i = 0; i < shift / RYU_LIMB_BITS; i++)
            low |= p[i];
        *sticky = low != 0;
    }
    for (i = 0; i < 2; i++)
        q[i] = RYU_LIMB_BITS == 64 ? qp[i] : qp[2 * i] | (uint64_t)qp[2 * i + 1] << 32;
    return e2;
}
