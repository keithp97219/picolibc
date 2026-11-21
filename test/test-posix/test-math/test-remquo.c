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
#include <math.h>

#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <limits.h>
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

/* Without a rounding mode, fesetround fails and its pass is skipped */
#ifndef FE_UPWARD
#define FE_UPWARD -1
#endif
#ifndef FE_DOWNWARD
#define FE_DOWNWARD -2
#endif
#ifndef FE_TOWARDZERO
#define FE_TOWARDZERO -3
#endif

/*
 * The special values and errors of remquo, frexp, modf, scalbln,
 * scalbn, logb, ilogb, nan, copysign, log1p, atan, erfc, the rounding functions,
 * fmax and fmin, for each type and in each rounding mode
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define ERROR_EXCEPTS (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW)

static int got_errno, got_except;

#define CALLT(expr)                               \
    do {                                          \
        errno = 0;                                \
        feclearexcept(FE_ALL_EXCEPT);             \
        gotT = (expr);                            \
        got_except = fetestexcept(FE_ALL_EXCEPT); \
        got_errno = errno;                        \
    } while (0)

#define CALLI(var, expr)                          \
    do {                                          \
        errno = 0;                                \
        feclearexcept(FE_ALL_EXCEPT);             \
        var = (expr);                             \
        got_except = fetestexcept(FE_ALL_EXCEPT); \
        got_errno = errno;                        \
    } while (0)

/* A "may fail" error: when reported, errno is err and the exception except */
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

/* No error */
static int
no_error(const char *what)
{
    return may_error(what, 0, 0);
}

/* A "shall fail" error: reported as errno err and exception except */
static int
must_error(const char *what, int err, int except)
{
    int ok = 1;

    if ((math_errhandling & MATH_ERRNO) && got_errno != err) {
        printf("%s: errno %d, expected %d\n", what, got_errno, err);
        ok = 0;
    }
    if ((math_errhandling & MATH_ERREXCEPT) && (got_except & ERROR_EXCEPTS) != except) {
        printf("%s: exceptions 0x%x, expected 0x%x\n", what, got_except, except);
        ok = 0;
    }
    return ok;
}

#ifdef __GLIBC__
/* glibc 2.43's remquo doesn't set errno for its domain errors */
#define REMQUO_ERROR(what, err, except) may_error(what, err, except)
/* nor scalbln for an overflow rounded to the largest finite value */
#define DIRECTED_OVERFLOW(what, err, except) may_error(what, err, except)
/* nor logb for its pole error */
#define LOGB_ERROR(what, err, except) may_error(what, err, except)
#else
#define LOGB_ERROR(what, err, except)        must_error(what, err, except)
#define DIRECTED_OVERFLOW(what, err, except) must_error(what, err, except)
#define REMQUO_ERROR(what, err, except)      must_error(what, err, except)
#endif

#define T        double
#define F(n)     n
#define LIT(x)   x
#define MIN_SUB  0x1p-1074
#define MIN_NORM DBL_MIN
#define MIN_EXP  DBL_MIN_EXP
#define MANT_DIG DBL_MANT_DIG
static T gotd;
#define gotT gotd
#include "test-remquo-type.h"
#undef T
#undef F
#undef LIT
#undef MIN_SUB
#undef MIN_NORM
#undef MIN_EXP
#undef MANT_DIG
#undef gotT

#define T        float
#define F(n)     n##f
#define LIT(x)   x##f
#define MIN_SUB  0x1p-149f
#define MIN_NORM FLT_MIN
#define MIN_EXP  FLT_MIN_EXP
#define MANT_DIG FLT_MANT_DIG
static T gotf;
#define gotT gotf
#include "test-remquo-type.h"
#undef T
#undef F
#undef LIT
#undef MIN_SUB
#undef MIN_NORM
#undef MIN_EXP
#undef MANT_DIG
#undef gotT

#define T        long double
#define F(n)     n##l
#define LIT(x)   x##L
#define MIN_SUB  LDBL_TRUE_MIN
#define MIN_NORM LDBL_MIN
#define MIN_EXP  LDBL_MIN_EXP
#define MANT_DIG LDBL_MANT_DIG
static T gotl;
#define gotT gotl
#include "test-remquo-type.h"

static const int modes[] = { FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO };

int
main(void)
{
    unsigned i;

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        if (fesetround(modes[i]) != 0)
            continue;
        test_type(modes[i]);
        test_typef(modes[i]);
        test_typel(modes[i]);
    }
    fesetround(FE_TONEAREST);
    printf("success\n");
    return 0;
}
