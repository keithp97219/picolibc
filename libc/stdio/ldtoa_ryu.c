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
 * IEEE binary128 formats, from Ryu's generic_128.c with the same
 * changes as d2d in dtoa_ryu.c: stop at max_digits, round to
 * max_decimals digits after the decimal point in 'f' mode and fix up
 * a carry out of the top digit. 128-bit values use _u128, which works
 * without compiler support for __uint128_t.
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
 * m2 is the significand including the leading one, mmShift is set
 * unless the gap to the next lower value is half the gap to the next
 * higher one.
 */
static floating_decimal_128
ld2d(const _u128 m2, const uint32_t ieeeExponent, const uint32_t mmShift, int max_digits,
     bool fmode, int max_decimals)
{
    // We subtract 2 so that the bounds computation has 2 additional bits.
    const int32_t e2 = (int32_t)(ieeeExponent == 0 ? LONG_DOUBLE_DENORM_EXP : ieeeExponent)
        - LONG_DOUBLE_BIAS - LONG_DOUBLE_MANTISSA_BITS - 2;
    const bool even = (_u128_lo64(m2) & 1) == 0;
    const bool acceptBounds = even;
    /* Set if we're truncating more digits to meet format request */
    bool       truncate_max = false;

#ifdef RYU_DEBUG
    printf("-> %s * 2^%d\n", s(m2), e2 + 2);
#endif

    // Step 2: Determine the interval of valid decimal representations.
    const _u128 mv = _u128_lshift(m2, 2);
    // We would compute mp and mm like this:
    // uint128_t mp = 4 * m2 + 2;
    // uint128_t mm = mv - 1 - mmShift;
    const _u128 mp = _u128_plus_64(mv, 2);
    const _u128 mm = _u128_minus_64(mv, 1 + mmShift);

    // Step 3: Convert to a decimal power base using 256-bit arithmetic.
    _u128       vr, vp, vm;
    int32_t     e10;
    bool        vmIsTrailingZeros = false;
    bool        vrIsTrailingZeros = false;
    uint64_t    pow5[4];
    if (e2 >= 0) {
        // I tried special-casing q == 0, but there was no effect on performance.
        // This expression is slightly faster than max(0, log10Pow2(e2) - 1).
        const uint32_t q = generic_log10Pow2(e2) - (e2 > 3);
        e10 = (int32_t)q;
        const int32_t k = FLOAT_128_POW5_INV_BITCOUNT + (int32_t)generic_pow5bits((int32_t)q) - 1;
        const int32_t i = -e2 + (int32_t)q + k;
        generic_computeInvPow5(q, pow5);
        vr = mulShift_128(mv, pow5, i);
        vp = mulShift_128(mp, pow5, i);
        vm = mulShift_128(mm, pow5, i);
#ifdef RYU_DEBUG
        printf("%s * 2^%d / 10^%u\n", s(mv), e2, q);
#endif
        // floor(log_5(2^128)) = 55, this is very conservative
        if (q <= 55) {
            // Only one of mp, mv, and mm can be a multiple of 5, if any.
            uint32_t mvMod5;
            _u128_divmod_5(mv, &mvMod5);
            if (mvMod5 == 0) {
                vrIsTrailingZeros = multipleOfPowerOf5_128(mv, q);
            } else if (acceptBounds) {
                // Same as min(e2 + (~mm & 1), pow5Factor(mm)) >= q
                // <=> e2 + (~mm & 1) >= q && pow5Factor(mm) >= q
                // <=> true && pow5Factor(mm) >= q, since e2 >= q.
                vmIsTrailingZeros = multipleOfPowerOf5_128(mm, q);
            } else {
                // Same as min(e2 + 1, pow5Factor(mp)) >= q.
                vp = _u128_minus_64(vp, multipleOfPowerOf5_128(mp, q));
            }
        }
    } else {
        // This expression is slightly faster than max(0, log10Pow5(-e2) - 1).
        const uint32_t q = generic_log10Pow5(-e2) - (-e2 > 1);
        e10 = (int32_t)q + e2;
        const int32_t i = -e2 - (int32_t)q;
        const int32_t k = (int32_t)generic_pow5bits(i) - FLOAT_128_POW5_BITCOUNT;
        const int32_t j = (int32_t)q - k;
        generic_computePow5(i, pow5);
        vr = mulShift_128(mv, pow5, j);
        vp = mulShift_128(mp, pow5, j);
        vm = mulShift_128(mm, pow5, j);
#ifdef RYU_DEBUG
        printf("%s * 5^%d / 10^%u\n", s(mv), -e2, q);
        printf("%u %d %d %d\n", q, i, k, j);
#endif
        if (q <= 1) {
            // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
            // mv = 4 * m2, so it always has at least two trailing 0 bits.
            vrIsTrailingZeros = true;
            if (acceptBounds) {
                // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
                vmIsTrailingZeros = mmShift == 1;
            } else {
                // mp = mv + 2, so it always has at least one trailing 0 bit.
                vp = _u128_minus_64(vp, 1);
            }
        } else if (q < 127) {
            // We want to know if the full product has at least q trailing zeros.
            // We need to compute min(ntz(mv), pow5Factor(mv) - e2) >= q
            // <=> ntz(mv) >= q  &&  pow5Factor(mv) - e2 >= q
            // <=> ntz(mv) >= q    (e2 is negative and -e2 >= q)
            vrIsTrailingZeros = multipleOfPowerOf2_128(mv, q);
        }
    }
#ifdef RYU_DEBUG
    printf("e10=%d\n", e10);
    printf("V+=%s\n", s(vp));
    printf("V =%s\n", s(vr));
    printf("V-=%s\n", s(vm));
    printf("vm is trailing zeros=%s\n", vmIsTrailingZeros ? "true" : "false");
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

    // Step 4: Find the shortest decimal representation in the interval of valid representations.
    int32_t  removed = 0;
    uint8_t  lastRemovedDigit = 0;
    _u128    output;
    uint32_t vpMod10, vmMod10, vrMod10;
    int      vrLength = _u128_digits(vr);

    /*
     * If limiting decimals, then limit the max digits to no more than
     * the number of digits left of the decimal plus the number of
     * digits right of the decimal. See d2d in dtoa_ryu.c
     */

    int      save_max_digits = max_digits;
    if (fmode) {
        int exp = e10 + vrLength - 1;
        max_digits = min_int(max_digits, max_int(max_decimals < 0, max_decimals + exp + 1));
    }

    for (;;) {
        const _u128 vpDiv10 = _u128_divmod_10(vp, &vpMod10);
        const _u128 vmDiv10 = _u128_divmod_10(vm, &vmMod10);
        if (!_u128_gt(vpDiv10, vmDiv10)) {
            if (vrLength <= max_digits || (max_digits == 0 && _u128_is_zero(vr)))
                break;
            else
                truncate_max = true;
        }
        const _u128 vrDiv10 = _u128_divmod_10(vr, &vrMod10);
        vmIsTrailingZeros &= vmMod10 == 0;
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t)vrMod10;
        vr = vrDiv10;
        vp = vpDiv10;
        vm = vmDiv10;
        if (vrLength > 1)
            --vrLength;
        ++removed;
    }
