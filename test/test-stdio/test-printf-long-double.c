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
 * Check decimal conversion of 80-bit and binary128 long double
 * values: %Le and %Lf output (or ecvtl/fcvtl when printf lacks long
 * double support) against exactly rounded values, and a round trip
 * through strtold.
 *
 * picolibc rounds exactly up to 21 (80-bit) or 36 (binary128) digits,
 * enough to read back as the same value, and appends zeros past that.
 * The C library used for the -native build rounds exactly at every
 * precision.
 *
 * The expected digits come from test-printf-long-double.h for a fixed
 * set of values and from a small big-number conversion for random
 * values.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>

#if defined(__PICOLIBC__) && !defined(__IO_FLOAT_EXACT)
#define SKIP "inexact conversions"
#elif __SIZEOF_LONG_DOUBLE__ <= 8 || (__LDBL_MANT_DIG__ != 64 && __LDBL_MANT_DIG__ != 113)
#define SKIP "long double format"
#endif

#ifdef SKIP

int
main(void)
{
    printf("skipping: %s\n", SKIP);
    return 77;
}

#else

#ifdef __PICOLIBC__
#ifdef _HAS_IO_LONG_DOUBLE
#define USE_PRINTF
#endif
#define EXACT_ALL false
#else
#define USE_PRINTF
#define EXACT_ALL true
#endif

/* Digits generated exactly; enough for any value to round-trip */
#if __LDBL_MANT_DIG__ == 64
#define MAX_DIG 21
#else
#define MAX_DIG 36
#endif

struct ld_vector {
    long double value;
    const char *digits; /* leading digits of the exact value */
    int         exp;    /* decimal exponent of digits[0] */
    bool        sticky; /* any non-zero digits after 'digits' */
};

#define LDV(value, digits, exp, sticky) { value, digits, exp, sticky }

#include "test-printf-long-double.h"

#define NVECTORS (sizeof(ld_vectors) / sizeof(ld_vectors[0]))

/*
 * Round the digit string 'digits' (with 'sticky' set if digits beyond
 * it are non-zero) to 'ndig' digits, half to even. Returns the
 * exponent adjustment (1 when rounding carries into a new digit).
 */
static int
round_digits(const char *digits, bool sticky, int ndig, char *out)
{
    int len = (int)strlen(digits);
    int i;

    if (ndig < 0) {
        out[0] = '\0';
        return 0;
    }
    if (ndig >= len) {
        memcpy(out, digits, len);
        memset(out + len, '0', ndig - len);
        out[ndig] = '\0';
        return 0;
    }
    memcpy(out, digits, ndig);
    out[ndig] = '\0';

    bool up;
    char next = digits[ndig];
    if (next > '5')
        up = true;
    else if (next < '5')
        up = false;
    else {
        bool rest = sticky;
        for (i = ndig + 1; i < len; i++)
            if (digits[i] != '0')
                rest = true;
        if (rest)
            up = true;
        else
            up = ndig > 0 && ((out[ndig - 1] - '0') & 1);
    }
    if (!up)
        return 0;
    for (i = ndig - 1; i >= 0; i--) {
        if (out[i] != '9') {
            out[i]++;
            return 0;
        }
        out[i] = '0';
    }
    /* carried out of the top: 1 followed by zeros, one more digit */
    out[0] = '1';
    if (ndig > 0)
        out[ndig] = '0';
    out[ndig + 1] = '\0';
    return 1;
}

static void
format_e(char *buf, bool neg, const char *digits, int ndig, int exp)
{
    char *o = buf;
    int   i;

    if (neg)
        *o++ = '-';
    *o++ = digits[0];
    if (ndig > 1)
        *o++ = '.';
    for (i = 1; i < ndig; i++)
        *o++ = digits[i];
    sprintf(o, "e%c%02d", exp < 0 ? '-' : '+', exp < 0 ? -exp : exp);
}

