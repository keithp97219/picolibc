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
/* glibc declares pthread_rwlock_clockwrlock and pthread_rwlock_clockrdlock only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_READER_WRITER_LOCKS > 0 && _POSIX_THREADS > 0 && _POSIX_TIMEOUTS > 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Reader-writer locks */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NTHREADS 8
#define NLOOPS   20000

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static long             value_a, value_b;
static int              readers_inside, max_readers_inside;

static void *
worker(void *arg)
{
    intptr_t id = (intptr_t)arg;

    for (int i = 0; i < NLOOPS; i++) {
        if ((i + id) % 8 == 0) {
            pthread_rwlock_wrlock(&rwlock);
            value_a++;
            value_b++;
            pthread_rwlock_unlock(&rwlock);
        } else {
            int inside;

            pthread_rwlock_rdlock(&rwlock);
            inside = __atomic_add_fetch(&readers_inside, 1, __ATOMIC_RELAXED);
            if (inside > __atomic_load_n(&max_readers_inside, __ATOMIC_RELAXED))
                __atomic_store_n(&max_readers_inside, inside, __ATOMIC_RELAXED);
            if (value_a != value_b)
                return (void *)1;
            __atomic_sub_fetch(&readers_inside, 1, __ATOMIC_RELAXED);
            pthread_rwlock_unlock(&rwlock);
        }
    }
    return NULL;
}

static void
timeout_in(clockid_t clock, struct timespec *ts, long ms)
{
    clock_gettime(clock, ts);
    ts->tv_nsec += ms * 1000 * 1000;
    while (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec++;
    }
}

static void *
try_locks(void *arg)
{
    struct timespec ts;
    intptr_t        held = (intptr_t)arg; /* 1 read locked, 2 write locked */

    if (pthread_rwlock_trywrlock(&rwlock) != EBUSY)
        return (void *)1;
    timeout_in(CLOCK_REALTIME, &ts, 20);
    if (pthread_rwlock_timedwrlock(&rwlock, &ts) != ETIMEDOUT)
        return (void *)2;
    timeout_in(CLOCK_MONOTONIC, &ts, 20);
    if (pthread_rwlock_clockwrlock(&rwlock, CLOCK_MONOTONIC, &ts) != ETIMEDOUT)
        return (void *)3;
    if (held == 1) {
        /* Readers share the lock */
        if (pthread_rwlock_tryrdlock(&rwlock) != 0)
            return (void *)4;
        pthread_rwlock_unlock(&rwlock);
        timeout_in(CLOCK_REALTIME, &ts, 20);
        if (pthread_rwlock_timedrdlock(&rwlock, &ts) != 0)
            return (void *)5;
        pthread_rwlock_unlock(&rwlock);
    } else {
        if (pthread_rwlock_tryrdlock(&rwlock) != EBUSY)
            return (void *)6;
        timeout_in(CLOCK_REALTIME, &ts, 20);
        if (pthread_rwlock_timedrdlock(&rwlock, &ts) != ETIMEDOUT)
            return (void *)7;
        timeout_in(CLOCK_MONOTONIC, &ts, 20);
        if (pthread_rwlock_clockrdlock(&rwlock, CLOCK_MONOTONIC, &ts) != ETIMEDOUT)
            return (void *)8;
    }
    return NULL;
}

static intptr_t
in_thread(void *(*func)(void *), void *arg)
{
    pthread_t thread;
    void     *result;

    check(pthread_create(&thread, NULL, func, arg) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    return (intptr_t)result;
}

int
main(void)
{
    pthread_t            threads[NTHREADS];
    pthread_rwlockattr_t attr;
    pthread_rwlock_t     other;
    void                *result;
    int                  value;

    /* Attributes */
    check(pthread_rwlockattr_init(&attr) == 0, "pthread_rwlockattr_init");
    check(pthread_rwlockattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_PRIVATE,
          "default pshared");
    check(pthread_rwlockattr_setpshared(&attr, -1) == EINVAL, "bad pshared");
    check(pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) == 0, "setpshared");
    check(pthread_rwlockattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_SHARED,
          "getpshared");
    check(pthread_rwlock_init(&other, &attr) == 0, "pthread_rwlock_init");
    check(pthread_rwlockattr_destroy(&attr) == 0, "pthread_rwlockattr_destroy");
    check(pthread_rwlock_wrlock(&other) == 0, "wrlock shared");
    check(pthread_rwlock_unlock(&other) == 0, "unlock shared");
    check(pthread_rwlock_destroy(&other) == 0, "pthread_rwlock_destroy");

    /* Readers and writers exclude each other */
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, worker, (void *)(intptr_t)i) == 0,
              "pthread_create");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join");
        check(result == NULL, "writers excluded readers");
    }
    check(value_a == (long)NTHREADS * NLOOPS / 8, "writer count");

    /* A thread may hold several read locks */
    check(pthread_rwlock_rdlock(&rwlock) == 0, "rdlock 1");
    check(pthread_rwlock_rdlock(&rwlock) == 0, "rdlock 2");
    check(pthread_rwlock_tryrdlock(&rwlock) == 0, "tryrdlock");
    check(in_thread(try_locks, (void *)1) == 0, "locks while read locked");
    for (int i = 0; i < 3; i++)
        check(pthread_rwlock_unlock(&rwlock) == 0, "unlock read");

    check(pthread_rwlock_wrlock(&rwlock) == 0, "wrlock");
    check(in_thread(try_locks, (void *)2) == 0, "locks while write locked");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock write");

    check(pthread_rwlock_trywrlock(&rwlock) == 0, "trywrlock");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock trywrlock");
    check(pthread_rwlock_destroy(&rwlock) == 0, "pthread_rwlock_destroy");

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
