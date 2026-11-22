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
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_SPIN_LOCKS && TEST_POSIX_THREADS

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NTHREADS 6
#define NLOOPS   20000

/* Spin locks protect a counter, and trylock fails while one is held */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_spinlock_t spin;
static long               counter;

static void *
spin_count(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOPS; i++) {
        pthread_spin_lock(&spin);
        counter++;
        pthread_spin_unlock(&spin);
    }
    return NULL;
}

static void *
spin_try(void *arg)
{
    (void)arg;
    return (void *)(intptr_t)pthread_spin_trylock(&spin);
}

int
main(void)
{
    pthread_t threads[NTHREADS];
    void     *result;

    check(pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE) == 0, "pthread_spin_init");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, spin_count, NULL) == 0, "pthread_create spin");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join spin");
    check(counter == (long)NTHREADS * NLOOPS, "spin protected counter");

    check(pthread_spin_lock(&spin) == 0, "pthread_spin_lock");
    {
        pthread_t thread;
        check(pthread_create(&thread, NULL, spin_try, NULL) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == (void *)(intptr_t)EBUSY, "trylock held spin lock");
    }
    check(pthread_spin_unlock(&spin) == 0, "pthread_spin_unlock");
    check(pthread_spin_trylock(&spin) == 0, "pthread_spin_trylock");
    check(pthread_spin_unlock(&spin) == 0, "pthread_spin_unlock");
    check(pthread_spin_destroy(&spin) == 0, "pthread_spin_destroy");

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