/* digits represent 0.digits * 10^decpt */
static void
format_f(char *buf, bool neg, const char *digits, int decpt, int prec)
{
    char *o = buf;
    int   len = (int)strlen(digits);
    int   pos;

    if (neg)
        *o++ = '-';
    if (decpt <= 0)
        *o++ = '0';
    for (pos = 0; pos < decpt; pos++)
        *o++ = pos < len ? digits[pos] : '0';
    if (prec > 0)
        *o++ = '.';
    for (pos = decpt; pos < decpt + prec; pos++)
        *o++ = (pos >= 0 && pos < len) ? digits[pos] : '0';
    *o = '\0';
}

/*
 * Round to ndig digits, but no more than MAX_DIG in picolibc, which
 * pads with zeros past that. Returns the exponent adjustment.
 */
static int
expect_digits(const struct ld_vector *v, int ndig, char *out)
{
    int adj;

    if (ndig > MAX_DIG && !EXACT_ALL) {
        adj = round_digits(v->digits, v->sticky, MAX_DIG, out);
        memset(out + MAX_DIG + adj, '0', ndig - MAX_DIG);
        out[ndig + adj] = '\0';
        return adj;
    }
    return round_digits(v->digits, v->sticky, ndig, out);
}

/* Expected %.*Le output, prec + 1 digits */
static void
expect_e(const struct ld_vector *v, bool neg, int prec, char *buf)
{
    char out[128];
    int  ndig = prec + 1;
    int  exp = v->exp + expect_digits(v, ndig, out);

    format_e(buf, neg, out, ndig, exp);
}

/* Expected %.*Lf output */
static void
expect_f(const struct ld_vector *v, bool neg, int prec, char *buf)
{
    char out[128];
    int  ndig = v->exp + 1 + prec;
    int  decpt;

    if (ndig < 0) {
        /* less than a tenth of the last place */
        out[0] = '\0';
        decpt = 0;
    } else {
        decpt = v->exp + 1 + expect_digits(v, ndig, out);
    }
    format_f(buf, neg, out, decpt, prec);
}

static void
convert_e(long double x, int prec, char *buf, size_t size)
{
#ifdef USE_PRINTF
    snprintf(buf, size, "%.*Le", prec, x);
#else
    char digits[64];
    int  decpt, sign;
    (void)size;
    ecvtl_r(x, prec + 1, &decpt, &sign, digits, sizeof(digits));
    format_e(buf, sign, digits, prec + 1, decpt - 1);
#endif
}

static void
convert_f(long double x, int prec, char *buf, size_t size)
{
#ifdef USE_PRINTF
    snprintf(buf, size, "%.*Lf", prec, x);
#else
    char digits[128];
    int  decpt, sign;
    (void)size;
    fcvtl_r(x, prec, &decpt, &sign, digits, sizeof(digits));
    format_f(buf, sign, digits, decpt, prec);
#endif
}

/* Check %Le and %Lf output for v and -v */
static int
check_value(const struct ld_vector *v, const char *name)
{
    int  prec;
    int  errors = 0;
    int  neg;
    int  len = (int)strlen(v->digits);
    char expect[128], got[128];

    for (neg = 0; neg < 2; neg++) {
        long double x = neg ? -v->value : v->value;
        for (prec = 0; prec < MAX_DIG + 3; prec++) {
            /* need one more exact digit to round */
            if (prec + 1 >= len)
                break;
            expect_e(v, neg, prec, expect);
            convert_e(x, prec, got, sizeof(got));
            if (strcmp(expect, got) != 0) {
                printf("%s %%.%dLe: expect %s got %s\n", name, prec, expect, got);
                errors++;
            }
        }
        if (-25 <= v->exp && v->exp <= 25) {
            for (prec = 0; prec < 40; prec++) {
                int ndig = v->exp + 1 + prec;
                if (ndig >= len && (EXACT_ALL || ndig <= MAX_DIG))
                    break;
                expect_f(v, neg, prec, expect);
                convert_f(x, prec, got, sizeof(got));
                if (strcmp(expect, got) != 0) {
                    printf("%s %%.%dLf: expect %s got %s\n", name, prec, expect, got);
                    errors++;
                }
            }
        }
    }
    return errors;
}

