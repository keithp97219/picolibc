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
#include <fenv.h>
#include "../test-fp-env.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* gcc doesn't know FENV_ACCESS; the build uses -frounding-math -ftrapping-math */
#ifdef __clang__
/* Some clang targets, such as hexagon, ignore it */
#pragma clang diagnostic ignored "-Wignored-pragmas"
#pragma STDC FENV_ACCESS ON
#endif

/*
 * The exception flag functions, the rounding mode functions and the
 * environment functions, and what the rounding mode does to arithmetic
 * and the rounding functions
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#if defined(FE_DIVBYZERO) && defined(FE_INEXACT) && defined(FE_INVALID) && defined(FE_OVERFLOW) \
    && defined(FE_UNDERFLOW) && defined(FE_DOWNWARD) && defined(FE_TONEAREST)                   \
    && defined(FE_TOWARDZERO) && defined(FE_UPWARD)

#define ALL_FIVE (FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

/*
 * The flags raised, but for inexact: feraiseexcept may raise it along
 * with overflow or underflow
 */
#define RAISED() (fetestexcept(FE_ALL_EXCEPT) & ~FE_INEXACT)

static volatile float       onef = 1.0f, threef = 3.0f, zerof = 0.0f;
static volatile double      one = 1.0, three = 3.0, zero = 0.0, two = 2.0;
static volatile long double onel = 1.0L;

static const int            modes[] = { FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO };
static const char * const   mode_names[]
    = { "FE_TONEAREST", "FE_UPWARD", "FE_DOWNWARD", "FE_TOWARDZERO" };

/* rint, nearbyint, lrint and llrint of 2.5 and -2.5 in each mode */
static const double rint_pos[] = { 2.0, 3.0, 2.0, 2.0 };
static const double rint_neg[] = { -2.0, -2.0, -3.0, -2.0 };

static void
check_mode(unsigned m)
{
    int             mode = modes[m];
    const char     *name = mode_names[m];
    volatile double half = 2.5;

    check(fesetround(mode) == 0, name);
    check(fegetround() == mode, name);

    /* Arithmetic follows the mode, for each type */
#define CHECK_ADD(T, o, EPS, what)                                                      \
    do {                                                                                \
        volatile T up = o + (EPS) / 4, down = -o - (EPS) / 4, third = o / (o + o + o);  \
        T          expect_up = (mode == FE_UPWARD) ? o + (EPS) : o;                     \
        T          expect_down = (mode == FE_DOWNWARD) ? -o - (EPS) : -o;               \
        if (up != expect_up || down != expect_down) {                                   \
            printf("%s: %s 1 + eps/4 or -1 - eps/4 rounded wrongly\n", name, what);     \
            exit(1);                                                                    \
        }                                                                               \
        if ((mode == FE_UPWARD) != (third * (o + o + o) > o) && mode != FE_TONEAREST) { \
            printf("%s: %s 1/3 rounded wrongly\n", name, what);                         \
            exit(1);                                                                    \
        }                                                                               \
    } while (0)

    CHECK_ADD(float, onef, FLT_EPSILON, "float");
    if (TEST_DOUBLE_ROUNDS)
        CHECK_ADD(double, one, DBL_EPSILON, "double");
    if (TEST_LONG_DOUBLE_ROUNDS)
        CHECK_ADD(long double, onel, LDBL_EPSILON, "long double");
    check(rintf((float)half) == (float)rint_pos[m] && lrintf((float)-half) == (long)rint_neg[m],
          "rintf, lrintf");
    if (TEST_LONG_DOUBLE_ROUNDS)
        check(rintl((long double)half) == (long double)rint_pos[m]
                  && llrintl((long double)-half) == (long long)rint_neg[m],
              "rintl, llrintl");

    /* round and lround don't */
    check(round(half) == 3.0 && lround(-half) == -3, "round");

    /* The rest is double, which may be done in software ignoring the mode */
    if (!TEST_DOUBLE_ROUNDS)
        return;

    /* sqrt is correctly rounded: sqrt(2) upward and downward differ */
    {
        volatile double r = sqrt(two);

        if (mode == FE_UPWARD)
            check(r * r > 2.0, "sqrt rounds upward");
        else if (mode == FE_DOWNWARD || mode == FE_TOWARDZERO)
            check(r * r < 2.0 || r == 0x1.6a09e667f3bccp+0, "sqrt rounds down");
    }

    /* fma rounds once, in the current mode */
    {
        volatile double r = fma(one, one, DBL_EPSILON / 4);

        check(r == (mode == FE_UPWARD ? 1.0 + DBL_EPSILON : 1.0), "fma rounding");
    }

    /* rint, nearbyint, lrint and llrint use the current mode */
    check(rint(half) == rint_pos[m] && rint(-half) == rint_neg[m], "rint");
    check(nearbyint(half) == rint_pos[m] && nearbyint(-half) == rint_neg[m], "nearbyint");
    check(lrint(half) == (long)rint_pos[m] && lrint(-half) == (long)rint_neg[m], "lrint");
    check(llrint(half) == (long long)rint_pos[m] && llrint(-half) == (long long)rint_neg[m],
          "llrint");
}

