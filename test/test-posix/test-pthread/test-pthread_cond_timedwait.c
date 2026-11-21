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
/* glibc declares pthread_cond_clockwait only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_CLOCK_SELECTION > 0 && _POSIX_THREADS > 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Condition variables and their attributes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NCONSUMERS 4
#define NITEMS     10000
#define QUEUE_SIZE 8

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  not_full = PTHREAD_COND_INITIALIZER;
static int             queue[QUEUE_SIZE];
static int             head, tail, used;
static long            total;

static void *
consumer(void *arg)
{
    (void)arg;
    for (;;) {
        int item;

        pthread_mutex_lock(&lock);
        while (used == 0)
            pthread_cond_wait(&not_empty, &lock);
        item = queue[tail];
        tail = (tail + 1) % QUEUE_SIZE;
        used--;
        if (item >= 0)
            total += item;
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&lock);
        if (item < 0)
            return NULL;
    }
}

static void
produce(int item)
{
    pthread_mutex_lock(&lock);
    while (used == QUEUE_SIZE)
        pthread_cond_wait(&not_full, &lock);
    queue[head] = item;
    head = (head + 1) % QUEUE_SIZE;
    used++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}

static pthread_cond_t gate = PTHREAD_COND_INITIALIZER;
static int            open_gate;
static int            waiting;

static void *
wait_gate(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&lock);
    waiting++;
    pthread_cond_signal(&not_full);
    while (!open_gate)
        pthread_cond_wait(&gate, &lock);
    pthread_mutex_unlock(&lock);
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

int
main(void)
{
    pthread_t           threads[NCONSUMERS];
    pthread_cond_t      cond;
    pthread_condattr_t  attr;
    pthread_mutex_t     recursive;
    pthread_mutexattr_t mattr;
    struct timespec     ts;
    clockid_t           clock;
    int                 value;

    /* Producer and consumers */
    for (int i = 0; i < NCONSUMERS; i++)
        check(pthread_create(&threads[i], NULL, consumer, NULL) == 0, "pthread_create consumer");
    for (int i = 1; i <= NITEMS; i++)
        produce(i);
    for (int i = 0; i < NCONSUMERS; i++)
        produce(-1);
    for (int i = 0; i < NCONSUMERS; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join consumer");
    check(total == (long)NITEMS * (NITEMS + 1) / 2, "all items consumed");

    /* Broadcast wakes every waiter */
    for (int i = 0; i < NCONSUMERS; i++)
        check(pthread_create(&threads[i], NULL, wait_gate, NULL) == 0, "pthread_create gate");
    pthread_mutex_lock(&lock);
    while (waiting < NCONSUMERS)
        pthread_cond_wait(&not_full, &lock);
    open_gate = 1;
    check(pthread_cond_broadcast(&gate) == 0, "pthread_cond_broadcast");
    pthread_mutex_unlock(&lock);
    for (int i = 0; i < NCONSUMERS; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join gate");

    /* Attributes */
    check(pthread_condattr_init(&attr) == 0, "pthread_condattr_init");
    check(pthread_condattr_getclock(&attr, &clock) == 0 && clock == CLOCK_REALTIME,
          "default clock");
    check(pthread_condattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_PRIVATE,
          "default pshared");
    check(pthread_condattr_setpshared(&attr, -1) == EINVAL, "bad pshared");
    check(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) == 0, "setclock");
    check(pthread_condattr_getclock(&attr, &clock) == 0 && clock == CLOCK_MONOTONIC, "getclock");

    /* Timed waits on each clock */
    check(pthread_cond_init(&cond, &attr) == 0, "pthread_cond_init monotonic");
    check(pthread_condattr_destroy(&attr) == 0, "pthread_condattr_destroy");
    pthread_mutex_lock(&lock);
    timeout_in(CLOCK_MONOTONIC, &ts, 50);
    check(pthread_cond_timedwait(&cond, &lock, &ts) == ETIMEDOUT, "monotonic timedwait");
    timeout_in(CLOCK_REALTIME, &ts, 50);
    check(pthread_cond_clockwait(&cond, &lock, CLOCK_REALTIME, &ts) == ETIMEDOUT,
          "realtime clockwait");
    ts.tv_nsec = -1;
    check(pthread_cond_timedwait(&cond, &lock, &ts) == EINVAL, "bad timeout");
    pthread_mutex_unlock(&lock);
    check(pthread_cond_destroy(&cond) == 0, "pthread_cond_destroy");

    check(pthread_cond_init(&cond, NULL) == 0, "pthread_cond_init");
    pthread_mutex_lock(&lock);
    timeout_in(CLOCK_REALTIME, &ts, 50);
    check(pthread_cond_timedwait(&cond, &lock, &ts) == ETIMEDOUT, "realtime timedwait");
    timeout_in(CLOCK_MONOTONIC, &ts, 50);
    check(pthread_cond_clockwait(&cond, &lock, CLOCK_MONOTONIC, &ts) == ETIMEDOUT,
          "monotonic clockwait");
    pthread_mutex_unlock(&lock);

    /* A recursive mutex is owned by the waiter again after the wait */
    check(pthread_mutexattr_init(&mattr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) == 0, "settype");
    check(pthread_mutex_init(&recursive, &mattr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&mattr) == 0, "pthread_mutexattr_destroy");
    pthread_mutex_lock(&recursive);
    timeout_in(CLOCK_REALTIME, &ts, 10);
    check(pthread_cond_timedwait(&cond, &recursive, &ts) == ETIMEDOUT, "recursive timedwait");
    check(pthread_mutex_unlock(&recursive) == 0, "recursive unlock");
    check(pthread_mutex_unlock(&recursive) == EPERM, "recursive mutex released");
    check(pthread_cond_destroy(&cond) == 0, "pthread_cond_destroy");

    /* Signal and broadcast without waiters */
    check(pthread_cond_signal(&gate) == 0, "pthread_cond_signal");
    check(pthread_cond_broadcast(&gate) == 0, "pthread_cond_broadcast");

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
