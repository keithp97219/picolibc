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
#include <stdlib.h>

/* isgreater, isgreaterequal, isless, islessequal, islessgreater and isunordered */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    /* Not constants, so the compiler doesn't evaluate them itself */
    volatile double      one = 1.0, two = 2.0, qnan = NAN, inf = INFINITY;
    volatile float       onef = 1.0f, qnanf = NAN;
    volatile long double onel = 1.0L, twol = 2.0L;

    check(isgreater(two, one) && !isgreater(one, two) && !isgreater(one, one), "isgreater");
    check(isgreaterequal(two, one) && isgreaterequal(one, one) && !isgreaterequal(one, two),
          "isgreaterequal");
    check(isless(one, two) && !isless(two, one) && !isless(one, one), "isless");
    check(islessequal(one, two) && islessequal(one, one) && !islessequal(two, one), "islessequal");
    check(islessgreater(one, two) && islessgreater(two, one) && !islessgreater(one, one),
          "islessgreater");
    check(!isunordered(one, two) && !isunordered(inf, -inf), "isunordered of numbers");
    check(isgreater(inf, two) && isless(-inf, one), "infinities");

    /* A NaN is unordered: every comparison is false */
    check(isunordered(qnan, one) && isunordered(one, qnan) && isunordered(qnan, qnan),
          "isunordered NaN");
    check(!isgreater(qnan, one) && !isgreaterequal(qnan, one) && !isless(qnan, one)
              && !islessequal(qnan, one) && !islessgreater(qnan, one),
          "comparisons with a NaN");

    /* Other types, and mixed ones */
    check(isless(onef, two) && isunordered(qnanf, onef), "float");
    check(isless(onel, twol) && isgreater(twol, one), "long double");

    printf("success\n");
    return 0;
}
