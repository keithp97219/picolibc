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

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.

/*
 * Long double to decimal conversion for the Intel/Motorola 80-bit and
 * IEEE binary128 formats, using the multiply-and-shift from Ryu's
 * generic_128.c the way __ryu_d2d (ryu_d2d.c) does for double: compute
 * the exact leading digits, then round them half-even to the
 * requested number. 128-bit values use _u128, which works without
 * compiler support for __uint128_t.
 *
 * The double-double format is handled by ldtoa_engine.c
 */

#define _NEED_IO_LONG_DOUBLE

#include "dtoa.h"

#if defined(_NEED_IO_FLOAT_LARGE) && (__LDBL_MANT_DIG__ == 64 || __LDBL_MANT_DIG__ == 113)

#include "ryu/generic_128.h"
#include "../../libm/ld/math_ld.h"

#ifdef RYU_DEBUG
#include <inttypes.h>
#include <stdio.h>

static const char *
s(_u128 v)
{
    static char b[40];
    int         len = _u128_digits(v);
    uint32_t    c;

    b[len] = '\0';
    while (len--) {
        v = _u128_divmod_10(v, &c);
        b[len] = '0' + c;
    }
    return b;
}
#endif

#define LONG_DOUBLE_EXPONENT_BITS 15
#define LONG_DOUBLE_BIAS          16383
/* Mantissa bits after the leading one */
#define LONG_DOUBLE_MANTISSA_BITS (__LDBL_MANT_DIG__ - 1)

/*
 * The exponent of values with a biased exponent of zero. Motorola's
 * 80-bit format, unlike Intel's, doesn't adjust that to match the
 * smallest normal exponent
 */
#if __LDBL_MANT_DIG__ == 64 && defined(__m68k__)
#define LONG_DOUBLE_DENORM_EXP 0
#else
#define LONG_DOUBLE_DENORM_EXP 1
#endif

// A floating decimal representing m * 10^e.
typedef struct floating_decimal_128 {
    _u128   mantissa;
    int32_t exponent;
    int16_t olength;
} floating_decimal_128;

/*
 * Compute the decimal digits of m2 * 2^e2, correctly rounded
 * (round-half-even) to max_digits digits. In fmode, limit the result
 * to max_decimals digits past the decimal point (before the decimal
 * point if negative).
 *
 * m2 must be normalized so that its top bit is bit
 * LONG_DOUBLE_MANTISSA_BITS (subnormal values are shifted up to
 * match). mk-ryu-table-128.py checks that the tables then yield an
 * exact value for floor(m2 * 2^e2 / 10^e10) with at least
 * LDTOA_MAX_DIG + 1 digits (22 for 80-bit values, 37 for binary128)
 * across the whole exponent range, so up to LDTOA_MAX_DIG digits can
 * always be generated exactly.
 */
