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
 * Check that strtof, strtod and strtold round decimal input correctly,
 * as C23 F.5 and IEEE 754 5.12.2 require: exactly for up to
 * CR_DIG significant digits, and for longer strings by first rounding
 * to CR_DIG digits in the current rounding direction.
 *
 * Expected values come from an exact decimal expansion of the values
 * and midpoints around a random binary number, computed here with a
 * small bignum.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <fenv.h>

#if defined(__PICOLIBC__)
#ifdef __LDBL_DECIMAL_DIG__
#define CR_DIG (__LDBL_DECIMAL_DIG__ + 3)
#else
#define CR_DIG (__DECIMAL_DIG__ + 3)
#endif
#else
/* Assume other C libraries round exactly for any number of digits */
#define CR_DIG 12499
#endif

#ifdef __PICOLIBC__
#include <picolibc.h>
#if !defined(__IO_FLOAT_EXACT)
#define SKIP_TESTS
#endif
#endif

#if __LDBL_MANT_DIG__ > __DBL_MANT_DIG__ && !defined(__PICOLIBC__) || defined(_TEST_LONG_DOUBLE)
#define CHECK_LONG_DOUBLE
#endif

#ifndef NRANDOM
#define NRANDOM 100
#endif

static int  errors;
static long checks;

/* Little-endian bignum big enough for the exact long double minimum */
#define BIG_WORDS 1300

struct big {
    int      n;
    uint32_t w[BIG_WORDS];
};

static void
big_mul_add(struct big *b, uint32_t m, uint32_t a)
{
    uint64_t c = a;
    int      i;

    for (i = 0; i < b->n; i++) {
        c += (uint64_t)b->w[i] * m;
        b->w[i] = (uint32_t)c;
        c >>= 32;
    }
    if (c)
        b->w[b->n++] = (uint32_t)c;
}

static uint32_t
big_div_small(struct big *b, uint32_t d)
{
    uint64_t r = 0;
    int      i;

    for (i = b->n; i-- > 0;) {
        r = (r << 32) | b->w[i];
        b->w[i] = (uint32_t)(r / d);
        r %= d;
    }
    while (b->n && b->w[b->n - 1] == 0)
        b->n--;
    return (uint32_t)r;
}

/* Decimal number: digits[0..len) * 10**exp, no leading or trailing zeros */
#define DEC_DIGITS 12500

struct dec {
    int  len;
    int  exp;
    char d[DEC_DIGITS];
};

static void
dec_norm(struct dec *v)
{
    int lead = 0;
    while (lead < v->len && v->d[lead] == '0')
        lead++;
    if (lead) {
        memmove(v->d, v->d + lead, v->len - lead);
        v->len -= lead;
    }
    while (v->len && v->d[v->len - 1] == '0') {
        v->len--;
        v->exp++;
    }
    if (v->len == 0)
        v->exp = 0;
}

static struct big tmp_big;

/* Set v to m * 2**e exactly, m is mw 32-bit words */
static void
dec_from_bin(struct dec *v, const uint32_t *m, int mw, int e)
{
    struct big *b = &tmp_big;
    char        rev[DEC_DIGITS];
    int         n = 0, i;

    memcpy(b->w, m, mw * sizeof(uint32_t));
    b->n = mw;
    while (b->n && b->w[b->n - 1] == 0)
        b->n--;
    v->exp = 0;
    if (e >= 0) {
        for (; e >= 31; e -= 31)
            big_mul_add(b, 1UL << 31, 0);
        big_mul_add(b, 1UL << e, 0);
    } else {
        v->exp = e;
        for (; e <= -13; e += 13)
            big_mul_add(b, 1220703125, 0);
        for (; e < 0; e++)
            big_mul_add(b, 5, 0);
    }
    while (b->n) {
        uint32_t r = big_div_small(b, 1000000000);
        for (i = 0; i < 9; i++) {
            rev[n++] = '0' + r % 10;
            r /= 10;
        }
    }
    for (i = 0; i < n; i++)
        v->d[i] = rev[n - 1 - i];
    v->len = n;
    dec_norm(v);
}

static int
dec_cmp(const struct dec *a, const struct dec *b)
{
    int i, ta, tb;

    if (a->len == 0 || b->len == 0)
        return (a->len != 0) - (b->len != 0);
    ta = a->len + a->exp;
    tb = b->len + b->exp;
    if (ta != tb)
        return ta < tb ? -1 : 1;
    for (i = 0; i < a->len || i < b->len; i++) {
        char ca = i < a->len ? a->d[i] : '0';
        char cb = i < b->len ? b->d[i] : '0';
        if (ca != cb)
            return ca < cb ? -1 : 1;
    }
    return 0;
}

