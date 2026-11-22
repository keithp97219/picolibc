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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* abs, labs, llabs, div, ldiv and lldiv: quotients truncate toward zero */

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
    div_t              d;
    ldiv_t             ld;
    lldiv_t            lld;
    /* Not constants, so the compiler calls the functions */
    volatile int       i = -7, j = 2;
    volatile long      l = -7, m = 2;
    volatile long long ll = -7, mm = 2;

    check(abs(i) == 7 && abs(-i) == 7 && abs(0) == 0, "abs");
    check(abs(INT_MAX) == INT_MAX && abs(-INT_MAX) == INT_MAX, "abs at the limits");
    check(labs(l) == 7 && labs(LONG_MAX) == LONG_MAX && labs(-LONG_MAX) == LONG_MAX, "labs");
    check(llabs(ll) == 7 && llabs(LLONG_MAX) == LLONG_MAX && llabs(-LLONG_MAX) == LLONG_MAX,
          "llabs");

    d = div(i, j);
    check(d.quot == -3 && d.rem == -1, "div truncates");
    d = div(-i, j);
    check(d.quot == 3 && d.rem == 1, "div positive");
    d = div(-i, -j);
    check(d.quot == -3 && d.rem == 1, "div by a negative divisor");
    d = div(INT_MIN, 1);
    check(d.quot == INT_MIN && d.rem == 0, "div of INT_MIN");

    ld = ldiv(l, m);
    check(ld.quot == -3 && ld.rem == -1, "ldiv truncates");
    ld = ldiv(LONG_MAX, 2);
    check(ld.quot == LONG_MAX / 2 && ld.rem == 1, "ldiv of LONG_MAX");

    lld = lldiv(ll, mm);
    check(lld.quot == -3 && lld.rem == -1, "lldiv truncates");
    lld = lldiv(LLONG_MIN, -LLONG_MAX);
    check(lld.quot == 1 && lld.rem == -1, "lldiv of LLONG_MIN");

    printf("success\n");
    return 0;
}
