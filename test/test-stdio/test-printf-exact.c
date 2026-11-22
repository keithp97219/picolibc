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
 * Check that %.Ne, %.Ng and %.Nf produce correctly rounded results
 * (round-half-even on the exact binary value) for every precision
 * that fits within the number of digits the exact (ryu) conversion
 * code generates: 17 for double and 9 for float.
 *
 * The reference digits are computed with a small big-number
 * conversion of the exact binary value.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifndef __PICOLIBC__
#define TEST_DOUBLE
#elif defined(__IO_NO_FLOATING_POINT) || !defined(__IO_FLOAT_EXACT)
#define TEST_SKIP
#elif defined(_HAS_IO_FLOAT)
#define TEST_FLOAT
#elif defined(_HAS_IO_DOUBLE)
#define TEST_DOUBLE
#else
#define TEST_SKIP
#endif

#ifdef TEST_SKIP

int
main(void)
{
    printf("skipping, no exact float printf\n");
    return 77;
}

#else

#if defined(TEST_DOUBLE) && __SIZEOF_DOUBLE__ == 8
typedef double   test_t;
typedef uint64_t test_bits_t;
#define MANT_BITS   52
#define EXP_BITS    11
#define MAX_DIG     17
#define TEST_ARG(x) (x)
#else
#ifdef TEST_DOUBLE
typedef double test_t;
#define TEST_ARG(x) (x)
#else
typedef float test_t;
#define TEST_ARG(x) printf_float(x)
#endif
typedef uint32_t test_bits_t;
#define MANT_BITS 23
#define EXP_BITS  8
#define MAX_DIG   9
#endif

#define EXP_BIAS ((1 << (EXP_BITS - 1)) - 1)

#ifndef NUM_RANDOM
#define NUM_RANDOM 2000
#endif

/* Large enough for m * 5^1074 */
#define BIG_LIMBS 100
#define LIMB_BASE 1000000000UL

struct big {
    int      n;
    uint32_t l[BIG_LIMBS];
};

static void
big_mul(struct big *b, uint32_t x)
{
    uint64_t carry = 0;
    int      i;

    for (i = 0; i < b->n; i++) {
        uint64_t t = (uint64_t)b->l[i] * x + carry;
        b->l[i] = (uint32_t)(t % LIMB_BASE);
        carry = t / LIMB_BASE;
    }
    while (carry) {
        b->l[b->n++] = (uint32_t)(carry % LIMB_BASE);
        carry /= LIMB_BASE;
    }
}

/* Exact decimal value: digits[0..ndig) * 10^(exp10 - ndig + 1) */
static char digits[800];
static int  ndig;
static int  exp10;

static void
exact_digits(test_bits_t mant, int e2)
{
    struct big b;
    char       tmp[10];
    int        i, j;

    b.n = 0;
    while (mant) {
        b.l[b.n++] = (uint32_t)(mant % LIMB_BASE);
        mant /= LIMB_BASE;
    }
    if (e2 >= 0) {
        for (i = e2; i > 0; i -= 16)
            big_mul(&b, (uint32_t)1 << (i > 16 ? 16 : i));
    } else {
        for (i = -e2; i > 0; i -= 13) {
            uint32_t p = 1;
            for (j = 0; j < (i > 13 ? 13 : i); j++)
                p *= 5;
            big_mul(&b, p);
        }
    }
    ndig = 0;
    for (i = b.n - 1; i >= 0; i--) {
        uint32_t v = b.l[i];
        for (j = 8; j >= 0; j--) {
            tmp[j] = (char)('0' + v % 10);
            v /= 10;
        }
        for (j = 0; j < 9; j++) {
            if (ndig == 0 && tmp[j] == '0')
                continue;
            digits[ndig++] = tmp[j];
        }
    }
    exp10 = ndig - 1 + (e2 < 0 ? e2 : 0);
}

/*
 * Round the exact value to 'n' significant digits. Returns the number
 * of digits in 'out' (0 if the value rounds to zero) and sets *exp to
 * the decimal exponent of the first digit.
 */