static int
check_vectors(void)
{
    unsigned i;
    int      errors = 0;
    char     name[32];

    for (i = 0; i < NVECTORS; i++) {
        snprintf(name, sizeof(name), "vector %u", i);
        errors += check_value(&ld_vectors[i], name);
    }
    return errors;
}

/* Specific results from bug reports and the old conversion code */
static int
check_strings(void)
{
    int errors = 0;
#ifdef USE_PRINTF
    static const struct {
        const char *fmt;
        long double value;
        const char *expect;
    } tests[] = {
        { "%Lf",    1234321234321234.0L,     "1234321234321234.000000"         },
#if __LDBL_MANT_DIG__ == 64
        { "%.2Lf",  999.995L,                "999.99"                          }, /* 999.99499999999999999955... */
#else
        { "%.2Lf", 999.995L, "1000.00" }, /* 999.99500000000000000000000000000000394... */
#endif
        { "%.0Lf",  0.5L,                    "0"                               },
        { "%.0Lf",  1.5L,                    "2"                               },
        { "%.0Lf",  2.5L,                    "2"                               },
        { "%.1Lf",  0.25L,                   "0.2"                             },
        { "%.1Lf",  0.35L,                   "0.3"                             },
        { "%.3Lf",  0.0004L,                 "0.000"                           },
        { "%.3Lf",  0.0006L,                 "0.001"                           },
        { "%.3Lf",  9.9996L,                 "10.000"                          },
        { "%.3Le",  9.9996L,                 "1.000e+01"                       },
        { "%Le",    0.0L,                    "0.000000e+00"                    },
        { "%Le",    -0.0L,                   "-0.000000e+00"                   },
        { "%Lf",    (long double)INFINITY,   "inf"                             },
        { "%Lf",    -(long double)INFINITY,  "-inf"                            },
        { "%Lf",    (long double)NAN,        "nan"                             },
        { "%Lg",    100000.0L,               "100000"                          },
        { "%Lg",    1000000.0L,              "1e+06"                           },
        { "%Lg",    0.0001L,                 "0.0001"                          },
#if __LDBL_MANT_DIG__ == 64
        { "%.18Le", 18446744073709551615.0L, "1.844674407370955162e+19"        },
        { "%.19Le", 18446744073709551615.0L, "1.8446744073709551615e+19"       },
#endif
#if __LDBL_MANT_DIG__ == 64 && defined(__PICOLIBC__)
        /* zeros past 21 digits */
        { "%.25Le", 0.1L,                    "1.0000000000000000000100000e-01" },
#elif __LDBL_MANT_DIG__ == 64
        { "%.25Le", 0.1L, "1.0000000000000000000135525e-01" },
#endif
    };
    unsigned i;
    char     buf[128];

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        snprintf(buf, sizeof(buf), tests[i].fmt, tests[i].value);
        if (strcmp(buf, tests[i].expect) != 0) {
            printf("\"%s\": expect %s got %s\n", tests[i].fmt, tests[i].expect, buf);
            errors++;
        }
    }
#endif
    return errors;
}

static uint64_t rand_state = 0x123456789abcdefULL;

static uint64_t
rand64(void)
{
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 7;
    rand_state ^= rand_state << 17;
    return rand_state;
}

