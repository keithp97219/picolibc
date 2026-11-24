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
 * The C23, GNU, BSD and picolibc classification names are functions in
 * C++, never macros, and accept each floating and integer type.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <math.h>
#include <stdio.h>

#if defined(issubnormal) || defined(iszero) || defined(issignaling) || defined(iscanonical) \
    || defined(iseqsig)
#error C23 classification name defined as a macro
#endif
#if defined(isfinitef) || defined(isinff) || defined(isnanf) || defined(isnormalf) \
    || defined(iszerof)
#error float classification name defined as a macro
#endif
#if defined(isfinitel) || defined(isinfl) || defined(isnanl) || defined(isnormall) \
    || defined(iszerol)
#error long double classification name defined as a macro
#endif

/* Member functions with these names, as in the report */
struct value {
    double v;
    bool   iszero() const { return v == 0; }
    bool   issubnormal() const { return false; }
    bool   isfinitef() const { return true; }
};

static int errors;

#define check(expr)                                                \
    do {                                                           \
        if (!(expr)) {                                             \
            printf("%s:%d: %s failed\n", __FILE__, __LINE__, #expr); \
            errors++;                                              \
        }                                                          \
    } while (0)

int
main(void)
{
    struct value zero = { 0.0 };

    check(zero.iszero());
    check(!zero.issubnormal());
    check(zero.isfinitef());

    check(iszero(0.0f));
    check(iszero(0.0));
    check(iszero(0.0L));
    check(iszero(0));
    check(!iszero(1.0f));
    check(!iszero(1.0));
    check(!iszero(1.0L));
    check(!iszero(1L));

    check(!issubnormal(1.0f));
    check(!issubnormal(0.0));
    check(!issubnormal(1));
#ifdef __FLT_HAS_DENORM__
    check(issubnormal(__FLT_DENORM_MIN__));
#endif
#ifdef __DBL_HAS_DENORM__
    check(issubnormal(__DBL_DENORM_MIN__));
#endif

    check(!issignaling(1.0f));
    check(!issignaling(1.0));
    check(!issignaling(1.0L));
    check(!issignaling(1));

    check(iscanonical(1.0f));
    check(iscanonical(1.0));
    check(iscanonical(1.0L));
    check(iscanonical(1u));

    check(iseqsig(1.0f, 1.0f));
    check(iseqsig(1.0f, 1));
    check(iseqsig(1, 1.0));
    check(!iseqsig(1.0, 2.0L));
    check(!iseqsig(2, 1));

    check(isfinitef(1.0f));
    check(isnormalf(1.0f));
    check(!isnormalf(0.0f));
    check(iszerof(0.0f));
    check(!isinff(1.0f));
    check(!isnanf(1.0f));
#ifdef __HAVE_LONG_DOUBLE
    check(isfinitel(1.0L));
    check(isnormall(1.0L));
    check(iszerol(0.0L));
    check(!isinfl(1.0L));
    check(!isnanl(1.0L));
#endif

#ifndef __FINITE_MATH_ONLY__
    check(!issignaling(NAN));
    check(!iszero(NAN));
    check(!iseqsig(NAN, NAN));
    check(!isfinitef(INFINITY));
    check(isinff(INFINITY));
    check(isnanf(NAN));
#ifdef __HAVE_LONG_DOUBLE
    check(isinfl((long double)INFINITY));
    check(isnanl((long double)NAN));
#endif
#endif

    return errors != 0;
}
