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

#define _NEED_IO_FLOAT32

#include "dtoa.h"
#include "ryu/ryu.h"

#include "ryu/common.h"

#define FLOAT_MANTISSA_BITS  23
#define FLOAT_EXPONENT_BITS  8
#define FLOAT_BIAS           127
#define DOUBLE_MANTISSA_BITS 52

int
__ftoa_engine(uint32_t bits, struct dtoa *dtoa, int max_digits, bool fmode, int max_decimals)
{
    // Decode bits into sign, mantissa, and exponent.
    const bool     ieeeSign = ((bits >> (FLOAT_MANTISSA_BITS + FLOAT_EXPONENT_BITS)) & 1) != 0;
    const uint32_t ieeeMantissa = bits & (((uint32_t)1 << FLOAT_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent
        = (uint32_t)((bits >> FLOAT_MANTISSA_BITS) & (((uint32_t)1 << FLOAT_EXPONENT_BITS) - 1));

    uint8_t flags = 0;

    if (ieeeSign)
        flags |= DTOA_MINUS;

    if (ieeeExponent == 0 && ieeeMantissa == 0) {
        flags |= DTOA_ZERO;
        dtoa->digits[0] = '0';
        dtoa->flags = flags;
        dtoa->exp = 0;
        return 1;
    }
    if (ieeeExponent == (((uint32_t)1 << FLOAT_EXPONENT_BITS) - 1)) {
        if (ieeeMantissa) {
            flags |= DTOA_NAN;
        } else {
            flags |= DTOA_INF;
        }
        dtoa->flags = flags;
        return 0;
    }

    /*
     * Convert to the double format used by __ryu_d2d. The 32-bit
     * float version of Ryu doesn't generate enough digits to
     * produce exactly rounded results, so use the 64-bit version.
     */
    int32_t  e2;
    uint64_t m2;
    if (ieeeExponent == 0) {
        e2 = 1 - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
        m2 = ieeeMantissa;
    } else {
        e2 = (int32_t)ieeeExponent - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
        m2 = ((uint32_t)1 << FLOAT_MANTISSA_BITS) | ieeeMantissa;
    }
    while (!(m2 & (1ull << DOUBLE_MANTISSA_BITS))) {
        m2 <<= 1;
        e2--;
    }

    // Don't generate more digits than the conversion is defined for
    if (max_digits > FTOA_MAX_DIG)
        max_digits = FTOA_MAX_DIG;

    floating_decimal_64 v = __ryu_d2d(m2, e2, max_digits, fmode, max_decimals);

    uint32_t            mant = (uint32_t)v.mantissa; // no more than FTOA_MAX_DIG digits
    int32_t             olength = v.olength;
    int32_t             exp = v.exponent + olength - 1;

    int                 i;

    for (i = 0; i < olength; i++) {
        dtoa->digits[olength - i - 1] = (mant % 10) + '0';
        mant /= 10;
    }

    dtoa->exp = exp;
    dtoa->flags = flags;
    return olength;
}
