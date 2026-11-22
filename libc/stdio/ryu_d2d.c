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

#include "local-stdio.h"

#include "ryu/common.h"
#include "ryu/d2s_intrinsics.h"

// Returns the number of decimal digits in v
static int
decimalLength(const uint64_t v)
{
    int      len = 1;
    uint64_t c = 10;
    while (c <= v) {
        len++;
        c = (c << 3) + (c << 1);
    }
    return len;
}

/*
 * Compute the decimal digits of m2 * 2^e2, correctly rounded
 * (round-half-even) to max_digits digits. In fmode, limit the result
 * to max_decimals digits past the decimal point (before the decimal
 * point if negative).
 *
 * m2 must be normalized to 2^52 <= m2 < 2^53 (subnormal doubles and
 * floats are shifted up to match), and e2 must be in the range
 * [-1126, 971]. With 4 * m2 < 2^55, the 125-bit tables yield an exact
 * value for floor(m2 * 2^e2 / 10^e10) across this range; this was
 * verified using the Ryu analysis (ComputeRequiredBitSizes), which
 * requires at most 124 bits for any exponent here.
 *
 * Unlike the original Ryu code, which generates the shortest string
 * that round-trips, this generates exactly the requested number of
 * digits. Ryu's choice of e10 leaves vr with at least 18 digits, so
 * up to 17 can always be generated exactly.
 */
floating_decimal_64
__ryu_d2d(uint64_t m2, int32_t e2, int max_digits, bool fmode, int max_decimals)
{
    // Ryu works with 4 * m2 so that the bounds can be computed too; keep
    // that scaling so that the precision analysis still applies.
    const uint64_t mv = 4 * m2;
    uint64_t       vr;
    uint64_t       pow5[2];
    int32_t        e10;
    bool           vrIsTrailingZeros;

    e2 -= 2;
    if (e2 >= 0) {
        // This expression is slightly faster than max_int(0, log10Pow2(e2) - 1).
        const uint32_t q = log10Pow2(e2) - (e2 > 3);
        e10 = (int32_t)q;
        const int32_t k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((int32_t)q) - 1;
        const int32_t i = -e2 + (int32_t)q + k;
        __double_computeInvPow5(q, pow5);
        vr = mulShift64(mv, pow5, i);
        // vr = floor(mv * 2^(e2 - q) / 5^q), which is exact iff 5^q divides mv
        vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
    } else {
        // This expression is slightly faster than max_int(0, log10Pow5(-e2) - 1).
        const uint32_t q = log10Pow5(-e2) - (-e2 > 1);
        e10 = (int32_t)q + e2;
        const int32_t i = -e2 - (int32_t)q;
        const int32_t k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
        const int32_t j = (int32_t)q - k;
        __double_computePow5(i, pow5);
        vr = mulShift64(mv, pow5, j);
        // vr = floor(mv * 5^i / 2^q), which is exact iff 2^q divides mv
        vrIsTrailingZeros = q < 64 && multipleOfPowerOf2(mv, q);
    }

    int len = decimalLength(vr);

    /*
     * In fmode, limit the number of digits to those left of the
     * decimal point plus max_decimals. exp is the exponent of the
     * first digit: if negative, there are -exp - 1 zeros past the
     * decimal point before the first digit, otherwise there are exp + 1
     * digits left of the decimal point.
     *
     * When called from fcvt, max_decimals may be less than zero,
     * indicating that we want to round left of the decimal point. In
     * that case, make sure we generate at least one digit.
     */
    int save_max_digits = max_digits;
    if (fmode) {
        int exp = e10 + len - 1;
        int ndigits = max_decimals + exp + 1;

        if (ndigits < 0 && max_decimals >= 0) {
            // Value is less than half of the last decimal place
            floating_decimal_64 zero = { 0, (int16_t)-max_decimals, 0 };
            return zero;
        }
        max_digits = min_int(max_digits, max_int(max_decimals < 0, ndigits));
    }

    // Remove digits, remembering the last one and whether the rest were zero
    uint8_t lastRemovedDigit = 0;
    while (len > max_digits) {
        const uint64_t vrDiv10 = div10(vr);
        const uint32_t vrMod10 = ((uint32_t)vr) - 10 * ((uint32_t)vrDiv10);
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t)vrMod10;
        vr = vrDiv10;
        e10++;
        len--;
    }

    if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
        // Round even if the exact number is .....50..0.
        lastRemovedDigit = 4;
    }

    if (lastRemovedDigit >= 5) {
        vr++;
        if (len == 0 || decimalLength(vr) > len) {
            // Carried out of the top digit, giving a power of ten. In
            // fmode, the extra digit is left of the decimal point, so
            // there's room for it unless that would exceed the buffer.
            if (fmode ? len + 1 > save_max_digits : len + 1 > max_digits) {
                vr = div10(vr);
                e10++;
            } else {
                len++;
            }
        }
    }

    floating_decimal_64 fd;
    fd.mantissa = vr;
    fd.exponent = (int16_t)e10;
    fd.olength = (int16_t)len;
    return fd;
}
