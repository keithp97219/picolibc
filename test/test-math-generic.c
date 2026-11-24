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
 * The classification and comparison macros pick the float, double or
 * long double function by the argument's type, and work for other
 * floating types such as _Float16. Built with -std=c99
 * -pedantic-errors, which needs the _Generic they use to be marked as
 * an extension.
 */

#define _GNU_SOURCE
/* The <ssp/...> fortify wrappers aren't -pedantic clean */
#undef _FORTIFY_SOURCE
#include <math.h>
#include <stdio.h>

static int errors;

#define check(expr)                                                  \
    do {                                                             \
        if (!(expr)) {                                               \
            printf("%s:%d: %s failed\n", __FILE__, __LINE__, #expr); \
            errors++;                                                \
        }                                                            \
    } while (0)

int
main(void)
{
    float       f = 1.0f;
    double      d = 1.0;
    long double l = 1.0L;

    check(fpclassify(f) == FP_NORMAL);
    check(fpclassify(d) == FP_NORMAL);
    check(fpclassify(l) == FP_NORMAL);
    check(fpclassify(0.0f) == FP_ZERO);
    check(isfinite(f) && isfinite(d) && isfinite(l));
    check(isnormal(f) && isnormal(d) && isnormal(l));
    check(!signbit(d) && signbit(-d));
    check(signbit(-f) && signbit(-l));

    check(iszero(0.0f) && iszero(0.0) && iszero(0.0L));
    check(!issubnormal(f) && !issubnormal(d) && !issubnormal(l));
    check(!issignaling(f) && !issignaling(d) && !issignaling(l));
    check(iscanonical(f) && iscanonical(d) && iscanonical(l));

    /* Mixed arguments compare in the wider type */
    check(iseqsig(f, d));
    check(iseqsig(d, l));
#if __DBL_MANT_DIG__ > __FLT_MANT_DIG__
    check(!iseqsig(16777217.0, 16777216.0f));
#endif

#ifdef __FLT16_MANT_DIG__
    {
        __extension__ _Float16 h = 1;

        check(fpclassify(h) == FP_NORMAL);
        check(isnormal(h));
        check(!iszero(h));
        check(iseqsig(h, f));
    }
#endif

    return errors != 0;
}
