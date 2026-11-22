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

#ifndef _ATOF_EXACT_H_
#define _ATOF_EXACT_H_

#include <stdint.h>

/*
 * Exact decimal to binary conversion for strtod and scanf.
 *
 * C23 F.5 (and IEEE 754-2019 5.12.2) require decimal strings with up
 * to CR_DECIMAL_DIG significant digits to be correctly rounded, where
 * CR_DECIMAL_DIG is at least M + 3 and M is the largest of the
 * *_DECIMAL_DIG values. Longer strings are first rounded to
 * CR_DECIMAL_DIG digits in the current rounding direction, then that
 * value is correctly rounded. The same limit applies to every type,
 * so even strtof must handle ATOF_EXACT_DIG digits.
 */

#ifdef __LDBL_DECIMAL_DIG__
#define ATOF_EXACT_DIG (__LDBL_DECIMAL_DIG__ + 3)
#else
#define ATOF_EXACT_DIG (__DECIMAL_DIG__ + 3)
#endif

/* Upper bound on the bits in a number of ATOF_EXACT_DIG digits, plus one for rounding */
#define ATOF_EXACT_BITS (ATOF_EXACT_DIG * 3322 / 1000 + 2)

/* 32-bit words needed to hold such a number */
#define ATOF_EXACT_NWORDS ((ATOF_EXACT_BITS + 31) / 32)

/* Words in the converted significand: up to 113 + 3 bits */
#define ATOF_EXACT_QWORDS 4

/*
 * Multiply the little-endian bignum a of n words by m and add c,
 * returning the new length. The caller makes sure there is room for
 * one more word.
 */
static inline int
__atof_exact_mul_add(uint32_t *a, int n, uint32_t m, uint32_t c)
{
    uint64_t carry = c;
    int      i;

    for (i = 0; i < n; i++) {
        carry += (uint64_t)a[i] * m;
        a[i] = (uint32_t)carry;
        carry >>= 32;
    }
    if (carry)
        a[n++] = (uint32_t)carry;
    return n;
}

/*
 * Convert n[0..nn) * 10**e10 into q * 2**ret, where q has at least
 * mant_dig + 2 significant bits. *sticky is set when bits below q
 * were discarded. Values too large or too small for the type return
 * a q/ret pair that overflows or underflows it with *sticky set.
 */
int __atof_exact_float(const uint32_t *n, int nn, int e10, uint32_t *q, int *sticky);
int __atof_exact_double(const uint32_t *n, int nn, int e10, uint32_t *q, int *sticky);
int __atof_exact_long_double(const uint32_t *n, int nn, int e10, uint32_t *q, int *sticky);

#endif /* _ATOF_EXACT_H_ */
