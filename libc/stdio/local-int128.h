/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LOCAL_INT128_H_
#define _LOCAL_INT128_H_

/*
 * 128-bit unsigned integers and the operations picolibc uses on them,
 * with a struct of two 64-bit values where the compiler doesn't
 * provide __uint128_t.
 *
 * Define _U128_EMULATE to use the struct version (and the 32-bit
 * division helpers) even when the compiler has __uint128_t; this is
 * only useful for testing.
 */

#include <stdbool.h>
#include <stdint.h>

/* 64x64->128 multiply, from ryu_umul128.c */
uint64_t __umul128(uint64_t a, uint64_t b, uint64_t *productHi);

#if defined(__SIZEOF_INT128__) && !defined(_U128_EMULATE)
typedef __uint128_t _u128;
typedef __int128_t  _i128;
#define to_u128(x)             (x)
#define from_u128(x)           (x)
#define _u128_to_ld(a)         ((long double)(a))
#define _u128_is_zero(a)       ((a) == 0)
#define _i128_lt_zero(a)       ((_i128)(a) < 0)
#define _u128_plus_64(a, b)    ((a) + (b))
#define _u128_plus(a, b)       ((a) + (b))
#define _u128_minus(a, b)      ((a) - (b))
#define _u128_minus_64(a, b)   ((a) - (b))
#define _u128_times_10(a)      ((a) * 10)
#define _u128_times_base(a, b) ((a) * (b))
#define _u128_to_ld(a)         ((long double)(a))
#define _u128_oflow(a)                                                                   \
    ((a) >= (((((_u128)0xffffffffffffffffULL) << 64) | 0xffffffffffffffffULL) - 9 / 10))
#define _u128_zero            (_u128)0
#define _u128_lshift(a, b)    ((_u128)(a) << (b))
#define _u128_lshift_64(a, b) ((_u128)(a) << (b))
#define _u128_rshift(a, b)    ((a) >> (b))
#define _i128_rshift(a, b)    ((_i128)(a) >> (b))
#define _u128_or_64(a, b)     ((a) | (_u128)(b))
#define _u128_and_64(a, b)    ((uint64_t)(a) & (b))
#define _u128_or(a, b)        ((a) | (b))
#define _u128_and(a, b)       ((a) & (b))
#define _u128_eq(a, b)        ((a) == (b))
#define _u128_ge(a, b)        ((a) >= (b))
#define _i128_ge(a, b)        ((_i128)(a) >= (_i128)(b))
#define _u128_lt(a, b)        ((a) < (b))
#define _i128_lt(a, b)        ((_i128)(a) < (_i128)(b))
#define _u128_not(a)          (~(a))
#define _u128_make(hi, lo)    ((((_u128)(hi)) << 64) | (uint64_t)(lo))
#define _u128_hi64(a)         ((uint64_t)((a) >> 64))
#define _u128_lo64(a)         ((uint64_t)(a))
#define _u128_mul_64(a, b)    ((_u128)(uint64_t)(a) * (uint64_t)(b))
#define _u128_times_64(a, b)  ((a) * (uint64_t)(b))
#else
typedef struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint64_t lo, hi;
#else
    uint64_t hi, lo;
#endif
} _u128;
#define _u128_zero (_u128) { 0, 0 }

static inline _u128
to_u128(uint64_t x)
{
    _u128 a = { .hi = 0, .lo = x };
    return a;
}

static inline uint64_t
from_u128(_u128 a)
{
    return a.lo;
}

static inline long double
_u128_to_ld(_u128 a)
{
    return (long double)a.hi * ((long double)(1LL << 32) * (long double)(1LL << 32))
        + (long double)a.lo;
}

static inline bool
_u128_is_zero(_u128 a)
{
    return a.hi == 0 && a.lo == 0;
}

static inline bool
_i128_lt_zero(_u128 a)
{
    return (int64_t)a.hi < 0;
}

static inline bool
_u128_eq(_u128 a, _u128 b)
{
    return (a.hi == b.hi) && (a.lo == b.lo);
}

static inline bool
_u128_lt(_u128 a, _u128 b)
{
    if (a.hi == b.hi)
        return a.lo < b.lo;
    return a.hi < b.hi;
}

static inline bool
_i128_lt(_u128 a, _u128 b)
{
    if (a.hi == b.hi) {
        if ((int64_t)a.hi < 0)
            return a.lo > b.lo;
        else
            return a.lo < b.lo;
    }
    return (int64_t)a.hi < (int64_t)b.hi;
}

static inline bool
_u128_ge(_u128 a, _u128 b)
{
    if (a.hi == b.hi)
        return a.lo >= b.lo;
    return a.hi >= b.hi;
}

static inline bool
_i128_ge(_u128 a, _u128 b)
{
    if (a.hi == b.hi) {
        if ((int64_t)a.hi < 0)
            return a.lo <= b.lo;
        else
            return a.lo >= b.lo;
    }
    return (int64_t)a.hi >= (int64_t)b.hi;
}

static inline _u128
_u128_plus_64(_u128 a, uint64_t b)
{
    _u128 v;

    v.lo = a.lo + b;
    v.hi = a.hi;
    if (v.lo < a.lo)
        v.hi++;
    return v;
}

