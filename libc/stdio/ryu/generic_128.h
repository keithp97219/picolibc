// Copyright 2018 Ulf Adams
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
#ifndef RYU_GENERIC_128_H
#define RYU_GENERIC_128_H

/*
 * Helpers for the 128-bit Ryu code, used for long double formats
 * with more than 64 bits: Intel/Motorola 80-bit and IEEE binary128.
 * 128-bit values use the _u128 type from local-int128.h, which is
 * emulated with two 64-bit values where the compiler lacks
 * __uint128_t.
 */

#include "common.h"
#include "../local-int128.h"

#define FLOAT_128_POW5_INV_BITCOUNT 251
#define FLOAT_128_POW5_BITCOUNT     251

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 32768.
static inline uint32_t
generic_pow5bits(const int32_t e)
{
    assert(e >= 0);
    assert(e <= 1 << 15);
    return (uint32_t)((((uint64_t)e * 163391164108059ull) >> 46) + 1);
}

// Returns floor(log_10(2^e)).
static inline uint32_t
generic_log10Pow2(const int32_t e)
{
    // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
    assert(e >= 0);
    assert(e <= 1 << 15);
    return (uint32_t)((((uint64_t)e) * 169464822037455ull) >> 49);
}

// Returns floor(log_10(5^e)).
static inline uint32_t
generic_log10Pow5(const int32_t e)
{
    // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
    assert(e >= 0);
    assert(e <= 1 << 15);
    return (uint32_t)((((uint64_t)e) * 196742565691928ull) >> 48);
}

// Computes 5^i in the form required by Ryu, and stores it in the given pointer.
void __generic_computePow5(const uint32_t i, uint64_t * const result);
#define generic_computePow5(i, r) __generic_computePow5(i, r)

// Computes 5^-i in the form required by Ryu, and stores it in the given pointer.
void __generic_computeInvPow5(const uint32_t i, uint64_t * const result);
#define generic_computeInvPow5(i, r) __generic_computeInvPow5(i, r)

/*
 * Multi-word arithmetic for decimal input (atod_ryu.c), in words of
 * the native size: 64 bits where the compiler has a 128-bit type for
 * the products, 32 bits otherwise.
 */
#if defined(__SIZEOF_INT128__) && __SIZEOF_POINTER__ >= 8
typedef uint64_t    ryu_limb;
typedef __uint128_t ryu_dlimb;
#define RYU_LIMB_BITS 64
#else
typedef uint32_t ryu_limb;
typedef uint64_t ryu_dlimb;
#define RYU_LIMB_BITS 32
#endif

// Words needed for bits
#define RYU_LIMBS(bits) (((bits) + RYU_LIMB_BITS - 1) / RYU_LIMB_BITS)

// r[0 .. na + nb) = a[0 .. na) * b[0 .. nb)
static inline void
ryu_mul(const ryu_limb * const a, const int na, const ryu_limb * const b, const int nb,
        ryu_limb * const r)
{
    int i, j;

    for (j = 0; j < na + nb; j++)
        r[j] = 0;
    for (i = 0; i < na; i++) {
        ryu_dlimb carry = 0;
        for (j = 0; j < nb; j++) {
            carry += (ryu_dlimb)a[i] * b[j] + r[i + j];
            r[i + j] = (ryu_limb)carry;
            carry >>= RYU_LIMB_BITS;
        }
        r[i + nb] = (ryu_limb)carry;
    }
}

// Returns word k of a[0 .. na) >> shift
static inline ryu_limb
ryu_shift_limb(const ryu_limb * const a, const int na, const int shift, const int k)
{
    int      w = shift / RYU_LIMB_BITS + k;
    int      b = shift % RYU_LIMB_BITS;
    ryu_limb v = 0;

    if (w < na)
        v = a[w] >> b;
    if (b && w + 1 < na)
        v |= a[w + 1] << (RYU_LIMB_BITS - b);
    return v;
}