static floating_decimal_128
ld2d(const _u128 m2, int32_t e2, int max_digits, bool fmode, int max_decimals)
{
    // Ryu works with 4 * m2 so that the bounds can be computed too; keep
    // that scaling so that the precision analysis still applies.
    const _u128 mv = _u128_lshift(m2, 2);
    _u128       vr;
    int32_t     e10;
    bool        vrIsTrailingZeros;
    uint64_t    pow5[4];

    e2 -= 2;

#ifdef RYU_DEBUG
    printf("-> %s * 2^%d\n", s(mv), e2);
#endif

    /*
     * Unlike Ryu, which picks q one below floor(log10(2^e2)), use two
     * below that, so that vr has one more digit than needed.
     */
    if (e2 >= 0) {
        const int32_t  lq = (int32_t)generic_log10Pow2(e2) - 2;
        const uint32_t q = lq < 0 ? 0 : (uint32_t)lq;
        e10 = (int32_t)q;
        const int32_t k = FLOAT_128_POW5_INV_BITCOUNT + (int32_t)generic_pow5bits((int32_t)q) - 1;
        const int32_t i = -e2 + (int32_t)q + k;
        generic_computeInvPow5(q, pow5);
        vr = mulShift_128(mv, pow5, i);
        // vr = floor(mv * 2^(e2 - q) / 5^q), which is exact iff 5^q divides mv
        vrIsTrailingZeros = multipleOfPowerOf5_128(mv, q);
#ifdef RYU_DEBUG
        printf("%s * 2^%d / 10^%u\n", s(mv), e2, q);
#endif
    } else {
        const int32_t  lq = (int32_t)generic_log10Pow5(-e2) - 2;
        const uint32_t q = lq < 0 ? 0 : (uint32_t)lq;
        e10 = (int32_t)q + e2;
        const int32_t i = -e2 - (int32_t)q;
        const int32_t k = (int32_t)generic_pow5bits(i) - FLOAT_128_POW5_BITCOUNT;
        const int32_t j = (int32_t)q - k;
        generic_computePow5((uint32_t)i, pow5);
        vr = mulShift_128(mv, pow5, j);
        // vr = floor(mv * 5^i / 2^q), which is exact iff 2^q divides mv
        vrIsTrailingZeros = q < 128 && multipleOfPowerOf2_128(mv, q);
#ifdef RYU_DEBUG
        printf("%s * 5^%d / 10^%u\n", s(mv), -e2, q);
        printf("%u %d %d %d\n", q, i, k, j);
#endif
    }
#ifdef RYU_DEBUG
    printf("e10=%d\n", e10);
    printf("V =%s\n", s(vr));
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

    int len = _u128_digits(vr);

    /*
     * In fmode, limit the number of digits to those left of the
     * decimal point plus max_decimals. See __ryu_d2d.
     */
    int save_max_digits = max_digits;
    if (fmode) {
        int exp = e10 + len - 1;
        int ndigits = max_decimals + exp + 1;

        if (ndigits < 0 && max_decimals >= 0) {
            // Value is less than half of the last decimal place
            floating_decimal_128 zero = { _u128_zero, -max_decimals, 0 };
            return zero;
        }
        max_digits = min_int(max_digits, max_int(max_decimals < 0, ndigits));
    }

    // Remove digits, remembering the last one and whether the rest were zero
    uint8_t lastRemovedDigit = 0;
    while (len > max_digits) {
        uint32_t vrMod10;
        vr = _u128_divmod_10(vr, &vrMod10);
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t)vrMod10;
        e10++;
        len--;
    }

    if (vrIsTrailingZeros && lastRemovedDigit == 5 && (_u128_lo64(vr) & 1) == 0) {
        // Round even if the exact number is .....50..0.
        lastRemovedDigit = 4;
    }

    if (lastRemovedDigit >= 5) {
        vr = _u128_plus_64(vr, 1);
        if (len == 0 || _u128_digits(vr) > len) {
            // Carried out of the top digit, giving a power of ten. In
            // fmode, the extra digit is left of the decimal point, so
            // there's room for it unless that would exceed the buffer.
            if (fmode ? len + 1 > save_max_digits : len + 1 > max_digits) {
                vr = _u128_div_10(vr);
                e10++;
            } else {
                len++;
            }
        }
    }

#ifdef RYU_DEBUG
    printf("O=%s\n", s(vr));
    printf("EXP=%d\n", e10);
#endif

    floating_decimal_128 fd;
    fd.mantissa = vr;
    fd.exponent = e10;
    fd.olength = (int16_t)len;
    return fd;
}