static inline _u128
_u128_plus(_u128 a, _u128 b)
{
    _u128 v;

    v.lo = a.lo + b.lo;
    v.hi = a.hi + b.hi;
    if (v.lo < a.lo)
        v.hi++;
    return v;
}

static inline _u128
_u128_minus_64(_u128 a, uint64_t b)
{
    _u128 v;

    v.lo = a.lo - b;
    v.hi = a.hi;
    if (v.lo > a.lo)
        v.hi--;
    return v;
}

static inline _u128
_u128_minus(_u128 a, _u128 b)
{
    _u128 v;

    v.lo = a.lo - b.lo;
    v.hi = a.hi - b.hi;
    if (v.lo > a.lo)
        v.hi--;
    return v;
}

static inline _u128
_u128_lshift(_u128 a, int amt)
{
    _u128 v;

    if (amt == 0) {
        v = a;
    } else if (amt < 64) {
        v.lo = a.lo << amt;
        v.hi = (a.lo >> (64 - amt)) | (a.hi << amt);
    } else {
        v.lo = 0;
        v.hi = a.lo << (amt - 64);
    }
    return v;
}

static inline _u128
_u128_lshift_64(uint64_t a, int amt)
{
    _u128 v;

    if (amt == 0) {
        v.lo = a;
        v.hi = 0;
    } else if (amt < 64) {
        v.lo = a << amt;
        v.hi = (a >> (64 - amt));
    } else {
        v.lo = 0;
        v.hi = a << (amt - 64);
    }
    return v;
}

static inline _u128
_u128_rshift(_u128 a, int amt)
{
    _u128 v;

    if (amt == 0) {
        v = a;
    } else if (amt < 64) {
        v.lo = (a.hi << (64 - amt)) | (a.lo >> amt);
        v.hi = a.hi >> amt;
    } else {
        v.hi = 0;
        v.lo = a.hi >> (amt - 64);
    }
    return v;
}

static inline _u128
_u128_and(_u128 a, _u128 b)
{
    _u128 v;

    v.hi = a.hi & b.hi;
    v.lo = a.lo & b.lo;
    return v;
}

static inline uint64_t
_u128_and_64(_u128 a, uint64_t b)
{
    return a.lo & b;
}

static inline _u128
_u128_or(_u128 a, _u128 b)
{
    _u128 v;

    v.lo = a.lo | b.lo;
    v.hi = a.hi | b.hi;
    return v;
}

static inline _u128
_u128_or_64(_u128 a, uint64_t b)
{
    _u128 v;

    v.lo = a.lo | b;
    v.hi = a.hi;
    return v;
}

static inline _u128
_u128_not(_u128 a)
{
    _u128 v;

    v.lo = ~a.lo;
    v.hi = ~a.hi;
    return v;
}

static inline _u128
_u128_times_10(_u128 a)
{
    return _u128_plus(_u128_lshift(a, 3), _u128_lshift(a, 1));
}

static inline _u128
_u128_times_base(_u128 a, int base)
{
    if (base == 10)
        return _u128_times_10(a);
    return _u128_lshift(a, 4);
}

static inline bool
_u128_oflow(_u128 a)
{
    return a.hi >= (0xffffffffffffffffULL - 9) / 10;
}

static inline _u128
_u128_make(uint64_t hi, uint64_t lo)
{
    _u128 v;

    v.hi = hi;
    v.lo = lo;
    return v;
}

static inline uint64_t
_u128_hi64(_u128 a)
{
    return a.hi;
}

static inline uint64_t
_u128_lo64(_u128 a)
{
    return a.lo;
}

/* 64x64->128 multiply */
static inline _u128
_u128_mul_64(uint64_t a, uint64_t b)
{
    _u128 v;

    v.lo = __umul128(a, b, &v.hi);
    return v;
}

/* Low 128 bits of a 128x64 multiply */
static inline _u128
_u128_times_64(_u128 a, uint64_t b)
{
    _u128 v = _u128_mul_64(a.lo, b);

    v.hi += a.hi * b;
    return v;
}
#endif

static inline bool
_u128_gt(_u128 a, _u128 b)
{
    return _u128_lt(b, a);
}

/*
 * 64-bit division by small constants. Where 64-bit division is a
 * library call, multiply by the reciprocal instead
 */
#if __SIZEOF_POINTER__ < 8 || defined(_U128_EMULATE)
static inline uint64_t
_u64_umulh(uint64_t a, uint64_t b)
{
    uint64_t hi;

    __umul128(a, b, &hi);
    return hi;
}

static inline uint64_t
_u64_div5(uint64_t x)
{
    return _u64_umulh(x, 0xCCCCCCCCCCCCCCCDULL) >> 2;
}

static inline uint64_t
_u64_div10(uint64_t x)
{
    return _u64_umulh(x, 0xCCCCCCCCCCCCCCCDULL) >> 3;
}
#else
#define _u64_div5(x)  ((uint64_t)(x) / 5)
#define _u64_div10(x) ((uint64_t)(x) / 10)
#endif