// Sets result to (mul[0 .. n) * 5^offset >> shift) + add, in RYU_LIMBS(64 * n)
// words, for n <= 5, offset < 111 and a result that fits.
static inline void
generic_mul_pow5_shift(const uint64_t * const mul, const int n, const uint32_t offset,
                       const uint32_t shift, const ryu_limb add, ryu_limb * const result)
{
    ryu_limb  a[RYU_LIMBS(64 * 5)], m[RYU_LIMBS(256)], p[RYU_LIMBS(64 * 5 + 256)];
    ryu_dlimb carry;
    int       na = n * (64 / RYU_LIMB_BITS);
    int       nm = 1, i, j;
    uint32_t  k = offset;

    for (j = 0; j < na; j++)
        a[j] = (ryu_limb)(mul[j * RYU_LIMB_BITS / 64] >> (j * RYU_LIMB_BITS % 64));

    // m = 5^offset, as many powers of five at a time as fit in a word
    m[0] = 1;
    while (k) {
        ryu_limb f = 1;
        for (j = 0; j < (RYU_LIMB_BITS == 64 ? 27 : 13) && k; j++, k--)
            f *= 5;
        carry = 0;
        for (i = 0; i < nm; i++) {
            carry += (ryu_dlimb)m[i] * f;
            m[i] = (ryu_limb)carry;
            carry >>= RYU_LIMB_BITS;
        }
        if (carry)
            m[nm++] = (ryu_limb)carry;
    }

    ryu_mul(a, na, m, nm, p);
    carry = add;
    for (j = 0; j < na; j++) {
        carry += ryu_shift_limb(p, na + nm, shift, j);
        result[j] = (ryu_limb)carry;
        carry >>= RYU_LIMB_BITS;
    }
}

// Computes 5^i (or 5^-i when inv) for decimal input of double-double in
// atod_ryu.c, from the tables in ryu_table_atod.c without error tables:
// 5^i is low and 5^-i high by less than ATOD_RYU_ERR units.
#define ATOD_RYU_ERR      3
#define ATOD_RYU_BITCOUNT FLOAT_128_POW5_BITCOUNT
void __atod_ryu_pow5(const uint32_t i, const bool inv, ryu_limb * const result);

// _u128_mul_256_shift, out of line
void __mul_256_shift(const uint64_t * const a, const int na, const uint64_t * const b,
                     const uint32_t shift, const uint32_t corr, uint64_t * const result);
#define mul_128_256_shift(a, b, s, c, r) __mul_256_shift(a, 2, b, s, c, r)
#define mul_256_256_shift(a, b, s, c, r) __mul_256_shift(a, 4, b, s, c, r)

static inline uint32_t
pow5Factor_128(_u128 value)
{
    uint32_t count;
    uint32_t rem;

    for (count = 0; !_u128_is_zero(value); ++count) {
        value = _u128_divmod_5(value, &rem);
        if (rem != 0)
            return count;
    }
    return 0;
}

// Returns true if value is divisible by 5^p.
static inline bool
multipleOfPowerOf5_128(const _u128 value, const uint32_t p)
{
    // I tried a case distinction on p, but there was no performance difference.
    return pow5Factor_128(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool
multipleOfPowerOf2_128(const _u128 value, const uint32_t p)
{
    return _u128_is_zero(_u128_and(value, _u128_minus_64(_u128_lshift(to_u128(1), p), 1)));
}

static inline _u128
mulShift_128(const _u128 m, const uint64_t * const mul, const int32_t j)
{
    assert(j > 0 && j < 384);
    uint64_t a[2];
    a[0] = _u128_lo64(m);
    a[1] = _u128_hi64(m);
    uint64_t result[4];
    mul_128_256_shift(a, mul, j, 0, result);
    return _u128_make(result[1], result[0]);
}

#endif // RYU_GENERIC_128_H
