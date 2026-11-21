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
/* The rand48 family is XSI */
#define _XOPEN_SOURCE 800
#include <stdlib.h>

#include <stdint.h>
#include <stdio.h>

/*
 * The rand48 family against the congruence POSIX documents:
 * X(n+1) = (a * X(n) + c) mod 2^48, a = 0x5DEECE66D and c = 0xB unless
 * lcong48 sets them, with each function's conversion of X.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

#define MASK48 ((UINT64_C(1) << 48) - 1)

static uint64_t a = UINT64_C(0x5DEECE66D);
static uint64_t c = 0xB;

static uint64_t
step(uint64_t x)
{
    return (a * x + c) & MASK48;
}

static uint64_t
from_array(const unsigned short v[3])
{
    return (uint64_t)v[0] | (uint64_t)v[1] << 16 | (uint64_t)v[2] << 32;
}

static void
to_array(unsigned short v[3], uint64_t x)
{
    v[0] = (unsigned short)x;
    v[1] = (unsigned short)(x >> 16);
    v[2] = (unsigned short)(x >> 32);
}

/* The conversions */
static double
as_drand(uint64_t x)
{
    return (double)x / (double)(UINT64_C(1) << 48);
}

static long
as_lrand(uint64_t x)
{
    return (long)(x >> 17);
}

static long
as_mrand(uint64_t x)
{
    return (long)(int32_t)(uint32_t)(x >> 16);
}

/* Seeds near the top of the range carry through every 16-bit part */
static const uint64_t seeds[] = {
    0,
    1,
    UINT64_C(0x1234ABCD330E),
    UINT64_C(0xFFFFFFFFFFFF),
    UINT64_C(0xFFFF0000FFFF),
    UINT64_C(0x8000FFFF8000),
};

#define NSEEDS (sizeof(seeds) / sizeof(seeds[0]))

int
main(void)
{
    unsigned short  xs[3], ys[3];
    unsigned short *prev;
    uint64_t        x, y;
    double          d;
    long            l;
    size_t          s;
    int             i;

    /* Without an initializer, drand48 still works, from a constant state */
    d = drand48();
    check(d >= 0.0 && d < 1.0, "drand48 without initialization");

    for (s = 0; s < NSEEDS; s++) {
        /* erand48, nrand48 and jrand48 step the caller's array */
        x = seeds[s];
        to_array(xs, x);
        for (i = 0; i < 20; i++) {
            x = step(x);
            d = erand48(xs);
            check(from_array(xs) == x, "erand48 steps X");
            check(d == as_drand(x) && d >= 0.0 && d < 1.0, "erand48 is 2^-48 X");
        }
        for (i = 0; i < 20; i++) {
            x = step(x);
            l = nrand48(xs);
            check(from_array(xs) == x, "nrand48 steps X");
            check(l == as_lrand(x) && l >= 0 && (unsigned long)l <= 0x7fffffffUL,
                  "nrand48 is X >> 17");
        }
        for (i = 0; i < 20; i++) {
            x = step(x);
            l = jrand48(xs);
            check(from_array(xs) == x, "jrand48 steps X");
            check(l == as_mrand(x), "jrand48 is X >> 16, signed");
        }

        /* seed48 sets X and returns the previous one */
        to_array(xs, seeds[s]);
        prev = seed48(xs);
        check(prev != NULL, "seed48");
        x = seeds[s];
        for (i = 0; i < 10; i++) {
            x = step(x);
            check(drand48() == as_drand(x), "drand48 after seed48");
            x = step(x);
            check(lrand48() == as_lrand(x), "lrand48 after seed48");
            x = step(x);
            check(mrand48() == as_mrand(x), "mrand48 after seed48");
        }
        to_array(ys, UINT64_C(0x5555AAAA5555));
        prev = seed48(ys);
        check(from_array(prev) == x, "seed48 returns the last X");
    }

    /* srand48: the high 32 bits from the seed, the low 16 bits 0x330E */
    srand48(0x12345678L);
    x = UINT64_C(0x12345678330E);
    check(lrand48() == as_lrand(step(x)), "srand48");
    srand48((long)0xFEDCBA98UL);
    x = UINT64_C(0xFEDCBA98330E);
    for (i = 0; i < 10; i++) {
        x = step(x);
        check(mrand48() == as_mrand(x), "srand48 of a large seed");
    }

    /* lcong48 sets X, a and c, for every function */
    {
        unsigned short param[7] = { 0x1111, 0x2222, 0x3333, 0xFFFF, 0xFFFF, 0xFFFF, 0x7777 };

        lcong48(param);
        a = MASK48;
        c = 0x7777;
        x = UINT64_C(0x333322221111);
        for (i = 0; i < 10; i++) {
            x = step(x);
            check(lrand48() == as_lrand(x), "lrand48 after lcong48");
        }
        y = UINT64_C(0xFFFFFFFFFFFF);
        to_array(ys, y);
        y = step(y);
        check(nrand48(ys) == as_lrand(y), "nrand48 uses lcong48's a and c");

        /* srand48 and seed48 restore the standard a and c */
        a = UINT64_C(0x5DEECE66D);
        c = 0xB;
        srand48(1);
        check(lrand48() == as_lrand(step(UINT64_C(0x00000001330E))), "srand48 after lcong48");
        lcong48(param);
        to_array(xs, 42);
        (void)seed48(xs);
        check(lrand48() == as_lrand(step(42)), "seed48 after lcong48");
    }

    /* Separate arrays are separate streams */
    to_array(xs, 1000);
    to_array(ys, 1000);
    (void)nrand48(xs);
    (void)nrand48(xs);
    (void)drand48();
    x = step(1000);
    check(nrand48(ys) == as_lrand(x), "one array doesn't depend on another");

    printf("success\n");
    return 0;
}