static int
round_digits(int n, char *out, int *exp)
{
    int  i;
    bool up;

    *exp = exp10;
    if (n < 0)
        return 0;
    if (n >= ndig) {
        memcpy(out, digits, ndig);
        memset(out + ndig, '0', n - ndig);
        return n;
    }
    if (digits[n] > '5')
        up = true;
    else if (digits[n] < '5')
        up = false;
    else {
        up = false;
        for (i = n + 1; i < ndig; i++)
            if (digits[i] != '0')
                up = true;
        if (!up)
            up = n > 0 && ((digits[n - 1] - '0') & 1);
    }
    memcpy(out, digits, n);
    if (up) {
        for (i = n - 1; i >= 0; i--) {
            if (out[i] != '9') {
                out[i]++;
                break;
            }
            out[i] = '0';
        }
        if (i < 0) {
            /* carried out of the top */
            if (n > 0)
                memmove(out + 1, out, n - 1);
            out[0] = '1';
            (*exp)++;
            if (n == 0)
                n = 1;
        }
    }
    return n;
}

static char *
fmt_exp(char *s, int exp)
{
    *s++ = 'e';
    if (exp < 0) {
        *s++ = '-';
        exp = -exp;
    } else
        *s++ = '+';
    if (exp >= 100) {
        if (exp >= 1000)
            *s++ = (char)('0' + exp / 1000);
        *s++ = (char)('0' + exp / 100 % 10);
    }
    *s++ = (char)('0' + exp / 10 % 10);
    *s++ = (char)('0' + exp % 10);
    return s;
}

static void
expect_e(char *s, int prec, bool strip)
{
    char d[MAX_DIG + 2];
    int  exp, i;

    round_digits(prec + 1, d, &exp);
    if (strip)
        while (prec > 0 && d[prec] == '0')
            prec--;
    *s++ = d[0];
    if (prec)
        *s++ = '.';
    for (i = 1; i <= prec; i++)
        *s++ = d[i];
    s = fmt_exp(s, exp);
    *s = '\0';
}

static void
format_f(char *s, const char *d, int nd, int exp, int prec)
{
    int i;

    if (exp < 0 || nd == 0)
        *s++ = '0';
    else
        for (i = 0; i <= exp; i++)
            *s++ = i < nd ? d[i] : '0';
    if (prec)
        *s++ = '.';
    for (i = 1; i <= prec; i++) {
        int idx = exp + i;
        *s++ = (nd != 0 && idx >= 0 && idx < nd) ? d[idx] : '0';
    }
    *s = '\0';
}

static void
expect_f(char *s, int prec)
{
    char d[MAX_DIG + 2];
    int  exp;
    int  n = exp10 + 1 + prec;
    int  nd;

    if (n == 0) {
        /* Value is in [0.1, 1) * 10^-prec; check against half a unit */
        bool up = digits[0] > '5';
        int  i;
        if (digits[0] == '5')
            for (i = 1; i < ndig; i++)
                if (digits[i] != '0')
                    up = true;
        nd = up;
        d[0] = '1';
        exp = -prec;
    } else
        nd = round_digits(n, d, &exp);
    format_f(s, d, nd, exp, prec);
}

static void
expect_g(char *s, int prec)
{
    char d[MAX_DIG + 2];
    int  exp, nd;

    if (prec == 0)
        prec = 1;
    nd = round_digits(prec, d, &exp);
    if (-4 <= exp && exp < prec) {
        int fprec = prec - 1 - exp;
        while (nd > 1 && d[nd - 1] == '0' && fprec > 0) {
            nd--;
            fprec--;
        }
        format_f(s, d, nd, exp, fprec);
    } else
        expect_e(s, prec - 1, true);
}

static int  failures;
static long checks;

static void
check(test_bits_t bits, test_t v, const char *fmt, int prec, const char *expect)
{
    char got[400];

    snprintf(got, sizeof(got), fmt, prec, TEST_ARG(v));
    checks++;
    if (strcmp(got, expect) != 0) {
        if (failures < 50)
            printf("%#lx%08lx %s prec %d: got %s expect %s\n",
                   (unsigned long)((uint64_t)bits >> 16 >> 16), (unsigned long)(bits & 0xffffffff),
                   fmt, prec, got, expect);
        failures++;
    }
}

