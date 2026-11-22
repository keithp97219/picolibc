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