int
main(void)
{
    fexcept_t flag, flag2;
    fenv_t    env, env2;
    unsigned  m;
    int       r;

    /* The default environment is in place at startup */
    check(fegetround() == FE_TONEAREST, "startup rounding mode");
    check(fetestexcept(FE_ALL_EXCEPT) == 0, "startup exception flags");

    /* feraiseexcept, fetestexcept, feclearexcept */
    check(feraiseexcept(0) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0, "feraiseexcept(0)");
    check(feraiseexcept(FE_INVALID | FE_DIVBYZERO) == 0, "feraiseexcept");
    check(fetestexcept(FE_INVALID) == FE_INVALID, "fetestexcept one");
    check(fetestexcept(FE_ALL_EXCEPT) == (FE_INVALID | FE_DIVBYZERO), "fetestexcept all");
    check(fetestexcept(FE_INVALID | FE_OVERFLOW) == FE_INVALID, "fetestexcept part");
    check(feclearexcept(FE_INVALID) == 0, "feclearexcept");
    check(fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO, "feclearexcept clears only its flags");
    check(feclearexcept(0) == 0 && fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO, "feclearexcept(0)");
    check(feclearexcept(FE_ALL_EXCEPT) == 0 && fetestexcept(FE_ALL_EXCEPT) == 0,
          "feclearexcept(FE_ALL_EXCEPT)");
    check(feraiseexcept(ALL_FIVE) == 0 && fetestexcept(ALL_FIVE) == ALL_FIVE,
          "feraiseexcept all five");
    feclearexcept(FE_ALL_EXCEPT);

    /* The arithmetic raises the flags, for each type that raises them */
#define CHECK_RAISE(T, o, z, th, MAX, MIN, what)                                         \
    do {                                                                                 \
        volatile T v;                                                                    \
        v = o / z;                                                                       \
        check(fetestexcept(ALL_FIVE) == FE_DIVBYZERO, what " 1/0");                      \
        feclearexcept(FE_ALL_EXCEPT);                                                    \
        v = z / z;                                                                       \
        check(fetestexcept(ALL_FIVE) == FE_INVALID, what " 0/0");                        \
        feclearexcept(FE_ALL_EXCEPT);                                                    \
        v = o / th;                                                                      \
        check(fetestexcept(ALL_FIVE) == FE_INEXACT, what " 1/3");                        \
        feclearexcept(FE_ALL_EXCEPT);                                                    \
        v = MAX * th;                                                                    \
        check(fetestexcept(ALL_FIVE) == (FE_OVERFLOW | FE_INEXACT), what " overflow");   \
        feclearexcept(FE_ALL_EXCEPT);                                                    \
        v = MIN / th;                                                                    \
        check(fetestexcept(ALL_FIVE) == (FE_UNDERFLOW | FE_INEXACT), what " underflow"); \
        feclearexcept(FE_ALL_EXCEPT);                                                    \
        (void)v;                                                                         \
    } while (0)

    if (TEST_FLOAT_RAISES)
        CHECK_RAISE(float, onef, zerof, threef, FLT_MAX, FLT_MIN, "float");
    if (TEST_DOUBLE_RAISES)
        CHECK_RAISE(double, one, zero, three, DBL_MAX, DBL_MIN, "double");

    /* fegetexceptflag and fesetexceptflag */
    feraiseexcept(FE_OVERFLOW | FE_INEXACT);
    check(fegetexceptflag(&flag, FE_ALL_EXCEPT) == 0, "fegetexceptflag");
    check(fegetexceptflag(&flag2, FE_OVERFLOW) == 0, "fegetexceptflag one");
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_INVALID);
    check(fesetexceptflag(&flag, FE_ALL_EXCEPT) == 0, "fesetexceptflag");
    check(fetestexcept(FE_ALL_EXCEPT) == (FE_OVERFLOW | FE_INEXACT),
          "fesetexceptflag sets the saved state");
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_UNDERFLOW);
    check(fesetexceptflag(&flag, FE_OVERFLOW) == 0, "fesetexceptflag one");
    check(RAISED() == (FE_OVERFLOW | FE_UNDERFLOW),
          "fesetexceptflag sets only the flags asked for");
    check(fesetexceptflag(&flag2, FE_OVERFLOW) == 0 && RAISED() == (FE_OVERFLOW | FE_UNDERFLOW),
          "fesetexceptflag with a partial flag");
    check(fesetexceptflag(&flag, FE_UNDERFLOW) == 0 && RAISED() == FE_OVERFLOW,
          "fesetexceptflag clears a flag the saved state lacks");
    feclearexcept(FE_ALL_EXCEPT);

    /* fesetround rejects a mode that isn't one, leaving the mode alone */
    check(fesetround(FE_UPWARD) == 0, "fesetround(FE_UPWARD)");
    r = fesetround(-1);
    check(r != 0 && fegetround() == FE_UPWARD, "fesetround(-1)");
    r = fesetround(0x12345);
    check(r != 0 && fegetround() == FE_UPWARD, "fesetround(0x12345)");
    check(fesetround(FE_TONEAREST) == 0, "fesetround(FE_TONEAREST)");

    /* Each mode affects arithmetic and the rounding functions */
    for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++)
        check_mode(m);
    fesetround(FE_TONEAREST);
    feclearexcept(FE_ALL_EXCEPT);

    /* fegetenv and fesetenv save and restore the mode and the flags */
    fesetround(FE_DOWNWARD);
    feraiseexcept(FE_INEXACT);
    check(fegetenv(&env) == 0, "fegetenv");
    fesetround(FE_UPWARD);
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_INVALID);
    check(fesetenv(&env) == 0, "fesetenv");
    check(fegetround() == FE_DOWNWARD && fetestexcept(FE_ALL_EXCEPT) == FE_INEXACT,
          "fesetenv restores the environment");

    /* FE_DFL_ENV is the startup environment */
    check(fesetenv(FE_DFL_ENV) == 0, "fesetenv(FE_DFL_ENV)");
    check(fegetround() == FE_TONEAREST && fetestexcept(FE_ALL_EXCEPT) == 0,
          "FE_DFL_ENV is the default");

    /* feholdexcept saves the environment and clears the flags */
    fesetround(FE_TOWARDZERO);
    feraiseexcept(FE_DIVBYZERO);
    check(feholdexcept(&env) == 0, "feholdexcept");
    check(fetestexcept(FE_ALL_EXCEPT) == 0, "feholdexcept clears the flags");
    check(fegetround() == FE_TOWARDZERO, "feholdexcept keeps the mode");
    {
        /* In non-stop mode an exception doesn't trap */
        volatile float v = onef / zerof;

        check(v == HUGE_VALF && fetestexcept(FE_DIVBYZERO), "non-stop 1/0");
    }
    fesetround(FE_UPWARD);

    /* feupdateenv installs the environment, then raises the saved exceptions */
    feclearexcept(FE_ALL_EXCEPT);
    feraiseexcept(FE_OVERFLOW);
    check(feupdateenv(&env) == 0, "feupdateenv");
    check(fegetround() == FE_TOWARDZERO, "feupdateenv installs the mode");
    check(RAISED() == (FE_DIVBYZERO | FE_OVERFLOW), "feupdateenv keeps the raised exceptions");

    /* and with FE_DFL_ENV */
    feclearexcept(FE_ALL_EXCEPT);
    check(fegetenv(&env2) == 0, "fegetenv");
    feraiseexcept(FE_UNDERFLOW);
    check(feupdateenv(FE_DFL_ENV) == 0, "feupdateenv(FE_DFL_ENV)");
    check(fegetround() == FE_TONEAREST && RAISED() == FE_UNDERFLOW, "feupdateenv(FE_DFL_ENV)");
    check(fesetenv(&env2) == 0 && fegetround() == FE_TOWARDZERO && fetestexcept(FE_ALL_EXCEPT) == 0,
          "fesetenv of a saved environment");

    fesetenv(FE_DFL_ENV);
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no IEC 60559 exceptions and rounding modes, skipping\n");
    return 77;
}

#endif
