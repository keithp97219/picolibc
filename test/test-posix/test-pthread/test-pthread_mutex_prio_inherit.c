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
#if TEST_POSIX_THREADS && TEST_POSIX_THREAD_PRIO_INHERIT && TEST_POSIX_THREAD_PRIORITY_SCHEDULING

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Priority inheritance at work. A low priority thread holds a
 * PTHREAD_PRIO_INHERIT mutex while medium priority threads spin on
 * every processor, so it can't run. A high priority thread then
 * waits for the mutex: the holder must run at the waiter's priority,
 * above the spinners, and release the mutex at once. Without
 * inheritance the waiter would wait until the spinners give up, after
 * SPIN_SECONDS. This needs SCHED_FIFO, which only a privileged
 * process gets; otherwise the test is skipped.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SPIN_SECONDS 2
#define MAX_SPINNERS 1024
/* Spinners per processor, so that the scheduler leaves none to the holder */
#define SPIN_PER_CPU 2

static pthread_mutex_t mutex;
static struct timespec acquired;
static long long       deadline; /* CLOCK_MONOTONIC ns; zero until set */
static int             held, release, stop, spinning;

static long long
ns(const struct timespec *ts)
{
    return (long long)ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

static long long
now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ns(&ts);
}

static void
nap(void)
{
    struct timespec ms = { 0, 1000 * 1000 };

    nanosleep(&ms, NULL);
}

/*
 * Holds the mutex until told to release it, which it sees only when it
 * gets a processor again after each nap
 */
static void *
low(void *arg)
{
    (void)arg;
    if (pthread_mutex_lock(&mutex) != 0)
        return (void *)1;
    __atomic_store_n(&held, 1, __ATOMIC_SEQ_CST);
    while (!__atomic_load_n(&release, __ATOMIC_SEQ_CST))
        nap();
    if (pthread_mutex_unlock(&mutex) != 0)
        return (void *)2;
    return NULL;
}

/* Keeps a processor busy until the high priority thread is done, or the deadline */
static void *
spin(void *arg)
{
    long long limit;

    (void)arg;
    __atomic_add_fetch(&spinning, 1, __ATOMIC_SEQ_CST);
    while (!__atomic_load_n(&stop, __ATOMIC_SEQ_CST)) {
        limit = __atomic_load_n(&deadline, __ATOMIC_SEQ_CST);
        if (limit && now() >= limit)
            break;
    }
    return NULL;
}

static void *
high(void *arg)
{
    (void)arg;
    if (pthread_mutex_lock(&mutex) != 0)
        return (void *)1;
    clock_gettime(CLOCK_MONOTONIC, &acquired);
    __atomic_store_n(&stop, 1, __ATOMIC_SEQ_CST);
    if (pthread_mutex_unlock(&mutex) != 0)
        return (void *)2;
    return NULL;
}

static void
start(pthread_t *thread, void *(*func)(void *), int priority)
{
    pthread_attr_t     attr;
    struct sched_param param = { .sched_priority = priority };

    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0, "setinheritsched");
    check(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0, "setschedpolicy");
    check(pthread_attr_setschedparam(&attr, &param) == 0, "setschedparam");
    check(pthread_create(thread, &attr, func, NULL) == 0, "pthread_create");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");
}

static void
join(pthread_t thread)
{
    void *result;

    check(pthread_join(thread, &result) == 0 && result == NULL, "pthread_join");
}

int
main(void)
{
    static pthread_t    spinners[MAX_SPINNERS];
    pthread_mutexattr_t attr;
    pthread_t           low_thread, high_thread;
    struct sched_param  param;
    int                 min = sched_get_priority_min(SCHED_FIFO);
    int                 max = sched_get_priority_max(SCHED_FIFO);
    long                ncpu = sysconf(_SC_NPROCESSORS_ONLN), nspin;

    check(min >= 0 && max >= min + 4, "SCHED_FIFO priority range");
    param.sched_priority = max;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        printf("no SCHED_FIFO without privilege; skipping\n");
        return 77;
    }
    if (ncpu < 1)
        ncpu = 1;
    nspin = ncpu * SPIN_PER_CPU;
    if (nspin > MAX_SPINNERS)
        nspin = MAX_SPINNERS;

    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT) == 0, "setprotocol");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");

    start(&low_thread, low, min + 1);
    while (!__atomic_load_n(&held, __ATOMIC_SEQ_CST))
        nap();
    for (long i = 0; i < nspin; i++)
        start(&spinners[i], spin, min + 2);
    /* Once every processor has one, the rest wait their turn */
    while (__atomic_load_n(&spinning, __ATOMIC_SEQ_CST) < (nspin < ncpu ? nspin : ncpu))
        nap();
    __atomic_store_n(&deadline, now() + SPIN_SECONDS * 1000000000LL, __ATOMIC_SEQ_CST);

    /* The low priority thread now can't run, unless it inherits a priority */
    __atomic_store_n(&release, 1, __ATOMIC_SEQ_CST);
    start(&high_thread, high, min + 3);

    join(high_thread);
    join(low_thread);
    for (long i = 0; i < nspin; i++)
        join(spinners[i]);
    check(__atomic_load_n(&stop, __ATOMIC_SEQ_CST), "high priority thread ran");
    check(ns(&acquired) < deadline, "the holder inherited the waiter's priority");
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