int
__ldtoa_engine(long double x, struct dtoa *dtoa, int max_digits, bool fmode, int max_decimals)
{
    // Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
    _u128    m2;
    uint32_t ieeeExponent;
    bool     ieeeSign;
    bool     isZero, isNaN;

#if __LDBL_MANT_DIG__ == 64
    int32_t  se;
    uint32_t msw, lsw;

    GET_LDOUBLE_WORDS(se, msw, lsw, x);

    /* This format has an explicit leading one */
    const uint64_t ieeeMantissa = ((uint64_t)msw << 32) | lsw;

    ieeeSign = (se & 0x8000) != 0;
    ieeeExponent = (uint32_t)se & 0x7fff;
    isZero = ieeeMantissa == 0;
#ifdef __m68k__
    /* The leading bit doesn't matter for Motorola infinities */
    isNaN = (ieeeMantissa << 1) != 0;
#else
    isNaN = ieeeMantissa != (1ull << LONG_DOUBLE_MANTISSA_BITS);
#endif
    m2 = to_u128(ieeeMantissa);
#else
    uint64_t hi, lo;

    GET_LDOUBLE_WORDS64(hi, lo, x);

    const uint64_t ieeeMantissaHi = hi & ((1ull << (LONG_DOUBLE_MANTISSA_BITS - 64)) - 1);

    ieeeSign = (hi >> 63) != 0;
    ieeeExponent = (uint32_t)(hi >> (LONG_DOUBLE_MANTISSA_BITS - 64)) & 0x7fff;
    isNaN = ieeeMantissaHi != 0 || lo != 0;
    isZero = ieeeExponent == 0 && !isNaN;
    m2 = _u128_make(ieeeMantissaHi, lo);
    if (ieeeExponent != 0)
        m2 = _u128_or(m2, _u128_lshift(to_u128(1), LONG_DOUBLE_MANTISSA_BITS));
#endif

#ifdef RYU_DEBUG
    printf("IN=%c %u %s\n", ieeeSign ? '-' : '+', ieeeExponent, s(m2));
#endif

    uint8_t flags = 0;

    if (ieeeSign)
        flags |= DTOA_MINUS;
    if (ieeeExponent == ((1u << LONG_DOUBLE_EXPONENT_BITS) - 1u)) {
        if (isNaN) {
            flags |= DTOA_NAN;
        } else {
            flags |= DTOA_INF;
        }
        dtoa->flags = flags;
        return 0;
    }
    if (isZero) {
        flags |= DTOA_ZERO;
        dtoa->digits[0] = '0';
        dtoa->flags = flags;
        dtoa->exp = 0;
        return 1;
    }

    int32_t e2 = (int32_t)(ieeeExponent == 0 ? LONG_DOUBLE_DENORM_EXP : ieeeExponent)
        - LONG_DOUBLE_BIAS - LONG_DOUBLE_MANTISSA_BITS;

    // Normalize subnormals (and unnormals) so that there are enough significant bits
    while (_u128_lt(m2, _u128_lshift(to_u128(1), LONG_DOUBLE_MANTISSA_BITS))) {
        m2 = _u128_lshift(m2, 1);
        e2--;
    }

    // Don't generate more digits than the conversion is defined for
    if (max_digits > LDTOA_MAX_DIG)
        max_digits = LDTOA_MAX_DIG;

    floating_decimal_128 v = ld2d(m2, e2, max_digits, fmode, max_decimals);

    int32_t              olength = v.olength;
    int32_t              exp = v.exponent + olength - 1;
    uint64_t             low, high = 0;
    int                  i;

    /* Split into 19-digit pieces so that each fits in 64 bits */
    if (olength > 19)
        high = _u128_divmod_1e19(v.mantissa, &low);
    else
        low = _u128_lo64(v.mantissa);

    for (i = olength - 1; i >= 0; i--) {
        if (i == olength - 20)
            low = high;
        uint64_t q = _u64_div10(low);
        dtoa->digits[i] = (char)((uint32_t)low - 10 * (uint32_t)q) + '0';
        low = q;
    }

    dtoa->exp = exp;
    dtoa->flags = flags;
    return olength;
}

#endif /* _NEED_IO_FLOAT_LARGE && (__LDBL_MANT_DIG__ == 64 || __LDBL_MANT_DIG__ == 113) */
