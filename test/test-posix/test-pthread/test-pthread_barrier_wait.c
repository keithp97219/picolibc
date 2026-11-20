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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                  \
    || ((defined(_POSIX_BARRIERS) && _POSIX_BARRIERS > 0)   \
        && (defined(_POSIX_THREADS) && _POSIX_THREADS > 0))

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NTHREADS 6
#define NROUNDS  1000

/* Barrier attributes, and rounds where every thread arrives before any leaves */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_barrier_t barrier;
static int               arrived[NROUNDS];
static int               serial[NROUNDS];

static void *
rounds(void *arg)
{
    (void)arg;
    for (int r = 0; r < NROUNDS; r++) {
        int ret;

        __atomic_add_fetch(&arrived[r], 1, __ATOMIC_RELAXED);
        ret = pthread_barrier_wait(&barrier);
        if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
            __atomic_add_fetch(&serial[r], 1, __ATOMIC_RELAXED);
        else if (ret != 0)
            return (void *)1;
        /* Everybody has arrived for this round */
        if (__atomic_load_n(&arrived[r], __ATOMIC_RELAXED) != NTHREADS)
            return (void *)2;
    }
    return NULL;
}

int
main(void)
{
    pthread_t             threads[NTHREADS];
    pthread_barrierattr_t attr;
    void                 *result;
    int                   value;

    /* Attributes */
    check(pthread_barrierattr_init(&attr) == 0, "pthread_barrierattr_init");
    check(pthread_barrierattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_PRIVATE,
          "default pshared");
    check(pthread_barrierattr_setpshared(&attr, -1) == EINVAL, "bad pshared");
    check(pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) == 0, "setpshared");
    check(pthread_barrierattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_SHARED,
          "getpshared");
    check(pthread_barrier_init(&barrier, &attr, 0) == EINVAL, "zero count");
    check(pthread_barrier_init(&barrier, &attr, 1) == 0, "pthread_barrier_init 1");
    check(pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD, "single thread barrier");
    check(pthread_barrier_destroy(&barrier) == 0, "pthread_barrier_destroy");
    check(pthread_barrierattr_destroy(&attr) == 0, "pthread_barrierattr_destroy");

    /* Repeated rounds; each has one serial thread */
    check(pthread_barrier_init(&barrier, NULL, NTHREADS) == 0, "pthread_barrier_init");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, rounds, NULL) == 0, "pthread_create");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join");
        check(result == NULL, "barrier rounds");
    }
    for (int r = 0; r < NROUNDS; r++)
        check(serial[r] == 1, "one serial thread per round");
    check(pthread_barrier_destroy(&barrier) == 0, "pthread_barrier_destroy");

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
