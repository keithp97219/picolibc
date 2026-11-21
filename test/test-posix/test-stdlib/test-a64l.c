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
/* a64l and l64a are XSI */
#define _XOPEN_SOURCE 800
#include <stdlib.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * a64l and l64a: radix-64 digits, least significant first, at most six
 * of them, '.' for 0, '/' for 1, '0'-'9', 'A'-'Z' and 'a'-'z' for the
 * rest. Only the low 32 bits of a long count, and a64l sign-extends.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

static const char digits[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int
main(void)
{
    char        one[2] = { 0, 0 };
    const char *s;
    long        v;
    long long   vv;
    int         i;

    /* Each digit */
    for (i = 1; i < 64; i++) {
        one[0] = digits[i];
        check(a64l(one) == i, "a64l of one digit");
        s = l64a(i);
        check(s != NULL && strcmp(s, one) == 0, "l64a of one digit");
    }

    /* Zero is the empty string, both ways */
    s = l64a(0);
    check(s != NULL && s[0] == '\0', "l64a(0)");
    check(a64l("") == 0, "a64l of an empty string");

    /* The first digit is the least significant */
    check(strcmp(l64a(64), "./") == 0, "l64a(64)");
    check(a64l("./") == 64, "a64l(\"./\")");
    check(a64l("/.") == 1, "a64l(\"/.\")");
    check(strcmp(l64a(65), "//") == 0, "l64a(65)");

    /* Round trips */
    /* long long, so the steps can't overflow a 32-bit long */
    for (vv = 1; vv <= 0x7fffffffLL; vv = vv * 3 + 1) {
        v = (long)vv;
        s = l64a(v);
        check(s != NULL && strlen(s) <= 6, "l64a uses at most six digits");
        check(a64l(s) == v, "round trip");
    }
    check(a64l(l64a(0x7fffffffL)) == 0x7fffffffL, "round trip of 2^31 - 1");

    /* Only the first six characters count */
    check(a64l("zzzzzz") == a64l("zzzzzzzzzz"), "a64l uses the first six characters");
    check(a64l("A./") == a64l("A./\0zz"), "a64l stops at the null");

#ifndef __GLIBC__
    /*
     * The low 32 bits, sign-extended: six z's are 2^36 - 1, so -1.
     * glibc returns all 36 bits.
     */
    check(a64l("zzzzzz") == -1L, "a64l sign-extends the low 32 bits");
    check(a64l(".....0") == (long)INT32_MIN, "a64l of 2^31");
#endif

#if LONG_MAX > 0x7fffffffL
    /* l64a uses only the low 32 bits of a wider long */
    check(strcmp(l64a(0x100000041L), l64a(0x41L)) == 0, "l64a uses the low 32 bits");
#endif

    printf("success\n");
    return 0;
}