/*
 * Divide by 5 and 10 without a 128-bit division (which would be a
 * library call even with __uint128_t). Split the dividend into 64-bit
 * halves; with 2^64 = 5 * 0x3333333333333333 + 1 = 10 *
 * 0x1999999999999999 + 6, the quotient is
 *
 *     hi / d * 2^64 + (hi % d) * (2^64 / d) + lo / d +
 *     ((hi % d) * (2^64 % d) + lo % d) / d
 *
 * and the last term is small enough for 32-bit arithmetic
 */
static inline _u128
_u128_divmod_5(_u128 a, uint32_t *rem)
{
    uint64_t hi = _u128_hi64(a);
    uint64_t lo = _u128_lo64(a);
    uint64_t qh = _u64_div5(hi);
    uint64_t ql = _u64_div5(lo);
    uint32_t rh = (uint32_t)hi - 5 * (uint32_t)qh;
    uint32_t t = rh + ((uint32_t)lo - 5 * (uint32_t)ql); /* <= 8 */
    uint32_t tq = t >= 5;

    *rem = t - 5 * tq;
    return _u128_make(qh, ql + rh * 0x3333333333333333ULL + tq);
}

static inline _u128
_u128_divmod_10(_u128 a, uint32_t *rem)
{
    uint64_t hi = _u128_hi64(a);
    uint64_t lo = _u128_lo64(a);
    uint64_t qh = _u64_div10(hi);
    uint64_t ql = _u64_div10(lo);
    uint32_t rh = (uint32_t)hi - 10 * (uint32_t)qh;
    uint32_t t = rh * 6 + ((uint32_t)lo - 10 * (uint32_t)ql); /* <= 63 */
    uint32_t tq = (t * 205) >> 11;                            /* t / 10 for t < 1029 */

    *rem = t - 10 * tq;
    return _u128_make(qh, ql + rh * 0x1999999999999999ULL + tq);
}

static inline _u128
_u128_div_10(_u128 a)
{
    uint32_t rem;

    return _u128_divmod_10(a, &rem);
}

/*
 * Split a into a / 10^19 and a % 10^19, for a < 10^19 * 2^64.
 *
 * Estimate the quotient as floor(a * m / 2^127) with m = floor(2^127
 * / 10^19). That is low by less than 3, which the loop fixes.
 */
#define _U128_1E19 10000000000000000000ULL

static inline uint64_t
_u128_divmod_1e19(_u128 a, uint64_t *rem)
{
    const uint64_t m = 17014118346046923173ULL;
    _u128          t
        = _u128_plus_64(_u128_mul_64(_u128_hi64(a), m), _u128_hi64(_u128_mul_64(_u128_lo64(a), m)));
    uint64_t q = _u128_lo64(_u128_rshift(t, 63));
    _u128    r = _u128_minus(a, _u128_mul_64(q, _U128_1E19));

    while (_u128_hi64(r) != 0 || _u128_lo64(r) >= _U128_1E19) {
        r = _u128_minus_64(r, _U128_1E19);
        q++;
    }
    *rem = _u128_lo64(r);
    return q;
}

/* Number of decimal digits in a, 1 for 0 */
static inline int
_u128_digits(_u128 a)
{
    int   len = 1;
    _u128 p = to_u128(10);

    while (len < 39 && _u128_ge(a, p)) {
        len++;
        p = _u128_times_10(p);
    }
    return len;
}

/*
 * Multiply the na-word (na <= 4) value a by the 256-bit value b (both
 * arrays of 64-bit words, least significant first), shift the product
 * right by shift (0 < shift < 64 * (na + 2)), add corr and return the
 * low 256 bits in result. With na == 2, this is mul_128_256_shift from
 * Ryu's generic_128.h, computed one 64-bit word at a time.
 */
static inline void
_u128_mul_256_shift(const uint64_t *a, int na, const uint64_t *b, uint32_t shift, uint32_t corr,
                    uint64_t *result)
{
    uint64_t p[10];
    uint32_t word = shift >> 6;
    uint32_t bit = shift & 63;
    uint64_t carry;
    int      i, j;

    for (j = 0; j < 10; j++)
        p[j] = 0;
    for (i = 0; i < na; i++) {
        carry = 0;
        for (j = 0; j < 4; j++) {
            _u128 t = _u128_plus_64(_u128_plus_64(_u128_mul_64(a[i], b[j]), p[i + j]), carry);
            p[i + j] = _u128_lo64(t);
            carry = _u128_hi64(t);
        }
        p[i + 4] = carry;
    }

    carry = corr;
    for (j = 0; j < 4; j++) {
        uint64_t v = p[j + word] >> bit;
        if (bit)
            v |= p[j + word + 1] << (64 - bit);
        v += carry;
        carry = v < carry;
        result[j] = v;
    }
}

static inline void
_u128_mul_128_256_shift(const uint64_t *a, const uint64_t *b, uint32_t shift, uint32_t corr,
                        uint64_t *result)
{
    _u128_mul_256_shift(a, 2, b, shift, corr, result);
}

#endif /* _LOCAL_INT128_H_ */
