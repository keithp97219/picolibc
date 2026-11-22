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
#include <complex.h>

#include <fenv.h>
#include "../test-fp-env.h"
#include <float.h>
#include <math.h>
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

/*
 * <complex.h>: the macros, CMPLX, and the special values of cabs,
 * carg, cproj, conj, csqrt and clog along their branch cuts
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifdef __STDC_NO_COMPLEX__
#error "__STDC_NO_COMPLEX__ is defined"
#endif

#if !defined(complex) || !defined(_Complex_I) || !defined(I)
#error "complex, _Complex_I or I is missing"
#endif
#if !defined(CMPLX) || !defined(CMPLXF) || !defined(CMPLXL)
#error "CMPLX, CMPLXF or CMPLXL is missing"
#endif

_Static_assert(_Generic(_Complex_I, float _Complex: 1, default: 0), "_Complex_I type");
_Static_assert(_Generic((complex double)0, double _Complex: 1, default: 0), "complex");
_Static_assert(_Generic(CMPLX(1, 2), double complex: 1, default: 0), "CMPLX type");
_Static_assert(_Generic(CMPLXF(1, 2), float complex: 1, default: 0), "CMPLXF type");
_Static_assert(_Generic(CMPLXL(1, 2), long double complex: 1, default: 0), "CMPLXL type");

/* CMPLX makes initializers for static objects, and keeps infinite and NaN parts apart */
static double complex      zs = CMPLX(0.0, INFINITY);
static float complex       zsf = CMPLXF(-0.0f, NAN);
static long double complex zsl = CMPLXL(1.0L, -INFINITY);