/* A random finite value, often near 1 */
static long double
rand_ld(void)
{
    uint64_t    hi = rand64(), lo = rand64();
    int         exp;
    long double x;

#if __LDBL_MANT_DIG__ == 64
    x = ldexpl((long double)(lo | (1ULL << 63)), -63);
#else
    x = ldexpl((long double)(hi >> 15 | (1ULL << 48)), -48) + ldexpl((long double)lo, -112);
#endif
    if (hi & 1)
        exp = (int)(hi % (LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG)) + LDBL_MIN_EXP
            - LDBL_MANT_DIG;
    else
        exp = (int)((hi >> 1) % 256) - 128;
    x = ldexpl(x, exp);
    if (hi & 2)
        x = -x;
    return x;
}

/*
 * Exact decimal digits of a long double, using base 10^9 limbs. A
 * value m * 2^e2 is m * 2^e2 for e2 >= 0 and m * 5^-e2 * 10^e2
 * otherwise. 5^16494 * 2^113 has fewer than 11600 digits.
 */
#define BIG_LIMBS 1300
#define LIMB_BASE 1000000000UL

static uint32_t big[BIG_LIMBS];
static int      nbig;

static void
big_mul_add(uint32_t mul, uint32_t add)
{
    uint64_t carry = add;
    int      i;

    for (i = 0; i < nbig; i++) {
        uint64_t t = (uint64_t)big[i] * mul + carry;
        big[i] = (uint32_t)(t % LIMB_BASE);
        carry = t / LIMB_BASE;
    }
    while (carry) {
        big[nbig++] = (uint32_t)(carry % LIMB_BASE);
        carry /= LIMB_BASE;
    }
}

static void
exact_value(long double x, struct ld_vector *v, char *digits, size_t size)
{
    int         e, e2, i, j, n, total;
    long double m;
    uint64_t    hi, lo;
    char        tmp[9];

    frexpl(x, &e);
    e2 = e - LDBL_MANT_DIG;
    if (e2 < LDBL_MIN_EXP - LDBL_MANT_DIG)
        e2 = LDBL_MIN_EXP - LDBL_MANT_DIG;
    m = ldexpl(x, -e2);
    hi = (uint64_t)ldexpl(m, -64);
    lo = (uint64_t)(m - ldexpl((long double)hi, 64));

    nbig = 0;
    for (i = 7; i >= 0; i--)
        big_mul_add(1 << 16, (uint32_t)((i >= 4 ? hi >> (16 * (i - 4)) : lo >> (16 * i)) & 0xffff));
    if (e2 >= 0) {
        for (i = e2; i > 0; i -= 16)
            big_mul_add((uint32_t)1 << (i > 16 ? 16 : i), 0);
    } else {
        for (i = -e2; i > 0; i -= 13) {
            uint32_t p = 1;
            for (j = 0; j < (i > 13 ? 13 : i); j++)
                p *= 5;
            big_mul_add(p, 0);
        }
    }

    /* Leading digits, and whether any of the rest are non-zero */
    n = 0;
    total = 0;
    v->sticky = false;
    for (i = nbig - 1; i >= 0; i--) {
        uint32_t l = big[i];
        for (j = 8; j >= 0; j--) {
            tmp[j] = (char)('0' + l % 10);
            l /= 10;
        }
        for (j = 0; j < 9; j++) {
            if (total == 0 && tmp[j] == '0')
                continue;
            total++;
            if (n < (int)size - 1)
                digits[n++] = tmp[j];
            else if (tmp[j] != '0')
                v->sticky = true;
        }
    }
    digits[n] = '\0';
    v->value = x;
    v->digits = digits;
    v->exp = total - 1 + (e2 < 0 ? e2 : 0);
}

