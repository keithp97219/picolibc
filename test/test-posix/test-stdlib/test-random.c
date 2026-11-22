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
/* random, srandom, initstate and setstate are XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/*
 * random and rand: the default sequences are those of seed 1 (for
 * random, of initstate(1, state, 128)), the sizes initstate rounds to,
 * switching state arrays, the ranges, and rand's sequence staying
 * apart from random's.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define N 16

static void
sequence(long *v, int n)
{
    int i;

    for (i = 0; i < n; i++)
        v[i] = random();
}

/* The sequences from initstate(seed) with two sizes are the same */
static void
same_size(unsigned seed, size_t s1, size_t s2, const char *message)
{
    static char b1[512], b2[512];
    long        v1[N], v2[N];

    check(initstate(seed, b1, s1) != NULL, message);
    sequence(v1, N);
    check(initstate(seed, b2, s2) != NULL, message);
    sequence(v2, N);
    check(memcmp(v1, v2, sizeof(v1)) == 0, message);
}

#if TEST_POSIX_MULTI_PROCESS
/* In a fresh child, rand's default sequence is that of srand(1) */
static void
rand_default(void)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int r[N], i;

        for (i = 0; i < N; i++)
            r[i] = rand();
        srand(1);
        for (i = 0; i < N; i++)
            check(rand() == r[i], "rand's default sequence is srand(1)'s");
        fflush(stdout);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "rand_default");
}
#endif

int
main(void)
{
    long  d[N], v[N], w[N];
    char  state[128], other[128];
    char  raw[260];
    char *prev;
    int   r[N];
    int   i;

#if TEST_POSIX_MULTI_PROCESS
    /* Before anything here calls rand */
    rand_default();
#endif

    /* Before any initialization */
    sequence(d, N);

    srandom(1);
    sequence(v, N);
    check(memcmp(d, v, sizeof(d)) == 0, "the default sequence is srandom(1)'s");

    prev = initstate(1, state, sizeof(state));
    check(prev != NULL, "initstate returns the previous state");
    sequence(v, N);
    check(memcmp(d, v, sizeof(d)) == 0, "the default is initstate(1, state, 128)");

    /* The range */
    for (i = 0; i < 10000; i++) {
        long x = random();
        check(x >= 0 && x <= 0x7fffffffL, "random's range");
    }

    /* Sizes round down to 8, 32, 64, 128 or 256 */
    same_size(5, 8, 31, "8 to 31 bytes");
    same_size(5, 32, 63, "32 to 63 bytes");
    same_size(5, 64, 100, "64 to 127 bytes");
    same_size(5, 128, 255, "128 to 255 bytes");
    same_size(5, 256, 500, "256 bytes and more");

    /* Too small */
    check(initstate(1, raw, 7) == NULL, "initstate with less than 8 bytes");

    /* srandom reseeds whichever array is in use */
    {
        static char small[8];

        check(initstate(5, small, sizeof(small)) != NULL, "initstate with 8 bytes");
        sequence(v, N);
        srandom(5);
        sequence(w, N);
        check(memcmp(v, w, sizeof(v)) == 0, "srandom on an 8-byte state");
    }

    /* The state array needn't be aligned */
    check(initstate(3, raw + 1, 128) != NULL, "initstate of an unaligned array");
    sequence(v, N);
    check(initstate(3, state, 128) != NULL, "initstate of an aligned array");
    sequence(w, N);
    check(memcmp(v, w, sizeof(v)) == 0, "unaligned arrays work the same");

    /* setstate switches arrays, and each keeps its position */
    check(initstate(9, state, sizeof(state)) != NULL, "initstate A");
    v[0] = random();
    prev = initstate(9, other, sizeof(other));
    check(prev == state, "initstate returns the previous array");
    w[0] = random();
    w[1] = random();
    check(w[0] == v[0], "same seed, same start");
    check(setstate(state) == other, "setstate returns the previous array");
    check(random() == w[1], "the first array resumes where it was");
    check(setstate(prev) == state, "setstate back");

    /* rand: same seed, same sequence, in range */
    srand(77);
    for (i = 0; i < N; i++) {
        r[i] = rand();
        check(r[i] >= 0 && r[i] <= RAND_MAX, "rand's range");
    }
    srand(77);
    for (i = 0; i < N; i++)
        check(rand() == r[i], "srand repeats the sequence");

#ifndef __GLIBC__
    /*
     * POSIX: as if no function calls rand or srand. glibc's rand is
     * random, so srandom and random move it there.
     */
    srand(77);
    srandom(12);
    (void)random();
    check(initstate(4, other, sizeof(other)) != NULL, "initstate");
    (void)random();
    for (i = 0; i < N; i++)
        check(rand() == r[i], "random and srandom leave rand's sequence alone");
#endif

    printf("success\n");
    return 0;
}
