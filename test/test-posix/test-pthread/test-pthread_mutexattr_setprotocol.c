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
/* glibc declares pthread_mutex_clocklock only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS && TEST_POSIX_THREAD_PRIO_INHERIT && TEST_POSIX_THREAD_PRIO_PROTECT \
    && TEST_POSIX_TIMEOUTS

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Mutex priority inheritance and priority protection */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NTHREADS 8
#define NLOOPS   20000

static pthread_mutex_t mutex;
static long            counter;

static void *
count(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOPS; i++) {
        if (pthread_mutex_lock(&mutex) != 0)
            return (void *)1;
        counter++;
        if (pthread_mutex_unlock(&mutex) != 0)
            return (void *)2;
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
contend(void *arg)
{
    struct timespec ts;
#if TEST_POSIX_THREADS >= 202405L
    int ret;
#endif

    (void)arg;
    if (pthread_mutex_trylock(&mutex) != EBUSY)
        return (void *)1;
    timeout_in(CLOCK_REALTIME, &ts, 20);
    if (pthread_mutex_timedlock(&mutex, &ts) != ETIMEDOUT)
        return (void *)2;
    /* glibc reports EINVAL here on kernels without FUTEX_LOCK_PI2 */
#if TEST_POSIX_THREADS >= 202405L
    timeout_in(CLOCK_MONOTONIC, &ts, 20);
    ret = pthread_mutex_clocklock(&mutex, CLOCK_MONOTONIC, &ts);
    if (ret != ETIMEDOUT && ret != EINVAL)
        return (void *)3;
#endif
    if (pthread_mutex_unlock(&mutex) != EPERM)
        return (void *)4;
    return NULL;
}

static void
run_threads(void *(*func)(void *), int n)
{
    pthread_t threads[NTHREADS];
    void     *result;

    for (int i = 0; i < n; i++)
        check(pthread_create(&threads[i], NULL, func, NULL) == 0, "pthread_create");
    for (int i = 0; i < n; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join");
        if (result != NULL)
            printf("thread result %ld\n", (long)(intptr_t)result);
        check(result == NULL, "thread result");
    }
}

static void
init_mutex(int type, int protocol)
{
    pthread_mutexattr_t attr;

    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&attr, type) == 0, "settype");
    check(pthread_mutexattr_setprotocol(&attr, protocol) == 0, "setprotocol");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
}

int
main(void)
{
    pthread_mutexattr_t attr;
    pthread_cond_t      cond = PTHREAD_COND_INITIALIZER;
    struct timespec     ts;
    int                 value, old, ret;

    /* Attributes */
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_getprotocol(&attr, &value) == 0 && value == PTHREAD_PRIO_NONE,
          "default protocol");
    check(pthread_mutexattr_setprotocol(&attr, -1) == EINVAL, "bad protocol");
    check(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT) == 0, "setprotocol");
    check(pthread_mutexattr_getprotocol(&attr, &value) == 0 && value == PTHREAD_PRIO_PROTECT,
          "getprotocol");
    check(pthread_mutexattr_setprioceiling(&attr, -1) == EINVAL, "bad ceiling");
    check(pthread_mutexattr_setprioceiling(&attr, 10) == 0, "setprioceiling");
    check(pthread_mutexattr_getprioceiling(&attr, &value) == 0 && value == 10, "getprioceiling");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");

    {
        pthread_mutex_t none = PTHREAD_MUTEX_INITIALIZER;
        check(pthread_mutex_getprioceiling(&none, &value) == EINVAL, "ceiling of PRIO_NONE mutex");
    }

    /* Priority inheritance mutexes of each type */
    init_mutex(PTHREAD_MUTEX_NORMAL, PTHREAD_PRIO_INHERIT);
    run_threads(count, NTHREADS);
    check(counter == (long)NTHREADS * NLOOPS, "priority inheritance counter");
    check(pthread_mutex_destroy(&mutex) == 0, "pthread_mutex_destroy");

    init_mutex(PTHREAD_MUTEX_ERRORCHECK, PTHREAD_PRIO_INHERIT);
    check(pthread_mutex_unlock(&mutex) == EPERM, "unlock free errorcheck PI mutex");
    check(pthread_mutex_lock(&mutex) == 0, "lock errorcheck PI");
    check(pthread_mutex_lock(&mutex) == EDEADLK, "relock errorcheck PI");
    run_threads(contend, 1);
    check(pthread_mutex_unlock(&mutex) == 0, "unlock errorcheck PI");
    check(pthread_mutex_destroy(&mutex) == 0, "pthread_mutex_destroy");

    init_mutex(PTHREAD_MUTEX_RECURSIVE, PTHREAD_PRIO_INHERIT);
    check(pthread_mutex_lock(&mutex) == 0, "lock recursive PI 1");
    check(pthread_mutex_lock(&mutex) == 0, "lock recursive PI 2");
    check(pthread_mutex_trylock(&mutex) == 0, "trylock recursive PI");
    run_threads(contend, 1);
    for (int i = 0; i < 3; i++)
        check(pthread_mutex_unlock(&mutex) == 0, "unlock recursive PI");
    check(pthread_mutex_unlock(&mutex) == EPERM, "recursive PI released");

    /* Condition variables work with priority inheritance mutexes */
    check(pthread_mutex_lock(&mutex) == 0, "lock for cond");
    timeout_in(CLOCK_REALTIME, &ts, 10);
    check(pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT, "cond wait with PI mutex");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock after cond");
    check(pthread_mutex_destroy(&mutex) == 0, "pthread_mutex_destroy");

    /* Priority ceilings */
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT) == 0, "setprotocol");
    check(pthread_mutexattr_setprioceiling(&attr, 20) == 0, "setprioceiling");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
    check(pthread_mutex_getprioceiling(&mutex, &value) == 0 && value == 20,
          "pthread_mutex_getprioceiling");
    check(pthread_mutex_setprioceiling(&mutex, -1, &old) == EINVAL, "bad mutex ceiling");
    check(pthread_mutex_setprioceiling(&mutex, 30, &old) == 0 && old == 20,
          "pthread_mutex_setprioceiling");
    check(pthread_mutex_getprioceiling(&mutex, &value) == 0 && value == 30, "new ceiling");

    /*
     * Locking raises real-time threads to the ceiling. Some systems
     * refuse priority protection for non-real-time threads.
     */
    ret = pthread_mutex_lock(&mutex);
    check(ret == 0 || ret == EINVAL, "lock priority protected mutex");
    if (ret == 0) {
        check(pthread_mutex_unlock(&mutex) == 0, "unlock priority protected mutex");
        counter = 0;
        run_threads(count, NTHREADS);
        check(counter == (long)NTHREADS * NLOOPS, "priority protection counter");
    }
    check(pthread_mutex_destroy(&mutex) == 0, "pthread_mutex_destroy");

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