static void
test_bits(test_bits_t bits)
{
    test_t      v;
    test_bits_t mant;
    int         e2, biased, prec;
    char        expect[400];

    bits &= ~((test_bits_t)1 << (MANT_BITS + EXP_BITS));
    biased = (int)(bits >> MANT_BITS);
    if (biased == (1 << EXP_BITS) - 1)
        return;
    mant = bits & (((test_bits_t)1 << MANT_BITS) - 1);
    if (biased == 0) {
        if (mant == 0)
            return;
        e2 = 1 - EXP_BIAS - MANT_BITS;
    } else {
        mant |= (test_bits_t)1 << MANT_BITS;
        e2 = biased - EXP_BIAS - MANT_BITS;
    }
    memcpy(&v, &bits, sizeof(v));
    exact_digits(mant, e2);

    for (prec = 0; prec < MAX_DIG; prec++) {
        expect_e(expect, prec, false);
        check(bits, v, "%.*e", prec, expect);
    }
    for (prec = 0; prec <= MAX_DIG; prec++) {
        expect_g(expect, prec);
        check(bits, v, "%.*g", prec, expect);
    }
    for (prec = 0; prec <= MAX_DIG + 1; prec++) {
        /* Only check when the result needs no more than MAX_DIG digits */
        if (exp10 + 1 + prec > MAX_DIG)
            break;
        expect_f(expect, prec);
        check(bits, v, "%.*f", prec, expect);
    }
}

static uint64_t rand_state = 0x0123456789abcdefULL;

static uint64_t
next_rand(void)
{
    /* xorshift64 */
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 7;
    rand_state ^= rand_state << 17;
    return rand_state;
}

static test_bits_t
to_bits(test_t v)
{
    test_bits_t bits;
    memcpy(&bits, &v, sizeof(bits));
    return bits;
}

int
main(void)
{
    int    i, j;
    test_t v;

    /* Random bit patterns, covering every exponent */
    for (i = 0; i < NUM_RANDOM; i++)
        test_bits((test_bits_t)next_rand());

    /* Values near powers of ten */
    v = 1;
    for (i = 0; i <= MAX_DIG + 4; i++) {
        test_bits_t b = to_bits(v);
        test_bits(b - 1);
        test_bits(b);
        test_bits(b + 1);
        v *= 10;
    }
    v = 1;
    for (i = 0; i <= MAX_DIG + 4; i++) {
        v /= 10;
        test_bits(to_bits(v) - 1);
        test_bits(to_bits(v));
        test_bits(to_bits(v) + 1);
    }

    /* Subnormal values, including those with few significant bits */
    for (i = 0; i < NUM_RANDOM / 8; i++) {
        test_bits_t mant = (test_bits_t)next_rand() & (((test_bits_t)1 << MANT_BITS) - 1);
        test_bits(mant);
        test_bits(mant >> (next_rand() % MANT_BITS));
    }

    /* Values with short exact decimal expansions ending in 5 (ties) */
    for (i = 0; i < NUM_RANDOM / 4; i++) {
        uint32_t m = (uint32_t)next_rand() & 0xfffff;
        int      shift = (int)(next_rand() % 30) + 1;
        v = (test_t)m;
        for (j = 0; j < shift; j++)
            v /= 2;
        test_bits(to_bits(v));
    }

    /* Smallest and largest values */
    test_bits(1);
    test_bits(2);
    test_bits(3);
    test_bits(((test_bits_t)1 << MANT_BITS) - 1);
    test_bits((test_bits_t)1 << MANT_BITS);
    test_bits((((test_bits_t)1 << (MANT_BITS + EXP_BITS)) - 1) - ((test_bits_t)1 << MANT_BITS));

    printf("%ld checks, %d failures\n", checks, failures);
    return failures != 0;
}

#endif