#ifdef RYU_DEBUG
    printf("V+=%s\n", s(vp));
    printf("V =%s\n", s(vr));
    printf("V-=%s\n", s(vm));
    printf("d-10=%s\n", vmIsTrailingZeros ? "true" : "false");
#endif
    if (vmIsTrailingZeros) {
        while (!_u128_is_zero(vm)) {
            const _u128 vmDiv10 = _u128_divmod_10(vm, &vmMod10);
            if (vmMod10 != 0)
                break;
            const _u128 vrDiv10 = _u128_divmod_10(vr, &vrMod10);
            vrIsTrailingZeros &= lastRemovedDigit == 0;
            lastRemovedDigit = (uint8_t)vrMod10;
            vr = vrDiv10;
            vm = vmDiv10;
            if (vrLength > 1)
                --vrLength;
            ++removed;
        }
    }
#ifdef RYU_DEBUG
    printf("%s %d\n", s(vr), lastRemovedDigit);
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    if (vrIsTrailingZeros && lastRemovedDigit == 5 && (_u128_lo64(vr) & 1) == 0) {
        // Round even if the exact number is .....50..0.
        lastRemovedDigit = 4;
    }

    output = vr;
    e10 += removed;

    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    uint8_t carry = ((!truncate_max && _u128_eq(vr, vm) && (!acceptBounds || !vmIsTrailingZeros))
                     || lastRemovedDigit >= 5);
    int     len = vrLength;

    if (carry) {
        output = _u128_plus_64(output, 1);
        len = _u128_digits(output);

        /* This can only happen if output has carried out of the top digit */
        if (len > max_digits) {

            /* Recompute max digits in this case */
            if (fmode) {
                int exp = e10 + len - 1;
                max_digits = min_int(save_max_digits, max_int(1, max_decimals + exp + 1));
            }

            if (len > max_digits) {
                output = _u128_div_10(_u128_plus_64(output, 5));
                e10++;
                len--;
            }
        }
    }
    if (len > max_digits)
        len = max_digits;

#ifdef RYU_DEBUG
    printf("O=%s\n", s(output));
    printf("EXP=%d\n", e10);
#endif

    floating_decimal_128 fd;
    fd.exponent = e10;
    fd.olength = len;
    fd.mantissa = output;
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
    uint32_t mmShift;

#if __LDBL_MANT_DIG__ == 64
    int32_t  se;
    uint32_t msw, lsw;

    GET_LDOUBLE_WORDS(se, msw, lsw, x);

    /* This format has an explicit leading one */
    const uint64_t ieeeMantissa = ((uint64_t)msw << 32) | lsw;
    const uint64_t leading = 1ull << LONG_DOUBLE_MANTISSA_BITS;

    ieeeSign = (se & 0x8000) != 0;
    ieeeExponent = (uint32_t)se & 0x7fff;
    isZero = ieeeMantissa == 0;
#ifdef __m68k__
    /* The leading bit doesn't matter for Motorola infinities */
    isNaN = (ieeeMantissa & (leading - 1)) != 0;
    mmShift = ieeeMantissa != leading || ieeeExponent == 0;
#else
    isNaN = ieeeMantissa != leading;
    mmShift = ieeeMantissa != leading || ieeeExponent <= 1;
#endif
    m2 = to_u128(ieeeMantissa);
#else
    uint64_t hi, lo;

    GET_LDOUBLE_WORDS64(hi, lo, x);

    const uint64_t ieeeMantissaHi = hi & ((1ull << (LONG_DOUBLE_MANTISSA_BITS - 64)) - 1);

    ieeeSign = (hi >> 63) != 0;
    ieeeExponent = (uint32_t)(hi >> (LONG_DOUBLE_MANTISSA_BITS - 64)) & 0x7fff;
    const bool hasFraction = ieeeMantissaHi != 0 || lo != 0;

    isZero = ieeeExponent == 0 && !hasFraction;
    isNaN = hasFraction;
    mmShift = hasFraction || ieeeExponent <= 1;
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

    floating_decimal_128 v = ld2d(m2, ieeeExponent, mmShift, max_digits, fmode, max_decimals);

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
