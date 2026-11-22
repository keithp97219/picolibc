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

/*
 * Exact decimal to binary conversion using simple bignums.
 *
 * For e10 >= 0, n * 5**e10 is computed exactly and the top bits
 * extracted. For e10 < 0, n is divided by 5**-e10 one quotient bit
 * at a time. Either way the result is the exact leading bits plus a
 * sticky bit, which the caller rounds in the current rounding
 * direction. That is slow for large exponents but small, and correct
 * for every input.
 */

#include "local-stdio.h"
#include "atof_exact.h"

static int
big_len(const uint32_t *a, int n)
{
    while (n && a[n - 1] == 0)
        n--;
    return n;
}

static int
big_bitlen(const uint32_t *a, int n)
{
    return n ? n * 32 - __builtin_clz(a[n - 1]) : 0;
}

static int
big_mul_pow5(uint32_t *a, int n, int e)
{
    while (e > 0) {
        uint32_t m = 1;
        int      k;
        for (k = 0; k < 13 && k < e; k++)
            m *= 5;
        n = __atof_exact_mul_add(a, n, m, 0);
        e -= k;
    }
    return n;
}

/* a = b << s; a may be b. a needs room for n + s/32 + 1 words */
static int
big_shl(uint32_t *a, const uint32_t *b, int n, int s)
{
    int w = s >> 5, sh = s & 31, i;

    a[n + w] = 0;
    for (i = n - 1; i >= 0; i--) {
        uint32_t v = b[i];
        if (sh)
            a[i + w + 1] |= v >> (32 - sh);
        a[i + w] = v << sh;
    }
    for (i = 0; i < w; i++)
        a[i] = 0;
    return big_len(a, n + w + 1);
}

static int
big_cmp(const uint32_t *a, int na, const uint32_t *b, int nb)
{
    if (na != nb)
        return na - nb;
    while (na--)
        if (a[na] != b[na])
            return a[na] > b[na] ? 1 : -1;
    return 0;
}

/* a -= b, with a >= b */
static int
big_sub(uint32_t *a, int na, const uint32_t *b, int nb)
{
    uint32_t borrow = 0;
    int      i;

    for (i = 0; i < na; i++) {
        uint64_t d = (uint64_t)a[i] - (i < nb ? b[i] : 0) - borrow;
        a[i] = (uint32_t)d;
        borrow = (uint32_t)(d >> 32) & 1;
    }
    return big_len(a, na);
}

static void
q_shift_in(uint32_t *q, uint32_t bit)
{
    int i;

    for (i = 0; i < ATOF_EXACT_QWORDS; i++) {
        uint32_t top = q[i] >> 31;
        q[i] = (q[i] << 1) | bit;
        bit = top;
    }
}

