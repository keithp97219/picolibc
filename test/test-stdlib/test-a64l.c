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

#define _DEFAULT_SOURCE
#include <stdlib.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * a64l: only the low 32 bits of the value count, sign-extended to
 * long. Six digits hold 36 bits, so on LP64 targets the upper ones
 * must be dropped.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

int
main(void)
{
    long long vv;
    long      v;

    check(a64l("") == 0, "a64l of an empty string");
    check(a64l("./") == 64, "a64l(\"./\")");
    check(a64l("zzzzzz") == a64l("zzzzzzzzzz"), "a64l uses the first six characters");

    /* Round trips; long long, so the steps can't overflow a 32-bit long */
    for (vv = 1; vv <= 0x7fffffffLL; vv = vv * 3 + 1) {
        v = (long)vv;
        check(a64l(l64a(v)) == v, "round trip");
    }

#ifndef __GLIBC__
    /*
     * The low 32 bits, sign-extended: six z's are 2^36 - 1, so -1.
     * glibc returns all 36 bits.
     */
    check(a64l("zzzzzz") == -1L, "a64l sign-extends the low 32 bits");
    check(a64l(".....0") == (long)INT32_MIN, "a64l of 2^31");
#endif

    printf("success\n");
    return 0;
}
