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
//
// -DRYU_ONLY_64_BIT_OPS Avoid using uint128_t or 64-bit intrinsics. Slower,
//     depending on your compiler.
//

#include "local-stdio.h"

#ifdef FLOAT64

#define _NEED_IO_FLOAT64

#include "dtoa.h"

#include "ryu/ryu.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef RYU_DEBUG
#include <inttypes.h>
#include <stdio.h>
#endif

#include "ryu/common.h"
#include "ryu/d2s_intrinsics.h"

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS          1023

int
__dtoa_engine(uint64_t bits, struct dtoa *dtoa, int max_digits, bool fmode, int max_decimals)
{
    // Step 1: Decode the floating-point number, and unify normalized and subnormal cases.

#ifdef RYU_DEBUG
    printf("IN=");
    for (int32_t bit = 63; bit >= 0; --bit) {
        printf("%d", (int)((bits >> bit) & 1));
    }
    printf("\n");
#endif

    // Decode bits into sign, mantissa, and exponent.
    const bool     ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent
        = (uint32_t)((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

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
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u)) {
        if (ieeeMantissa) {
            flags |= DTOA_NAN;
        } else {
            flags |= DTOA_INF;
        }
        dtoa->flags = flags;
        return 0;
    }

    int32_t  e2;
    uint64_t m2;
    if (ieeeExponent == 0) {
        // Normalize subnormals so that there are enough significant bits
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
        while (!(m2 & (1ull << DOUBLE_MANTISSA_BITS))) {
            m2 <<= 1;
            e2--;
        }
    } else {
        e2 = (int32_t)ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    // Don't generate more digits than the conversion is defined for
    if (max_digits > DTOA_MAX_DIG)
        max_digits = DTOA_MAX_DIG;

    floating_decimal_64 v = __ryu_d2d(m2, e2, max_digits, fmode, max_decimals);

    uint64_t            mant = v.mantissa;
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

#endif /* FLOAT64 */
