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
/* glibc declares sem_clockwait only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_SEMAPHORES && TEST_POSIX_THREADS && TEST_POSIX_TIMEOUTS && TEST_POSIX_MONOTONIC_CLOCK

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Timed semaphore waits: when they time out, argument checks, signals and cancellation */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sem_t sem;

static void
timeout_in(clockid_t clock, struct timespec *ts, long ms)
{
    check(clock_gettime(clock, ts) == 0, "clock_gettime");
    ts->tv_nsec += ms * 1000 * 1000;
    while (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec++;
    }
}

/* a >= b */
static int
not_before(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec >= b->tv_nsec);
}

static int
value(void)
{
    int v;

    check(sem_getvalue(&sem, &v) == 0, "sem_getvalue");
    return v;
}

/* Wait on clock until 30ms from now; it must time out no earlier than that */
static void
times_out(clockid_t clock, int use_clockwait)
{
    struct timespec deadline, now;
    int             ret;

    timeout_in(clock, &deadline, 30);
    if (use_clockwait)
        ret = sem_clockwait(&sem, clock, &deadline);
    else
        ret = sem_timedwait(&sem, &deadline);
    check(ret == -1 && errno == ETIMEDOUT, "times out");
    check(clock_gettime(clock, &now) == 0, "clock_gettime");
    check(not_before(&now, &deadline), "not before the deadline");
    check(value() == 0, "value unchanged");
}

static void
post_handler(int sig)
{
    (void)sig;
    sem_post(&sem);
}

static pthread_t main_thread;

static void *
signaller(void *arg)
{
    (void)arg;
    pthread_kill(main_thread, SIGUSR1);
    return NULL;
}

static void *
cancelled_wait(void *arg)
{
    struct timespec ts;
    int             which = *(int *)arg;

    timeout_in(CLOCK_REALTIME, &ts, 60 * 1000);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_cancel(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    if (which == 0)
        sem_timedwait(&sem, &ts);
    else if (which == 1)
        sem_clockwait(&sem, CLOCK_REALTIME, &ts);
    else
        sem_wait(&sem);
    return NULL;
}

int
main(void)
{
    struct timespec  ts;
    struct sigaction act;
    pthread_t        thread;
    void            *result;
    int              ret;

    check(sem_init(&sem, 0, 0) == 0, "sem_init");

    /* Timeouts on each clock */
    times_out(CLOCK_REALTIME, 0);
    times_out(CLOCK_REALTIME, 1);
    times_out(CLOCK_MONOTONIC, 1);

    /* A deadline that has passed times out at once when the semaphore is locked */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(sem_timedwait(&sem, &ts) == -1 && errno == ETIMEDOUT, "past deadline");
    check(sem_clockwait(&sem, CLOCK_MONOTONIC, &ts) == -1 && errno == ETIMEDOUT,
          "past monotonic deadline");

    /* ... and never when it can be locked */
    check(sem_post(&sem) == 0, "sem_post");
    check(sem_timedwait(&sem, &ts) == 0, "past deadline, unlocked");
    check(sem_post(&sem) == 0, "sem_post");
    check(sem_clockwait(&sem, CLOCK_MONOTONIC, &ts) == 0, "past monotonic deadline, unlocked");

    /* picolibc doesn't check the deadline when it needn't wait, which POSIX allows */
#ifdef __PICOLIBC__
    ts.tv_nsec = -1;
    check(sem_post(&sem) == 0, "sem_post");
    check(sem_timedwait(&sem, &ts) == 0, "bad deadline, unlocked");
#endif

    /* Bad deadlines and clocks when it would block; the value stays the same */
    timeout_in(CLOCK_REALTIME, &ts, 1000);
    ts.tv_nsec = -1;
    errno = 0;
    check(sem_timedwait(&sem, &ts) == -1 && errno == EINVAL, "negative nanoseconds");
    errno = 0;
    check(sem_clockwait(&sem, CLOCK_REALTIME, &ts) == -1 && errno == EINVAL,
          "sem_clockwait negative nanoseconds");
    ts.tv_nsec = 1000000000L;
    errno = 0;
    check(sem_clockwait(&sem, CLOCK_MONOTONIC, &ts) == -1 && errno == EINVAL,
          "sem_clockwait a billion nanoseconds");
    timeout_in(CLOCK_REALTIME, &ts, 1000);
    errno = 0;
    check(sem_clockwait(&sem, (clockid_t)0x7fff, &ts) == -1 && errno == EINVAL, "unknown clock");
    errno = 0;
    check(sem_clockwait(&sem, CLOCK_PROCESS_CPUTIME_ID, &ts) == -1 && errno == EINVAL,
          "CPU-time clock");
    check(value() == 0, "value unchanged");

    /* sem_post is async-signal-safe, and wakes a waiter from a handler */
    memset(&act, 0, sizeof(act));
    act.sa_handler = post_handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    main_thread = pthread_self();
    check(pthread_create(&thread, NULL, signaller, NULL) == 0, "pthread_create");
    ret = sem_wait(&sem);
    /* An interrupted wait leaves the posted value for the next one */
    if (ret == -1) {
        check(errno == EINTR, "interrupted");
        check(sem_trywait(&sem) == 0, "posted by the handler");
    }
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(value() == 0, "value");

    /* The waits are cancellation points */
    for (int which = 0; which < 3; which++) {
        check(pthread_create(&thread, NULL, cancelled_wait, &which) == 0, "pthread_create");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == PTHREAD_CANCELED, "cancelled");
    }
    check(value() == 0, "value");

    check(sem_destroy(&sem) == 0, "sem_destroy");
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