/* Add one unit in the last place */
static void
dec_inc(struct dec *v)
{
    int i = v->len;
    while (i-- > 0) {
        if (v->d[i] != '9') {
            v->d[i]++;
            break;
        }
        v->d[i] = '0';
    }
    if (i < 0) {
        memmove(v->d + 1, v->d, v->len);
        v->d[0] = '1';
        v->len++;
    }
    dec_norm(v);
}

/* Keep the first k digits */
static void
dec_trunc(struct dec *v, int k)
{
    if (v->len > k) {
        v->exp += v->len - k;
        v->len = k;
        dec_norm(v);
    }
}

/* Append z zeros */
static void
dec_pad(struct dec *v, int z)
{
    memset(v->d + v->len, '0', z);
    v->len += z;
    v->exp -= z;
}

/* Subtract one unit in the last place */
static void
dec_dec(struct dec *v)
{
    int i = v->len;
    while (i-- > 0) {
        if (v->d[i] != '0') {
            v->d[i]--;
            break;
        }
        v->d[i] = '9';
    }
    dec_norm(v);
}

#define MAG_NEAREST 0
#define MAG_DOWN    1
#define MAG_UP      2

/* Round to CR_DIG digits as C23 F.5p3 specifies */
static void
dec_round_cr(struct dec *v, int mag)
{
    int up = 0;

    if (v->len <= CR_DIG)
        return;
    switch (mag) {
    case MAG_NEAREST:
        if (v->d[CR_DIG] > '5')
            up = 1;
        else if (v->d[CR_DIG] == '5')
            up = v->len > CR_DIG + 1 || ((v->d[CR_DIG - 1] - '0') & 1);
        break;
    case MAG_UP:
        up = 1; /* the discarded digits are not all zero */
        break;
    }
    /* Truncate without normalizing so dec_inc adds one in the last kept place */
    v->exp += v->len - CR_DIG;
    v->len = CR_DIG;
    if (up)
        dec_inc(v);
    else
        dec_norm(v);
}

static char str_buf[DEC_DIGITS + 32];

static const char *
dec_str(const struct dec *v, int neg, int style)
{
    char *s = str_buf;

    if (neg)
        *s++ = '-';
    if (v->len == 0) {
        strcpy(s, "0");
        return str_buf;
    }
    if (style & 1) {
        *s++ = v->d[0];
        *s++ = '.';
        memcpy(s, v->d + 1, v->len - 1);
        s += v->len - 1;
        sprintf(s, "e%d", v->exp + v->len - 1);
    } else {
        memcpy(s, v->d, v->len);
        s += v->len;
        sprintf(s, "e%d", v->exp);
    }
    return str_buf;
}

static uint32_t rand_state = 0x12345678;

static uint32_t
rnd(void)
{
    rand_state ^= rand_state << 13;
    rand_state ^= rand_state >> 17;
    rand_state ^= rand_state << 5;
    return rand_state;
}

static struct dec d_mid, d_x, d_s;

struct mode {
    int         mode;
    const char *name;
};

static const struct mode modes[] = {
    { FE_TONEAREST,  "nearest"    },
#ifdef FE_UPWARD
    { FE_UPWARD,     "upward"     },
#endif
#ifdef FE_DOWNWARD
    { FE_DOWNWARD,   "downward"   },
#endif
#ifdef FE_TOWARDZERO
    { FE_TOWARDZERO, "towardzero" },
#endif
};

#define NMODES (int)(sizeof(modes) / sizeof(modes[0]))

static int
mag_mode(int mode, int neg)
{
#ifdef FE_UPWARD
    if (mode == FE_UPWARD)
        return neg ? MAG_DOWN : MAG_UP;
#endif
#ifdef FE_DOWNWARD
    if (mode == FE_DOWNWARD)
        return neg ? MAG_UP : MAG_DOWN;
#endif
#ifdef FE_TOWARDZERO
    if (mode == FE_TOWARDZERO)
        return MAG_DOWN;
#endif
    (void)mode;
    (void)neg;
    return MAG_NEAREST;
}

/*
 * Generate the tests for one type. x is a random non-negative value,
 * below and above are its neighbours, the strings tested are near x
 * itself and near the midpoint between x and above.
 */
