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

#include <stdio.h>

/*
 * Strict POSIX: <math.h> declares neither the XSI interfaces nor names
 * POSIX doesn't give it. Each static object here collides with a
 * declaration of the same name.
 */

/* The XSI constants use the M_ prefix, reserved only for XSI */
#if defined(M_E) || defined(M_PI) || defined(M_PI_2) || defined(M_SQRT2) || defined(M_LN2) \
    || defined(M_PIl) || defined(M_EGAMMA)
#error "an XSI M_ constant in strict POSIX mode"
#endif

#ifdef MAXFLOAT
#error "MAXFLOAT in strict POSIX mode"
#endif

/* XSI functions and signgam */
static int j0 __attribute__((unused));
static int j1 __attribute__((unused));
static int jn __attribute__((unused));
static int y0 __attribute__((unused));
static int y1 __attribute__((unused));
static int yn __attribute__((unused));
static int signgam __attribute__((unused));

/* Extensions and interfaces POSIX removed */
static int j0f __attribute__((unused));
static int ynf __attribute__((unused));
static int finite __attribute__((unused));
static int finitef __attribute__((unused));
static int drem __attribute__((unused));
static int gamma __attribute__((unused));
static int lgamma_r __attribute__((unused));
static int significand __attribute__((unused));
static int infinity __attribute__((unused));
static int scalb __attribute__((unused));
static int pow10 __attribute__((unused));
static int sincos __attribute__((unused));

#if defined(isinff) || defined(isnanf) || defined(isfinitef)
#error "a classification extension in strict POSIX mode"
#endif

/* C23 adds these; POSIX.1-2024 is aligned with C17 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
static int exp10 __attribute__((unused));
static int sinpi __attribute__((unused));
#if defined(iszero) || defined(issignaling) || defined(FP_INT_UPWARD)
#error "a C23 name in strict C17 POSIX mode"
#endif
#endif

int
main(void)
{
    printf("success\n");
    return 0;
}
