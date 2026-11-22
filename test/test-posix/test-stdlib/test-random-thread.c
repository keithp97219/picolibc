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
/* random and srandom are XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * random, srandom, initstate and setstate must be thread-safe, with one
 * sequence for the process: a thread continues what another seeded,
 * and threads drawing at once share out the values of one sequence.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define N        8
#define NTHREADS 4
#define DRAWS    4000

static long got[N];
static long drawn[NTHREADS][DRAWS];

static void *
draw_some(void *arg)
{
    (void)arg;
    for (int i = 0; i < N; i++)
        got[i] = random();
    return NULL;
}

static void *
draw_many(void *arg)
{
    long *out = arg;

    for (int i = 0; i < DRAWS; i++)
        out[i] = random();
    return NULL;
}

static int
cmp_long(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;

    return (x > y) - (x < y);
}

int
main(void)
{
    static long all[NTHREADS * DRAWS], expect[NTHREADS * DRAWS];
    static char state[256];
    long        want[N];
    pthread_t   t[NTHREADS];
    int         i;

    /* Seeded in this thread, continued in another */
    srandom(5);
    for (i = 0; i < N; i++)
        want[i] = random();
    srandom(5);
    check(pthread_create(&t[0], NULL, draw_some, NULL) == 0, "pthread_create");
    check(pthread_join(t[0], NULL) == 0, "pthread_join");
    check(memcmp(want, got, sizeof(want)) == 0, "a thread uses the process's seed");

    /* The same for an initstate array */
    check(initstate(6, state, sizeof(state)) != NULL, "initstate");
    for (i = 0; i < N; i++)
        want[i] = random();
    srandom(6);
    check(pthread_create(&t[0], NULL, draw_some, NULL) == 0, "pthread_create");
    check(pthread_join(t[0], NULL) == 0, "pthread_join");
    check(memcmp(want, got, sizeof(want)) == 0, "a thread uses the initstate array");

    /* Threads drawing at once get the values of one sequence between them */
    srandom(7);
    for (i = 0; i < NTHREADS * DRAWS; i++)
        expect[i] = random();
    srandom(7);
    for (i = 0; i < NTHREADS; i++)
        check(pthread_create(&t[i], NULL, draw_many, drawn[i]) == 0, "pthread_create");
    for (i = 0; i < NTHREADS; i++)
        check(pthread_join(t[i], NULL) == 0, "pthread_join");
    memcpy(all, drawn, sizeof(all));
    qsort(all, NTHREADS * DRAWS, sizeof(long), cmp_long);
    qsort(expect, NTHREADS * DRAWS, sizeof(long), cmp_long);
    check(memcmp(all, expect, sizeof(all)) == 0, "concurrent random");

    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