static int
atof_exact(uint32_t *buf, int half, const uint32_t *n, int nn, int e10, int mant_dig, int min_exp,
           int max_exp, uint32_t *q, int *sticky)
{
    int qbits = mant_dig + 2;
    int ln = big_bitlen(n, nn);
    int i, nb, e2;

    for (i = 0; i < ATOF_EXACT_QWORDS; i++)
        q[i] = 0;

    /*
     * Values at least 2**max_exp overflow and values below half the
     * smallest subnormal underflow in every rounding mode; return a
     * value well beyond each limit with sticky set. Using 3.3 in place
     * of log2(10) keeps both tests conservative.
     */
    *sticky = 1;
    q[0] = 1;
    if (e10 >= 0 && ln - 1 + e10 * 33 / 10 >= max_exp)
        return max_exp + 1;
    if (e10 < 0 && ln + e10 * 33 / 10 <= min_exp - mant_dig - 1)
        return min_exp - mant_dig - 2;
    q[0] = 0;

    if (e10 >= 0) {
        int l, sh;

        nb = big_shl(buf, n, nn, 0);
        nb = big_mul_pow5(buf, nb, e10);
        l = big_bitlen(buf, nb);
        sh = l - qbits;
        if (sh < 0)
            sh = 0;
        for (i = l; i-- > sh;)
            q_shift_in(q, (buf[i >> 5] >> (i & 31)) & 1);
        *sticky = 0;
        for (i = 0; i < (sh >> 5); i++)
            if (buf[i])
                *sticky = 1;
        if ((sh & 31) && (buf[sh >> 5] & ((1UL << (sh & 31)) - 1)))
            *sticky = 1;
        e2 = e10 + sh;
    } else {
        uint32_t *y = buf, *r = buf + half;
        int       ny, nr, t;

        y[0] = 1;
        ny = big_mul_pow5(y, 1, -e10);

        /* Align the numerator and denominator */
        t = big_bitlen(y, ny) - ln;
        if (t >= 0) {
            nr = big_shl(r, n, nn, t);
        } else {
            ny = big_shl(y, y, ny, -t);
            nr = big_shl(r, n, nn, 0);
        }

        /* r < 2y, so each step generates one quotient bit */
        for (i = 0; i <= qbits; i++) {
            uint32_t bit = big_cmp(r, nr, y, ny) >= 0;
            if (bit)
                nr = big_sub(r, nr, y, ny);
            q_shift_in(q, bit);
            nr = big_shl(r, r, nr, 1);
        }
        *sticky = nr != 0;
        e2 = e10 - t - qbits;
    }
    return e2;
}

/*
 * Bound the stack buffer for each type. After the range checks
 * above, positive exponents are below (max_exp + 1) / 3.3 + 1 and
 * negative ones above -(ATOF_EXACT_BITS - min_exp + mant_dig + 2) / 3.3 - 1.
 */
#define POS_E(max_exp)               (((max_exp) + 1) * 10 / 33 + 1)
#define NEG_E(min_exp, mant)         ((ATOF_EXACT_BITS - (min_exp) + (mant) + 2) * 10 / 33 + 1)
#define P5_BITS(e)                   ((e) * 2322 / 1000 + 1)
#define WORDS_POS(max_exp)           ((ATOF_EXACT_BITS + P5_BITS(POS_E(max_exp))) / 32 + 2)
#define WORDS_NEG(min_exp, mant)     ((P5_BITS(NEG_E(min_exp, mant)) + ATOF_EXACT_BITS) / 32 + 3)
#define MAX(a, b)                    ((a) > (b) ? (a) : (b))
#define HALF(min_exp, mant, max_exp) MAX(WORDS_NEG(min_exp, mant), (WORDS_POS(max_exp) + 1) / 2)

#define ATOF_EXACT_FUNC(name, mant, min_exp, max_exp)                                            \
    int name(const uint32_t *n, int nn, int e10, uint32_t *q, int *sticky)                       \
    {                                                                                            \
        uint32_t buf[2 * HALF(min_exp, mant, max_exp)];                                          \
        return atof_exact(buf, HALF(min_exp, mant, max_exp), n, nn, e10, mant, min_exp, max_exp, \
                          q, sticky);                                                            \
    }

ATOF_EXACT_FUNC(__atof_exact_float, __FLT_MANT_DIG__, __FLT_MIN_EXP__, __FLT_MAX_EXP__)

#if __SIZEOF_DOUBLE__ == 8
ATOF_EXACT_FUNC(__atof_exact_double, __DBL_MANT_DIG__, __DBL_MIN_EXP__, __DBL_MAX_EXP__)
#elif __SIZEOF_LONG_DOUBLE__ == 8
ATOF_EXACT_FUNC(__atof_exact_double, __LDBL_MANT_DIG__, __LDBL_MIN_EXP__, __LDBL_MAX_EXP__)
#endif

#if __SIZEOF_LONG_DOUBLE__ > 8
ATOF_EXACT_FUNC(__atof_exact_long_double, __LDBL_MANT_DIG__, __LDBL_MIN_EXP__, __LDBL_MAX_EXP__)
#endif
