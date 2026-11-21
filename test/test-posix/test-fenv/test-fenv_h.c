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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* <fenv.h>'s types, macros and prototypes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The exception macros are distinct bits; FE_ALL_EXCEPT includes them all */
#ifdef FE_DIVBYZERO
_Static_assert((FE_DIVBYZERO & (FE_DIVBYZERO - 1)) == 0, "FE_DIVBYZERO is one bit");
_Static_assert((FE_ALL_EXCEPT & FE_DIVBYZERO) == FE_DIVBYZERO, "FE_ALL_EXCEPT has FE_DIVBYZERO");
#endif
#ifdef FE_INEXACT
_Static_assert((FE_INEXACT & (FE_INEXACT - 1)) == 0, "FE_INEXACT is one bit");
_Static_assert((FE_ALL_EXCEPT & FE_INEXACT) == FE_INEXACT, "FE_ALL_EXCEPT has FE_INEXACT");
#endif
#ifdef FE_INVALID
_Static_assert((FE_INVALID & (FE_INVALID - 1)) == 0, "FE_INVALID is one bit");
_Static_assert((FE_ALL_EXCEPT & FE_INVALID) == FE_INVALID, "FE_ALL_EXCEPT has FE_INVALID");
#endif
#ifdef FE_OVERFLOW
_Static_assert((FE_OVERFLOW & (FE_OVERFLOW - 1)) == 0, "FE_OVERFLOW is one bit");
_Static_assert((FE_ALL_EXCEPT & FE_OVERFLOW) == FE_OVERFLOW, "FE_ALL_EXCEPT has FE_OVERFLOW");
#endif
#ifdef FE_UNDERFLOW
_Static_assert((FE_UNDERFLOW & (FE_UNDERFLOW - 1)) == 0, "FE_UNDERFLOW is one bit");
_Static_assert((FE_ALL_EXCEPT & FE_UNDERFLOW) == FE_UNDERFLOW, "FE_ALL_EXCEPT has FE_UNDERFLOW");
#endif
#if defined(FE_DIVBYZERO) && defined(FE_INEXACT) && defined(FE_INVALID) && defined(FE_OVERFLOW) \
    && defined(FE_UNDERFLOW)
#define HAVE_ALL_EXCEPT 1
_Static_assert((FE_DIVBYZERO ^ FE_INEXACT ^ FE_INVALID ^ FE_OVERFLOW ^ FE_UNDERFLOW)
                   == (FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW),
               "exception bits distinct");
#else
#define HAVE_ALL_EXCEPT 0
#endif
#ifndef FE_ALL_EXCEPT
#error "FE_ALL_EXCEPT"
#endif

/* The rounding macros are distinct and non-negative */
#if defined(FE_DOWNWARD) && defined(FE_TONEAREST) && defined(FE_TOWARDZERO) && defined(FE_UPWARD)
#define HAVE_ALL_ROUND 1
_Static_assert(FE_DOWNWARD >= 0 && FE_TONEAREST >= 0 && FE_TOWARDZERO >= 0 && FE_UPWARD >= 0,
               "rounding macros non-negative");
_Static_assert(FE_DOWNWARD != FE_TONEAREST && FE_DOWNWARD != FE_TOWARDZERO
                   && FE_DOWNWARD != FE_UPWARD && FE_TONEAREST != FE_TOWARDZERO
                   && FE_TONEAREST != FE_UPWARD && FE_TOWARDZERO != FE_UPWARD,
               "rounding macros distinct");
#else
#define HAVE_ALL_ROUND 0
#endif

/* FE_DFL_ENV is a pointer to const fenv_t */
#ifndef FE_DFL_ENV
#error "FE_DFL_ENV"
#endif
_Static_assert(_Generic(FE_DFL_ENV, const fenv_t *: 1, default: 0), "FE_DFL_ENV type");

/* fexcept_t is a type */
static fexcept_t flag_object __attribute__((unused));

/* The prototypes */
#define PROTO(f, t) _Static_assert(_Generic(&f, t: 1, default: 0), #f " prototype")
PROTO(feclearexcept, int (*)(int));
PROTO(fegetenv, int (*)(fenv_t *));
PROTO(fegetexceptflag, int (*)(fexcept_t *, int));
PROTO(fegetround, int (*)(void));
PROTO(feholdexcept, int (*)(fenv_t *));
PROTO(feraiseexcept, int (*)(int));
PROTO(fesetenv, int (*)(const fenv_t *));
PROTO(fesetexceptflag, int (*)(const fexcept_t *, int));
PROTO(fesetround, int (*)(int));
PROTO(fetestexcept, int (*)(int));
PROTO(feupdateenv, int (*)(const fenv_t *));

int
main(void)
{
    /* The IEC 60559 option has all five exceptions and all four rounding modes */
    if (math_errhandling & MATH_ERREXCEPT)
        check(HAVE_ALL_EXCEPT, "all five exceptions");
#ifdef __STDC_IEC_559__
    check(HAVE_ALL_EXCEPT && HAVE_ALL_ROUND, "IEC 60559 macros");
#endif
    printf("success\n");
    return 0;
}