/* Random values, mostly of moderate size */
static int
check_random_exact(void)
{
    int              errors = 0;
    unsigned         i;
    char             digits[48];
    char             name[32];
    struct ld_vector v;

    for (i = 0; i < 400 && errors < 20; i++) {
        uint64_t    r = rand64();
        long double x;
#if __LDBL_MANT_DIG__ == 64
        x = ldexpl((long double)(rand64() | (1ULL << 63)), -63);
#else
        x = ldexpl((long double)(r >> 15 | (1ULL << 48)), -48)
            + ldexpl((long double)rand64(), -112);
#endif
        if (i < 16)
            x = ldexpl(x,
                       (int)(r % (LDBL_MAX_EXP - LDBL_MIN_EXP + LDBL_MANT_DIG)) + LDBL_MIN_EXP
                           - LDBL_MANT_DIG);
        else if (i < 48)
            /* subnormal, with few significant bits */
            x = ldexpl(x, LDBL_MIN_EXP - 2 - (int)(r % LDBL_MANT_DIG));
        else
            x = ldexpl(x, (int)(r % 600) - 300);
        if (x == 0)
            continue;
        exact_value(x, &v, digits, sizeof(digits));
        snprintf(name, sizeof(name), "random %u", i);
        errors += check_value(&v, name);
    }
    return errors;
}

/*
 * picolibc's strtold isn't exact for long double (__atold_engine
 * scales by rounded powers of ten), so allow some error there. The
 * full-precision strings of a value and its neighbors must still be
 * distinct and ordered.
 */
#if defined(__PICOLIBC__) && __LDBL_MANT_DIG__ == 64
#define STRTOLD_ULPS 4
#elif defined(__PICOLIBC__)
#define STRTOLD_ULPS 64
#else
#define STRTOLD_ULPS 0
#endif

/* Compare two %e strings numerically */
static int
cmp_e(const char *a, const char *b)
{
    bool        na = *a == '-', nb = *b == '-';
    const char *ea = strchr(a, 'e'), *eb = strchr(b, 'e');
    int         xa = atoi(ea + 1), xb = atoi(eb + 1);
    int         c;

    if (na != nb)
        return na ? -1 : 1;
    if (xa != xb)
        c = xa < xb ? -1 : 1;
    else
        c = strcmp(a, b);
    return na ? -c : c;
}

static int
check_round_trip(long double x)
{
    char        buf[3][64];
    long double v[3]
        = { nextafterl(x, -(long double)INFINITY), x, nextafterl(x, (long double)INFINITY) };
    int j;
    int errors = 0;

    for (j = 0; j < 3; j++)
        convert_e(v[j], MAX_DIG - 1, buf[j], sizeof(buf[j]));
    for (j = 0; j < 2; j++) {
        if (isinf(v[j]) || isinf(v[j + 1]) || v[j] == 0 || v[j + 1] == 0)
            continue;
        if (cmp_e(buf[j], buf[j + 1]) >= 0) {
            printf("neighbors out of order: %s %s\n", buf[j], buf[j + 1]);
            errors++;
        }
    }

    long double y = strtold(buf[1], NULL);
    long double lo = x, hi = x;
    for (j = 0; j < STRTOLD_ULPS; j++) {
        lo = nextafterl(lo, -(long double)INFINITY);
        hi = nextafterl(hi, (long double)INFINITY);
    }
    if (!(lo <= y && y <= hi)) {
        convert_e(y, MAX_DIG - 1, buf[0], sizeof(buf[0]));
        printf("round trip %s -> %s\n", buf[1], buf[0]);
        errors++;
    }
    return errors;
}

static int
check_random(void)
{
    int      errors = 0;
    unsigned i;

    for (i = 0; i < NVECTORS; i++)
        errors += check_round_trip(ld_vectors[i].value);
    for (i = 0; i < 2000 && errors < 20; i++)
        errors += check_round_trip(rand_ld());
    return errors;
}

int
main(void)
{
    int errors = 0;

#ifdef USE_PRINTF
    printf("checking printf\n");
#else
    printf("checking ecvtl_r and fcvtl_r\n");
#endif
    errors += check_vectors();
    errors += check_strings();
    errors += check_random_exact();
    errors += check_random();
    printf("%d errors\n", errors);
    return errors != 0;
}

#endif
