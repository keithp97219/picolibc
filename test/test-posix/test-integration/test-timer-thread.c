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

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && _POSIX_TIMERS > 0

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX timers running a function in a thread */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int             calls;
static int             value_ok = 1;
static pthread_t       caller;

static void
notify(union sigval value)
{
    pthread_mutex_lock(&lock);
    if (value.sival_int != 77)
        value_ok = 0;
    caller = pthread_self();
    calls++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}

int
main(void)
{
    struct sigevent   ev;
    struct itimerspec its;
    struct timespec   ts = { .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 };
    timer_t           timer;

    memset(&ev, 0, sizeof(ev));
    ev.sigev_notify = SIGEV_THREAD;
    ev.sigev_notify_function = notify;
    ev.sigev_value.sival_int = 77;
    check(timer_create(CLOCK_MONOTONIC, &ev, &timer) == 0, "timer_create");

    memset(&its, 0, sizeof(its));
    its.it_interval.tv_nsec = 10 * 1000 * 1000;
    its.it_value.tv_nsec = 10 * 1000 * 1000;
    check(timer_settime(timer, 0, &its, NULL) == 0, "timer_settime");

    pthread_mutex_lock(&lock);
    while (calls < 3)
        pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);
    check(value_ok, "notification value");
    check(!pthread_equal(caller, pthread_self()), "called in another thread");

    check(timer_delete(timer) == 0, "timer_delete");
    /* No calls after the timer is gone and pending ones have drained */
    nanosleep(&ts, NULL);
    pthread_mutex_lock(&lock);
    calls = 0;
    pthread_mutex_unlock(&lock);
    nanosleep(&ts, NULL);
    check(calls == 0, "no calls after timer_delete");

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
