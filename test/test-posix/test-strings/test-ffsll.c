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
/* ffs, ffsl and ffsll are XSI */
#define _XOPEN_SOURCE 800
/* glibc declares ffsl and ffsll only with _DEFAULT_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <limits.h>
#include <stdio.h>
#include <strings.h>

/* ffs, ffsl and ffsll: the index of the lowest set bit, counting from 1 */

#define proto(f, type) _Static_assert(_Generic(&(f), type: 1, default: 0), #f " prototype")

proto(ffs, int (*)(int));
proto(ffsl, int (*)(long));
proto(ffsll, int (*)(long long));

static int (* volatile p_ffs)(int) = ffs;
static int (* volatile p_ffsl)(long) = ffsl;
static int (* volatile p_ffsll)(long long) = ffsll;

static int ret;

static void
expect(const char *name, unsigned long long value, int got, int want)
{
    if (got != want) {
        printf("%s(%#llx) is %d, not %d\n", name, value, got, want);
        ret = 1;
    }
}

int
main(void)
{
    unsigned bit, low;

    expect("ffs", 0, p_ffs(0), 0);
    expect("ffsl", 0, p_ffsl(0), 0);
    expect("ffsll", 0, p_ffsll(0), 0);
    expect("ffs", 0, (ffs)(0), 0);
    expect("ffsl", 0, (ffsl)(0), 0);
    expect("ffsll", 0, (ffsll)(0), 0);

    /* Every lowest bit, with every pattern of bits above it */
    for (bit = 0; bit < sizeof(long long) * CHAR_BIT; bit++) {
        unsigned long long one = 1ULL << bit;
        unsigned long long all = ~0ULL << bit;
        unsigned long long some = (0x5a5a5a5a5a5a5a5aULL << bit) | one;

        low = bit + 1;
        if (bit < sizeof(int) * CHAR_BIT) {
            /* Values above INT_MAX convert to negative ints */
            expect("ffs", one, p_ffs((int)(unsigned)one), (int)low);
            expect("ffs", all, p_ffs((int)(unsigned)all), (int)low);
            expect("ffs", some, p_ffs((int)(unsigned)some), (int)low);
        }
        if (bit < sizeof(long) * CHAR_BIT) {
            expect("ffsl", one, p_ffsl((long)(unsigned long)one), (int)low);
            expect("ffsl", all, p_ffsl((long)(unsigned long)all), (int)low);
            expect("ffsl", some, p_ffsl((long)(unsigned long)some), (int)low);
        }
        expect("ffsll", one, p_ffsll((long long)one), (int)low);
        expect("ffsll", all, p_ffsll((long long)all), (int)low);
        expect("ffsll", some, p_ffsll((long long)some), (int)low);
    }

    /* The extremes */
    expect("ffs", (unsigned)INT_MIN, p_ffs(INT_MIN), (int)(sizeof(int) * CHAR_BIT));
    expect("ffs", -1, p_ffs(-1), 1);
    expect("ffsl", (unsigned long)LONG_MIN, p_ffsl(LONG_MIN), (int)(sizeof(long) * CHAR_BIT));
    expect("ffsll", (unsigned long long)LLONG_MIN, p_ffsll(LLONG_MIN),
           (int)(sizeof(long long) * CHAR_BIT));
    expect("ffsll", 1ULL << 32, p_ffsll(1LL << 32), 33);

    if (ret == 0)
        printf("success\n");
    return ret;
}