#define MAKE_TEST(T, NAME, STRTO, NEXT, FREXP, LDEXP, MANT, MIN_EXP, MAX, PMIN)                 \
    static void decompose_##NAME(T x, uint32_t *m, int *ep)                                     \
    {                                                                                           \
        int         e, i;                                                                       \
        long double v;                                                                          \
        (void)FREXP(x, &e);                                                                     \
        e -= MANT;                                                                              \
        if (x == 0 || e < (MIN_EXP) - (MANT))                                                   \
            e = (MIN_EXP) - (MANT);                                                             \
        v = (long double)LDEXP(x, -e);                                                          \
        for (i = 4; i-- > 0;) {                                                                 \
            uint32_t w = (uint32_t)ldexpl(v, -32 * i);                                          \
            m[i] = w;                                                                           \
            v -= ldexpl((long double)w, 32 * i);                                                \
        }                                                                                       \
        *ep = e;                                                                                \
    }                                                                                           \
                                                                                                \
    static void check_##NAME(const struct dec *s, int neg, int style, int m, T want)            \
    {                                                                                           \
        const char *str = dec_str(s, neg, style);                                               \
        char       *end;                                                                        \
        T           got;                                                                        \
        if (neg)                                                                                \
            want = -want;                                                                       \
        if (fesetround(modes[m].mode) != 0)                                                     \
            return;                                                                             \
        checks++;                                                                               \
        got = STRTO(str, &end);                                                                 \
        fesetround(FE_TONEAREST);                                                               \
        if (!(got == want && signbit(got) == signbit(want))) {                                  \
            if (errors < 50)                                                                    \
                printf(#STRTO " %s %.60s%s: got %a want %a\n", modes[m].name, str,              \
                       strlen(str) > 60 ? "..." : "", (double)got, (double)want);               \
            errors++;                                                                           \
        }                                                                                       \
        if (*end != '\0') {                                                                     \
            printf(#STRTO " %.60s: stopped at %.20s\n", str, end);                              \
            errors++;                                                                           \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    /* Check s, a value within half an ulp of x or between x and above */                       \
    static void try_##NAME(const struct dec *s, int style, T below, T x, T above, int x_even,   \
                           int at_mid)                                                          \
    {                                                                                           \
        int neg, m;                                                                             \
        for (neg = 0; neg < 2; neg++) {                                                         \
            for (m = 0; m < NMODES; m++) {                                                      \
                int mag = mag_mode(modes[m].mode, neg);                                         \
                T   want;                                                                       \
                int c;                                                                          \
                d_s = *s;                                                                       \
                dec_round_cr(&d_s, mag);                                                        \
                c = dec_cmp(&d_s, at_mid ? &d_mid : &d_x);                                      \
                if (at_mid) {                                                                   \
                    if (mag == MAG_NEAREST)                                                     \
                        want = c < 0 ? x : c > 0 ? above : x_even ? x : above;                  \
                    else if (mag == MAG_DOWN)                                                   \
                        want = x;                                                               \
                    else                                                                        \
                        want = above;                                                           \
                } else {                                                                        \
                    if (mag == MAG_NEAREST)                                                     \
                        want = x;                                                               \
                    else if (mag == MAG_DOWN)                                                   \
                        want = c < 0 ? below : x;                                               \
                    else                                                                        \
                        want = c > 0 ? above : x;                                               \
                }                                                                               \
                if (mag == MAG_DOWN && isinf(want))                                             \
                    want = MAX;                                                                 \
                check_##NAME(s, neg, style, m, want);                                           \
            }                                                                                   \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    /* Strings near the exact decimal value v */                                                \
    static void near_##NAME(const struct dec *v, T below, T x, T above, int x_even, int at_mid) \
    {                                                                                           \
        static const int  ks[] = { PMIN + 2, CR_DIG - 1, CR_DIG, CR_DIG + 1, CR_DIG + 3 };      \
        static struct dec t;                                                                    \
        unsigned          i;                                                                    \
        int               style = rnd() & 1;                                                    \
        if (v->len + PMIN + 4 < DEC_DIGITS) {                                                   \
            try_##NAME(v, style, below, x, above, x_even, at_mid);                              \
            t = *v; /* v + tiny */                                                              \
            dec_pad(&t, PMIN + 4);                                                              \
            t.d[t.len - 1] = '1';                                                               \
            try_##NAME(&t, style, below, x, above, x_even, at_mid);                             \
            t = *v; /* v - tiny */                                                              \
            dec_pad(&t, PMIN + 4);                                                              \
            dec_dec(&t);                                                                        \
            try_##NAME(&t, style, below, x, above, x_even, at_mid);                             \
        }                                                                                       \
        for (i = 0; i < sizeof(ks) / sizeof(ks[0]); i++) {                                      \
            if (v->len <= ks[i])                                                                \
                continue;                                                                       \
            t = *v;                                                                             \
            dec_trunc(&t, ks[i]);                                                               \
            try_##NAME(&t, style, below, x, above, x_even, at_mid);                             \
            t = *v;                                                                             \
            dec_trunc(&t, ks[i]);                                                               \
            if (t.len == ks[i]) {                                                               \
                dec_inc(&t);                                                                    \
                try_##NAME(&t, style, below, x, above, x_even, at_mid);                         \
            }                                                                                   \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    static void test_value_##NAME(T x)                                                          \
    {                                                                                           \
        uint32_t m[4];                                                                          \
        int      e;                                                                             \
        T        below = NEXT(x, (T)0);                                                         \
        T        above = NEXT(x, (T)INFINITY);                                                  \
        decompose_##NAME(x, m, &e);                                                             \
        if (x != 0) {                                                                           \
            dec_from_bin(&d_x, m, 4, e);                                                        \
            near_##NAME(&d_x, below, x, above, !(m[0] & 1), 0);                                 \
        }                                                                                       \
        /* midpoint (2m + 1) * 2**(e-1) */                                                      \
        {                                                                                       \
            uint32_t mm[5];                                                                     \
            uint64_t c = 1;                                                                     \
            int      i;                                                                         \
            for (i = 0; i < 4; i++) {                                                           \
                c += (uint64_t)m[i] * 2;                                                        \
                mm[i] = (uint32_t)c;                                                            \
                c >>= 32;                                                                       \
            }                                                                                   \
            mm[4] = (uint32_t)c;                                                                \
            dec_from_bin(&d_mid, mm, 5, e - 1);                                                 \
        }                                                                                       \
        near_##NAME(&d_mid, below, x, above, !(m[0] & 1), 1);                                   \
    }                                                                                           \
                                                                                                \
    static void test_##NAME(int count)                                                          \
    {                                                                                           \
        int i;                                                                                  \
        test_value_##NAME((T)0);                                                                \
        test_value_##NAME(NEXT((T)0, (T)1));                                                    \
        test_value_##NAME(MAX);                                                                 \
        test_value_##NAME(NEXT(MAX, (T)0));                                                     \
        test_value_##NAME(LDEXP((T)1, (MIN_EXP) - 1));                                          \
        test_value_##NAME(NEXT(LDEXP((T)1, (MIN_EXP) - 1), (T)0));                              \
        test_value_##NAME((T)1);                                                                \
        test_value_##NAME((T)16777216);                                                         \
        test_value_##NAME((T)9007199254740992.0L);                                              \
        for (i = 0; i < count; i++) {                                                           \
            T   v = 1;                                                                          \
            int b, e;                                                                           \
            for (b = 1; b < MANT; b++)                                                          \
                v = v * 2 + (T)(rnd() & 1);                                                     \
            if (rnd() & 1)                                                                      \
                e = (int)(rnd() % 120) - 60 - (MANT);                                           \
            else                                                                                \
                e = (int)(rnd() % (unsigned)(2 * -(MIN_EXP) + 2 * (MANT))) + (MIN_EXP)          \
                    - 2 * (MANT);                                                               \
            v = LDEXP(v, e);                                                                    \
            if (isinf(v))                                                                       \
                v = MAX;                                                                        \
            test_value_##NAME(v);                                                               \
        }                                                                                       \
    }

MAKE_TEST(float, float, strtof, nextafterf, frexpf, ldexpf, __FLT_MANT_DIG__, __FLT_MIN_EXP__,
          __FLT_MAX__, 9)
MAKE_TEST(double, double, strtod, nextafter, frexp, ldexp, __DBL_MANT_DIG__, __DBL_MIN_EXP__,
          __DBL_MAX__, 17)
#ifdef CHECK_LONG_DOUBLE
MAKE_TEST(long double, long_double, strtold, nextafterl, frexpl, ldexpl, __LDBL_MANT_DIG__,
          __LDBL_MIN_EXP__, __LDBL_MAX__, __LDBL_DECIMAL_DIG__)
#endif

/*
 * Inputs which were rounded twice before the fix. Strings with more
 * than CR_DIG digits round to CR_DIG digits first (C23 F.5p3), so
 * those carry a second pair of expected values for that case.
 */
static const struct {
    const char *s;
    int         ndig;
    double      d, d_cr;
    float       f, f_cr;
} fixed[] = {
    { "-18.2359332387141588",     18, -0x1.23c661ee85111p+4,  0,             -0x1.23c662p+4f, 0              },
    { "3879922696742144132e+134", 19, 0x1.da1dde912ac26p+506, 0,             (float)INFINITY, 0              },
    { "201.189382013492078",      18, 0x1.9260f6ade4cc5p+7,   0,             0x1.9260f6p+7f,  0              },
    { "16777217.000000001",       17, 0x1.000001p+24,         0,             0x1.000002p+24f, 0              },
    /* rounds to ...7754 with 20 digits, above the float midpoint */
    { "1.00000005960464477539",   21, 0x1.000001p+0,          0x1.000001p+0, 0x1p+0f,         0x1.000002p+0f },
    { "1.00000005960464477540",   21, 0x1.000001p+0,          0x1.000001p+0, 0x1.000002p+0f,  0x1.000002p+0f },
    /* rounds to the midpoint 2**53 + 1 with up to 22 digits */
    { "9007199254740993.0000001", 23, 0x1.0000000000001p+53,  0x1p+53,       0x1p+53f,        0x1p+53f       },
    { "1e400",                    1,  (double)INFINITY,       0,             (float)INFINITY, 0              },
    { "1e-400",                   1,  0.0,                    0,             0.0f,            0              },
};

#define NFIXED (int)(sizeof(fixed) / sizeof(fixed[0]))

static void
check_fixed(void)
{
    int    i;
    double d;
    float  f;

    for (i = 0; i < NFIXED; i++) {
        double want_d = fixed[i].ndig > CR_DIG ? fixed[i].d_cr : fixed[i].d;
        float  want_f = fixed[i].ndig > CR_DIG ? fixed[i].f_cr : fixed[i].f;

        d = strtod(fixed[i].s, NULL);
        if (d != want_d) {
            printf("strtod %s: got %a want %a\n", fixed[i].s, d, want_d);
            errors++;
        }
        f = strtof(fixed[i].s, NULL);
        if (f != want_f) {
            printf("strtof %s: got %a want %a\n", fixed[i].s, (double)f, (double)want_f);
            errors++;
        }
        d = 0;
        f = 0;
        if (sscanf(fixed[i].s, "%lf", &d) != 1 || d != want_d) {
            printf("sscanf %%lf %s: got %a want %a\n", fixed[i].s, d, want_d);
            errors++;
        }
        if (sscanf(fixed[i].s, "%f", &f) != 1 || f != want_f) {
            printf("sscanf %%f %s: got %a want %a\n", fixed[i].s, (double)f, (double)want_f);
            errors++;
        }
    }

    /*
     * 2**53 + 1 is a midpoint. With 39 significant digits, the string
     * is exact when CR_DIG >= 39; otherwise it rounds to the midpoint
     * and then to even.
     */
    d = strtod("9007199254740993.00000000000000000000001", NULL);
    if (d != (CR_DIG >= 39 ? 0x1.0000000000001p+53 : 0x1p+53)) {
        printf("strtod 2**53+1+tiny with CR_DIG %d: got %a\n", CR_DIG, d);
        errors++;
    }

#ifdef FE_UPWARD
    /* Hex input rounds in the current direction too */
    if (fesetround(FE_UPWARD) == 0) {
        d = strtod("0x1.00000000000008p0", NULL);
        fesetround(FE_TONEAREST);
        if (d != 0x1.0000000000001p0) {
            printf("strtod upward hex: got %a\n", d);
            errors++;
        }
    }
#endif
#ifdef FE_TOWARDZERO
    if (fesetround(FE_TOWARDZERO) == 0) {
        d = strtod("1e400", NULL);
        f = strtof("-1e400", NULL);
        fesetround(FE_TONEAREST);
        if (d != DBL_MAX || f != -FLT_MAX) {
            printf("towardzero overflow: got %a %a\n", d, (double)f);
            errors++;
        }
    }
#endif
}

int
main(void)
{
#ifdef SKIP_TESTS
    printf("skipping, io-float-exact disabled\n");
    return 77;
#endif
    check_fixed();
    test_float(NRANDOM * 4);
    test_double(NRANDOM * 4);
#ifdef CHECK_LONG_DOUBLE
    test_long_double(NRANDOM);
#endif
    printf("CR_DIG %d: %ld checks, %d errors\n", CR_DIG, checks, errors);
    return errors != 0;
}
