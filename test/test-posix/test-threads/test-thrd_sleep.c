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
#if _POSIX_THREADS > 0

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/*
 * thrd_sleep: it sleeps at least the duration as TIME_UTC measures
 * it, returns -1 with the remaining time when a caught signal
 * interrupts it, a negative value with errno for invalid durations,
 * and is a cancellation point
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NSEC 1000000000LL

static volatile int    signalled;
static volatile int    done;
static volatile int    returned;

/* What the interrupted sleep reported */
static struct timespec sleep_duration, sleep_remaining;
static long long       sleep_elapsed;
static int             sleep_ret, sleep_errno;

static void
handler(int sig)
{
    (void)sig;
    signalled++;
}

static long long
now_ns(void)
{
    struct timespec ts;

    check(timespec_get(&ts, TIME_UTC) == TIME_UTC, "timespec_get");
    return ts.tv_sec * NSEC + ts.tv_nsec;
}

static long long
ns(const struct timespec *ts)
{
    return ts->tv_sec * NSEC + ts->tv_nsec;
}

/* Sleeps 30 s, which main's signals interrupt; arg asks for one timespec for both */
static int
sleeper(void *arg)
{
    struct timespec duration = { 30, 0 }, remaining = { -1, -1 };
    long long       start = now_ns();

    sleep_duration = duration;
    if (arg) {
        sleep_ret = thrd_sleep(&duration, &duration);
        remaining = duration;
    } else {
        sleep_ret = thrd_sleep(&duration, &remaining);
    }
    sleep_errno = errno;
    sleep_elapsed = now_ns() - start;
    sleep_remaining = remaining;
    __atomic_store_n(&done, 1, __ATOMIC_RELEASE);
    return 0;
}

static void
check_interrupted(int same)
{
    struct timespec ts = { 0, 20000000 };
    thrd_t          thr;
    int             i, res;
    long long       remaining;

    done = 0;
    signalled = 0;
    check(thrd_create(&thr, sleeper, same ? &thr : NULL) == thrd_success, "thrd_create");
    /* A signal sent before the sleep starts doesn't end it, so keep sending */
    for (i = 0; i < 1000 && !__atomic_load_n(&done, __ATOMIC_ACQUIRE); i++) {
        check(pthread_kill(thr, SIGUSR1) == 0, "pthread_kill");
        thrd_sleep(&ts, NULL);
    }
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    check(signalled > 0, "handler ran");
    check(sleep_ret == -1, "thrd_sleep interrupted returns -1");
#ifndef __GLIBC__
    /* glibc's thrd_sleep leaves errno alone */
    check(sleep_errno == EINTR, "errno is EINTR");
#else
    (void)sleep_errno;
#endif
    check(sleep_remaining.tv_nsec >= 0 && sleep_remaining.tv_nsec < NSEC,
          "remaining time is valid");
    remaining = ns(&sleep_remaining);
    printf("%s timespec: interrupted after %lld ns with %lld ns remaining\n", same ? "one" : "two",
           sleep_elapsed, remaining);
    /*
     * The requested interval minus the time actually slept. Linux
     * computes it from the timer's expiry, which includes the
     * thread's timer slack (50 us by default), so when the signal
     * comes within the slack of the start it can exceed the request
     * by up to that much; allow 1 ms.
     */
    check(remaining > 0 && remaining <= ns(&sleep_duration) + NSEC / 1000,
          "remaining time in range");
    check(remaining >= ns(&sleep_duration) - sleep_elapsed - NSEC / 10,
          "remaining time accounts for the time slept");
}

static pthread_t main_thread;

/* Sends main an ignored signal while it sleeps */
static void *
ignored_sender(void *arg)
{
    struct timespec ts = { 0, 10000000 };
    int             i;

    (void)arg;
    for (i = 0; i < 5; i++) {
        thrd_sleep(&ts, NULL);
        pthread_kill(main_thread, SIGUSR2);
    }
    return NULL;
}

static void *
cancelled_sleeper(void *arg)
{
    struct timespec duration = { 60, 0 };

    (void)arg;
    pthread_cancel(pthread_self());
    (void)thrd_sleep(&duration, NULL);
    returned = 1;
    return NULL;
}

int
main(void)
{
    struct sigaction act;
    struct timespec  ts;
    long long        start, elapsed;
    pthread_t        pthr;
    void            *pres = NULL;

    alarm(120);

    ts = (struct timespec) { 0, 0 };
    check(thrd_sleep(&ts, NULL) == 0, "thrd_sleep zero");

    /* Never shorter than requested */
    ts = (struct timespec) { 0, 50000000 };
    start = now_ns();
    check(thrd_sleep(&ts, NULL) == 0, "thrd_sleep 50 ms");
    elapsed = now_ns() - start;
    check(elapsed >= ns(&ts), "slept at least 50 ms");

    ts = (struct timespec) { 1, 0 };
    start = now_ns();
    check(thrd_sleep(&ts, &ts) == 0, "thrd_sleep 1 s");
    elapsed = now_ns() - start;
    check(elapsed >= NSEC, "slept at least 1 s");

    /* A negative value, with errno set, for invalid durations */
    ts = (struct timespec) { 0, -1 };
    errno = 0;
    check(thrd_sleep(&ts, NULL) < 0, "thrd_sleep negative nanoseconds");
#ifndef __GLIBC__
    /* glibc's thrd_sleep leaves errno alone */
    check(errno == EINVAL, "errno for negative nanoseconds");
#endif
    ts = (struct timespec) { 0, 1000000000L };
    errno = 0;
    check(thrd_sleep(&ts, NULL) < 0, "thrd_sleep a billion nanoseconds");
#ifndef __GLIBC__
    check(errno == EINVAL, "errno for a billion nanoseconds");
#endif

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check_interrupted(0);
    check_interrupted(1);

    /* An ignored signal doesn't end the sleep */
    act.sa_handler = SIG_IGN;
    check(sigaction(SIGUSR2, &act, NULL) == 0, "sigaction");
    ts = (struct timespec) { 0, 200000000 };
    main_thread = pthread_self();
    check(pthread_create(&pthr, NULL, ignored_sender, NULL) == 0, "pthread_create");
    start = now_ns();
    check(thrd_sleep(&ts, NULL) == 0, "ignored signals");
    check(now_ns() - start >= ns(&ts), "slept at least 200 ms");
    check(pthread_join(pthr, NULL) == 0, "pthread_join");

    /* A cancellation point */
    check(pthread_create(&pthr, NULL, cancelled_sleeper, NULL) == 0, "pthread_create");
    check(pthread_join(pthr, &pres) == 0, "pthread_join");
    check(pres == PTHREAD_CANCELED && !returned, "thrd_sleep is a cancellation point");

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
