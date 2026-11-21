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

#define _POSIX_C_SOURCE 202405L
/* The Bessel functions are XSI */
#define _XOPEN_SOURCE 800
#include <math.h>

#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

/* Without an exception, the checks of it pass trivially */
#ifndef FE_DIVBYZERO
#define FE_DIVBYZERO 0
#endif
#ifndef FE_INEXACT
#define FE_INEXACT 0
#endif
#ifndef FE_INVALID
#define FE_INVALID 0
#endif
#ifndef FE_OVERFLOW
#define FE_OVERFLOW 0
#endif
#ifndef FE_UNDERFLOW
#define FE_UNDERFLOW 0
#endif

/* j0, j1 and jn: NaN, infinity, huge arguments and underflow */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define ERROR_EXCEPTS (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW)

static int    got_errno, got_except;
static double got;

/* Call expr with errno and the exception flags cleared, and note what it left */
#define CALL(expr)                                \
    do {                                          \
        errno = 0;                                \
        feclearexcept(FE_ALL_EXCEPT);             \
        got = (double)(expr);                     \
        got_except = fetestexcept(FE_ALL_EXCEPT); \
        got_errno = errno;                        \
    } while (0)

/*
 * A "may fail" error: when the implementation reports it, errno is
 * err and the exception is except; no other error is reported.
 */
static int
may_error(const char *what, int err, int except)
{
    int ok = 1;

    if ((math_errhandling & MATH_ERRNO) && got_errno != 0 && got_errno != err) {
        printf("%s: errno %d, expected 0 or %d\n", what, got_errno, err);
        ok = 0;
    }
    if ((math_errhandling & MATH_ERREXCEPT) && (got_except & ERROR_EXCEPTS & ~except) != 0) {
        printf("%s: exceptions 0x%x, expected only 0x%x\n", what, got_except, except);
        ok = 0;
    }
    return ok;
}

/* No error: errno untouched and no error exception */
static int
no_error(const char *what)
{
    return may_error(what, 0, 0);
}

static volatile double vzero = 0.0, vinf = HUGE_VAL;

/* A range error for a result which isn't representable returns 0.0 */
static int
range_zero(const char *what)
{
    if (!may_error(what, ERANGE, FE_UNDERFLOW))
        return 0;
    if (((math_errhandling & MATH_ERRNO) && got_errno == ERANGE) && got != 0) {
        printf("%s: range error returned %a\n", what, got);
        return 0;
    }
    return 1;
}

int
main(void)
{
    volatile double nanv = (double)NAN;
    unsigned        i;

    /* NaN gives NaN */
    CALL(j0(nanv));
    check(isnan(got) && no_error("j0(NaN)"), "j0(NaN)");
    CALL(j1(nanv));
    check(isnan(got) && no_error("j1(NaN)"), "j1(NaN)");
    CALL(jn(4, nanv));
    check(isnan(got) && no_error("jn(4, NaN)"), "jn(4, NaN)");

    /* +Inf gives +0 */
    CALL(j0(vinf));
    check(got == 0 && !signbit(got) && no_error("j0(Inf)"), "j0(Inf)");
    CALL(j1(vinf));
    check(got == 0 && !signbit(got) && no_error("j1(Inf)"), "j1(Inf)");
    CALL(jn(4, vinf));
    check(got == 0 && !signbit(got) && no_error("jn(4, Inf)"), "jn(4, Inf)");
#ifndef __GLIBC__
    /* glibc 2.43 returns -0 */
    CALL(jn(-3, vinf));
    check(got == 0 && !signbit(got) && no_error("jn(-3, Inf)"), "jn(-3, Inf)");
#endif

    /* Zero */
    CALL(j0(vzero));
    check(got == 1.0 && no_error("j0(0)"), "j0(0)");
    CALL(j1(vzero));
    check(got == 0 && no_error("j1(0)"), "j1(0)");
    CALL(jn(3, vzero));
    check(got == 0 && no_error("jn(3, 0)"), "jn(3, 0)");

    /* Huge arguments may be range errors, which return 0.0 */
    {
        static const double xs[] = { 1e20, 0x1p310, 1e300, DBL_MAX };

        for (i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
            volatile double x = xs[i];

            CALL(j0(x));
            check(fabs(got) < 1e-9 && range_zero("j0(huge)"), "j0(huge)");
            CALL(j1(-x));
            check(fabs(got) < 1e-9 && range_zero("j1(-huge)"), "j1(-huge)");
            CALL(jn(5, x));
            check(fabs(got) < 1e-9 && range_zero("jn(5, huge)"), "jn(5, huge)");
        }
    }

    /* An underflowing result which isn't representable is 0.0 */
    CALL(jn(100, 0x1p-20 + vzero));
    check(got == 0 && range_zero("jn(100, 2^-20)"), "jn(100, 2^-20)");
    CALL(jn(40, 0x1p-40 + vzero));
    check(got == 0 && range_zero("jn(40, 2^-40)"), "jn(40, 2^-40)");

    /* A representable underflowing result is the correct value */
    CALL(j1(0x1p-1030 + vzero));
    check(got == 0x1p-1031 && may_error("j1(2^-1030)", ERANGE, FE_UNDERFLOW), "j1(2^-1030)");
    CALL(jn(2, 0x1p-520 + vzero));
    check(got == 0x1p-1043 && may_error("jn(2, 2^-520)", ERANGE, FE_UNDERFLOW), "jn(2, 2^-520)");

    /* J(-n, x) is (-1)^n J(n, x) */
    {
        static const double xs[] = { 0x1p-30, 0.5, 1.0, 2.5, 10.0, 1e3 };

        for (i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
            volatile double x = xs[i];

            check(jn(0, x) == j0(x), "jn(0, x) is j0(x)");
            check(jn(1, x) == j1(x), "jn(1, x) is j1(x)");
            check(jn(-1, x) == -j1(x), "J(-1, x) is -J(1, x)");
            check(jn(-2, x) == jn(2, x), "J(-2, x) is J(2, x)");
        }
    }

    /* Ordinary arguments are not errors */
    CALL(j0(1.0 + vzero));
    check(got > 0.765 && got < 0.766 && no_error("j0(1)"), "j0(1)");
    CALL(jn(3, 2.0 + vzero));
    check(got > 0.128 && got < 0.129 && no_error("jn(3, 2)"), "jn(3, 2)");

    printf("success\n");
    return 0;
}
