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

/* y0, y1 and yn: NaN, negative, zero, infinity and overflow */

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

static volatile double vzero = 0.0, vtiny = 0x1p-1060, vinf = HUGE_VAL;

int
main(void)
{
    volatile double  nanv = (double)NAN;
    static const int orders[] = { 0, 1, 2, 3, -1, -2, -3, 10, -11 };
    unsigned         i;

    /* NaN gives NaN */
    CALL(y0(nanv));
    check(isnan(got) && no_error("y0(NaN)"), "y0(NaN)");
    CALL(y1(nanv));
    check(isnan(got) && no_error("y1(NaN)"), "y1(NaN)");
    CALL(yn(3, nanv));
    check(isnan(got) && no_error("yn(3, NaN)"), "yn(3, NaN)");

    /* A negative x gives NaN or the x == 0 value, and may be a domain error */
    CALL(y0(-1.0 + vzero));
    check((isnan(got) || got == -HUGE_VAL) && may_error("y0(-1)", EDOM, FE_INVALID), "y0(-1)");
    CALL(y1(-1.0 + vzero));
    check((isnan(got) || got == -HUGE_VAL) && may_error("y1(-1)", EDOM, FE_INVALID), "y1(-1)");
    CALL(yn(2, -1.0 + vzero));
    check((isnan(got) || got == -HUGE_VAL) && may_error("yn(2, -1)", EDOM, FE_INVALID),
          "yn(2, -1)");
    CALL(yn(-1, -1.0 + vzero));
    check((isnan(got) || got == HUGE_VAL) && may_error("yn(-1, -1)", EDOM, FE_INVALID),
          "yn(-1, -1)");
    CALL(y0(-vinf));
    check((isnan(got) || got == -HUGE_VAL) && may_error("y0(-Inf)", EDOM, FE_INVALID), "y0(-Inf)");

    /*
     * x == 0 is a pole: -HUGE_VAL, except +HUGE_VAL from yn when n is
     * negative and odd
     */
    CALL(y0(vzero));
    check(got == -HUGE_VAL && may_error("y0(0)", ERANGE, FE_DIVBYZERO), "y0(0)");
    CALL(y0(-vzero));
    check(got == -HUGE_VAL && may_error("y0(-0)", ERANGE, FE_DIVBYZERO), "y0(-0)");
    CALL(y1(vzero));
    check(got == -HUGE_VAL && may_error("y1(0)", ERANGE, FE_DIVBYZERO), "y1(0)");
    for (i = 0; i < sizeof(orders) / sizeof(orders[0]); i++) {
        int    n = orders[i];
        double expect = (n < 0 && (n & 1)) ? HUGE_VAL : -HUGE_VAL;
        char   what[32];

        snprintf(what, sizeof(what), "yn(%d, 0)", n);
        CALL(yn(n, vzero));
        if (got != expect) {
            printf("%s is %g, not %g\n", what, got, expect);
            exit(1);
        }
        check(may_error(what, ERANGE, FE_DIVBYZERO), "yn pole error");
        snprintf(what, sizeof(what), "yn(%d, -0)", n);
        CALL(yn(n, -vzero));
        if (got != expect) {
            printf("%s is %g, not %g\n", what, got, expect);
            exit(1);
        }
        check(may_error(what, ERANGE, FE_DIVBYZERO), "yn pole error");
    }

    /* +Inf gives +0 */
    CALL(y0(vinf));
    check(got == 0 && !signbit(got) && no_error("y0(Inf)"), "y0(Inf)");
    CALL(y1(vinf));
    check(got == 0 && !signbit(got) && no_error("y1(Inf)"), "y1(Inf)");
    CALL(yn(5, vinf));
    check(got == 0 && !signbit(got) && no_error("yn(5, Inf)"), "yn(5, Inf)");
    CALL(yn(-5, vinf));
    check(got == 0 && !signbit(got) && no_error("yn(-5, Inf)"), "yn(-5, Inf)");

    /*
     * Overflow gives -HUGE_VAL, except +HUGE_VAL from yn when n is
     * negative and odd
     */
    CALL(y1(vtiny));
    check(got == -HUGE_VAL && may_error("y1(tiny)", ERANGE, FE_OVERFLOW), "y1(tiny)");
    CALL(yn(2, vtiny));
    check(got == -HUGE_VAL && may_error("yn(2, tiny)", ERANGE, FE_OVERFLOW), "yn(2, tiny)");
    CALL(yn(-2, vtiny));
    check(got == -HUGE_VAL && may_error("yn(-2, tiny)", ERANGE, FE_OVERFLOW), "yn(-2, tiny)");
    CALL(yn(-1, vtiny));
    check(got == HUGE_VAL && may_error("yn(-1, tiny)", ERANGE, FE_OVERFLOW), "yn(-1, tiny)");
    CALL(yn(-3, vtiny));
    check(got == HUGE_VAL && may_error("yn(-3, tiny)", ERANGE, FE_OVERFLOW), "yn(-3, tiny)");
    CALL(yn(200, 1.0 + vzero));
    check(got == -HUGE_VAL && may_error("yn(200, 1)", ERANGE, FE_OVERFLOW), "yn(200, 1)");

    /* y0(x) is yn(0, x) and y1(x) is yn(1, x) */
    {
        static const double xs[] = { 0x1p-30, 0.5, 1.0, 2.5, 10.0, 1e3, 1e20 };

        for (i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
            volatile double x = xs[i];

            check(y0(x) == yn(0, x), "y0 is yn(0, x)");
            check(y1(x) == yn(1, x), "y1 is yn(1, x)");
            check(yn(-1, x) == -yn(1, x), "Y(-1, x) is -Y(1, x)");
        }
    }

    /* Ordinary arguments are not errors */
    CALL(y0(1.0 + vzero));
    check(got < 0.09 && got > 0.08 && no_error("y0(1)"), "y0(1)");
    CALL(yn(2, 1.0 + vzero));
    check(got < -1.6 && got > -1.7 && no_error("yn(2, 1)"), "yn(2, 1)");

    printf("success\n");
    return 0;
}