#define PROTO(f, t) _Static_assert(_Generic(&f, t: 1, default: 0), #f " prototype")
#define C1(fn)                                                 \
    PROTO(fn, double complex (*)(double complex));             \
    PROTO(fn##f, float complex (*)(float complex));            \
    PROTO(fn##l, long double complex (*)(long double complex))
#define R1(fn)                                         \
    PROTO(fn, double (*)(double complex));             \
    PROTO(fn##f, float (*)(float complex));            \
    PROTO(fn##l, long double (*)(long double complex))

R1(cabs);
C1(cacos);
C1(cacosh);
R1(carg);
C1(casin);
C1(casinh);
C1(catan);
C1(catanh);
C1(ccos);
C1(ccosh);
C1(cexp);
R1(cimag);
C1(clog);
C1(conj);
PROTO(cpow, double complex (*)(double complex, double complex));
PROTO(cpowf, float complex (*)(float complex, float complex));
PROTO(cpowl, long double complex (*)(long double complex, long double complex));
C1(cproj);
R1(creal);
C1(csin);
C1(csinh);
C1(csqrt);
C1(ctan);
C1(ctanh);

static volatile double vzero = 0.0, vinf = HUGE_VAL, vnan = (double)NAN, vone = 1.0;

#define same(a, b) ((a) == (b) && !signbit(a) == !signbit(b))

int
main(void)
{
    double complex z, r;
    double         pi = 3.141592653589793;

    /* I squared is -1 */
    z = (double complex)I * (double complex)I;
    check(creal(z) == -1.0 && cimag(z) == 0.0, "I * I");

    /* CMPLX */
    check(creal(zs) == 0.0 && !signbit(creal(zs)) && isinf(cimag(zs)), "CMPLX(0, Inf)");
    check(crealf(zsf) == 0.0f && signbit(crealf(zsf)) && isnan(cimagf(zsf)), "CMPLXF(-0, NaN)");
    check(creall(zsl) == 1.0L && isinf(cimagl(zsl)) && cimagl(zsl) < 0, "CMPLXL(1, -Inf)");
    z = CMPLX(vnan, -vzero);
    check(isnan(creal(z)) && same(cimag(z), -0.0), "CMPLX(NaN, -0)");

    /* creal, cimag and conj */
    z = CMPLX(3.0, -0.0);
    check(creal(z) == 3.0 && same(cimag(z), -0.0), "creal and cimag");
    r = conj(z);
    check(creal(r) == 3.0 && same(cimag(r), 0.0), "conj flips the sign of zero");
    r = conj(CMPLX(vone, vinf));
    check(creal(r) == 1.0 && cimag(r) == -HUGE_VAL, "conj(1 + Inf i)");

    /* cabs: Inf with any NaN part is +Inf; symmetric in its parts */
    check(cabs(CMPLX(vinf, vnan)) == HUGE_VAL, "cabs(Inf + NaN i)");
    check(cabs(CMPLX(vnan, -vinf)) == HUGE_VAL, "cabs(NaN - Inf i)");
    check(isnan(cabs(CMPLX(vnan, vone))), "cabs(NaN + 1 i)");
    check(same(cabs(CMPLX(-vzero, -vzero)), 0.0), "cabs(-0 - 0i)");
    check(cabs(CMPLX(3.0, 4.0)) == 5.0, "cabs(3 + 4i)");
    check(cabs(CMPLX(0.25 + vzero, 7.5)) == cabs(CMPLX(7.5, 0.25 + vzero))
              && cabs(CMPLX(0.25 + vzero, 7.5)) == cabs(CMPLX(0.25 + vzero, -7.5)),
          "cabs symmetry");

    /* carg: the branch cut on the negative real axis */
    check(carg(CMPLX(-vone, vzero)) == pi, "carg(-1 + 0i)");
    check(carg(CMPLX(-vone, -vzero)) == -pi, "carg(-1 - 0i)");
    check(carg(CMPLX(-vzero, vzero)) == pi, "carg(-0 + 0i)");
    check(same(carg(CMPLX(vzero, -vzero)), -0.0), "carg(+0 - 0i)");
    check(carg(CMPLX(vzero, -vone)) == -pi / 2, "carg(0 - i)");
    check(carg(CMPLX(-vinf, vinf)) == 3 * pi / 4, "carg(-Inf + Inf i)");
    check(isnan(carg(CMPLX(vnan, vone))), "carg(NaN + i)");

    /* cproj: every infinity projects to +Inf + i copysign(0, imaginary part) */
    r = cproj(CMPLX(-vinf, -2.0));
    check(creal(r) == HUGE_VAL && same(cimag(r), -0.0), "cproj(-Inf - 2i)");
    r = cproj(CMPLX(vnan, vinf));
    check(creal(r) == HUGE_VAL && same(cimag(r), 0.0), "cproj(NaN + Inf i)");
    r = cproj(CMPLX(vone, -vinf));
    check(creal(r) == HUGE_VAL && same(cimag(r), -0.0), "cproj(1 - Inf i)");
    r = cproj(CMPLX(2.0, -3.0));
    check(creal(r) == 2.0 && cimag(r) == -3.0, "cproj(2 - 3i)");
    {
        float complex       rf = cprojf(CMPLXF(1.0f, -INFINITY));
        long double complex rl = cprojl(CMPLXL(-INFINITY, 0.0L));

        check(crealf(rf) == HUGE_VALF && signbit(cimagf(rf)), "cprojf");
        check(creall(rl) == HUGE_VALL && cimagl(rl) == 0 && !signbit(cimagl(rl)), "cprojl");
    }

    /* csqrt: the branch cut, and the right half-plane */
    r = csqrt(CMPLX(-4.0 + vzero, vzero));
    check(same(creal(r), 0.0) && cimag(r) == 2.0, "csqrt(-4 + 0i)");
    r = csqrt(CMPLX(-4.0 + vzero, -vzero));
    check(same(creal(r), 0.0) && cimag(r) == -2.0, "csqrt(-4 - 0i)");
    r = csqrt(CMPLX(-vzero, vzero));
    check(same(creal(r), 0.0) && same(cimag(r), 0.0), "csqrt(-0 + 0i)");
    r = csqrt(CMPLX(vnan, vinf));
    check(creal(r) == HUGE_VAL && cimag(r) == HUGE_VAL, "csqrt(NaN + Inf i)");
    r = csqrt(CMPLX(-vinf, vone));
    check(same(creal(r), 0.0) && cimag(r) == HUGE_VAL, "csqrt(-Inf + i)");

    {
        volatile float zf = 0.0f;
        float complex  rf = csqrtf(CMPLXF(-4.0f + zf, -zf));

        check(crealf(rf) == 0 && !signbit(crealf(rf)) && cimagf(rf) == -2.0f, "csqrtf(-4 - 0i)");
        rf = csqrtf(CMPLXF((float)NAN, -HUGE_VALF));
        check(crealf(rf) == HUGE_VALF && cimagf(rf) == -HUGE_VALF, "csqrtf(NaN - Inf i)");
    }

    /* clog: the pole at zero and the branch cut */
    feclearexcept(FE_ALL_EXCEPT);
    r = clog(CMPLX(-vzero, vzero));
    check(creal(r) == -HUGE_VAL && cimag(r) == pi, "clog(-0 + 0i)");
    if ((math_errhandling & MATH_ERREXCEPT) && TEST_DOUBLE_RAISES)
        check(fetestexcept(FE_DIVBYZERO), "clog(-0 + 0i) raises divide-by-zero");
    feclearexcept(FE_ALL_EXCEPT);
    r = clog(CMPLX(vzero, -vzero));
    check(creal(r) == -HUGE_VAL && same(cimag(r), -0.0), "clog(+0 - 0i)");
    if ((math_errhandling & MATH_ERREXCEPT) && TEST_DOUBLE_RAISES)
        check(fetestexcept(FE_DIVBYZERO), "clog(+0 - 0i) raises divide-by-zero");
    r = clog(CMPLX(-vone, -vzero));
    check(same(creal(r), 0.0) && cimag(r) == -pi, "clog(-1 - 0i)");
    r = clog(CMPLX(vone, vinf));
    check(creal(r) == HUGE_VAL && cimag(r) == pi / 2, "clog(1 + Inf i)");

    printf("success\n");
    return 0;
}
