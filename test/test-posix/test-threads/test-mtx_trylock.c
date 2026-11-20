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
#if !defined(__PICOLIBC__) || (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/*
 * mtx_ mutexes: which result each call gives (thrd_busy,
 * thrd_timedout, thrd_error), mtx_trylock on a mutex the caller
 * holds, lock counts of recursive mutexes, and waits that signals
 * interrupt
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static mtx_t        mtx;
static volatile int released;
static volatile int signalled;

static void
handler(int sig)
{
    (void)sig;
    signalled++;
}

/* The deadline ms milliseconds from now; negative values are in the past */
static struct timespec
deadline(long ms)
{
    struct timespec ts;

    check(timespec_get(&ts, TIME_UTC) == TIME_UTC, "timespec_get");
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    } else if (ts.tv_nsec < 0) {
        ts.tv_sec--;
        ts.tv_nsec += 1000000000L;
    }
    return ts;
}

static int
after(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec >= b->tv_nsec);
}

/* mtx_trylock from another thread, which gives up the mutex again */
static int
try_other(void *arg)
{
    int ret = mtx_trylock(&mtx);

    (void)arg;
    if (ret == thrd_success && mtx_unlock(&mtx) != thrd_success)
        return -1;
    return ret;
}

static int
other_trylock(void)
{
    thrd_t thr;
    int    res = -2;

    check(thrd_create(&thr, try_other, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    return res;
}

static int
timed_other(void *arg)
{
    struct timespec ts = deadline(50), now;
    int             ret = mtx_timedlock(&mtx, &ts);

    (void)arg;
    if (ret == thrd_success)
        return mtx_unlock(&mtx) == thrd_success ? thrd_success : -1;
    /* It waits until after the deadline, as TIME_UTC measures it */
    check(timespec_get(&now, TIME_UTC) == TIME_UTC, "timespec_get");
    if (ret == thrd_timedout && !after(&now, &ts))
        return -3;
    return ret;
}

static int
past_other(void *arg)
{
    struct timespec ts = deadline(-1000);

    (void)arg;
    return mtx_timedlock(&mtx, &ts);
}

static int
invalid_other(void *arg)
{
    struct timespec ts = deadline(1000);

    (void)arg;
    ts.tv_nsec = 1000000000L;
    return mtx_timedlock(&mtx, &ts);
}

static int
run(int (*func)(void *))
{
    thrd_t thr;
    int    res = -2;

    check(thrd_create(&thr, func, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    return res;
}

/* Waits for the mutex while main sends signals; it must keep waiting */
static int
lock_waiter(void *arg)
{
    struct timespec ts = deadline(60000);
    int             ret;

    ret = arg ? mtx_timedlock(&mtx, &ts) : mtx_lock(&mtx);
    if (ret != thrd_success)
        return ret;
    ret = released ? thrd_success : -1;
    mtx_unlock(&mtx);
    return ret;
}

static void
check_signals(int timed)
{
    struct timespec ts = { 0, 10000000 };
    thrd_t          thr;
    int             res = -2, i;

    released = 0;
    signalled = 0;
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    check(thrd_create(&thr, lock_waiter, timed ? &mtx : NULL) == thrd_success, "thrd_create");
    for (i = 0; i < 5; i++) {
        check(pthread_kill(thr, SIGUSR1) == 0, "pthread_kill");
        thrd_sleep(&ts, NULL);
    }
    released = 1;
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    if (timed)
        check(res == thrd_success, "mtx_timedlock resumes waiting after a signal");
    else
        check(res == thrd_success, "mtx_lock resumes waiting after a signal");
    check(signalled > 0, "handler ran");
}

int
main(void)
{
    static const int plain_types[] = { mtx_plain, mtx_timed };
    static const int recursive_types[] = { mtx_plain | mtx_recursive, mtx_timed | mtx_recursive };
    struct sigaction act;
    struct timespec  ts;
    unsigned         i;
    int              res;

    alarm(60);

    for (i = 0; i < 2; i++) {
        check(mtx_init(&mtx, plain_types[i]) == thrd_success, "mtx_init");
        check(mtx_trylock(&mtx) == thrd_success, "mtx_trylock unlocked");
        /* A non-recursive mutex the caller holds is busy, without blocking */
        check(mtx_trylock(&mtx) == thrd_busy, "mtx_trylock by the owner");
        check(other_trylock() == thrd_busy, "mtx_trylock held by another thread");
        check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
        check(other_trylock() == thrd_success, "mtx_trylock after unlock");
        mtx_destroy(&mtx);
        /* A destroyed mutex can be initialized again */
        check(mtx_init(&mtx, plain_types[i]) == thrd_success, "mtx_init again");
        mtx_destroy(&mtx);
    }

    for (i = 0; i < 2; i++) {
        check(mtx_init(&mtx, recursive_types[i]) == thrd_success, "mtx_init recursive");
        check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
        /* The owner's mtx_trylock adds one to the lock count */
        check(mtx_trylock(&mtx) == thrd_success, "mtx_trylock by the owner");
        check(mtx_lock(&mtx) == thrd_success, "mtx_lock by the owner");
        if (recursive_types[i] & mtx_timed) {
            ts = deadline(-1000);
            check(mtx_timedlock(&mtx, &ts) == thrd_success, "mtx_timedlock by the owner");
            check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
        }
        check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock 3");
        check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock 2");
        check(other_trylock() == thrd_busy, "still held at a count of one");
        check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock 1");
        check(other_trylock() == thrd_success, "available at a count of zero");
        mtx_destroy(&mtx);
    }

#ifdef __PICOLIBC__
    /*
     * Reaching the lock count limit takes UINT_MAX locks, too many to
     * make; start just below it using picolibc's count, which holds
     * the locks beyond the first. The limit isn't thrd_nomem, which
     * these functions don't return.
     */
    check(mtx_init(&mtx, mtx_timed | mtx_recursive) == thrd_success, "mtx_init");
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    mtx.__count = UINT_MAX - 1;
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock to the limit");
    check(mtx_lock(&mtx) == thrd_error, "mtx_lock past the lock count limit");
    res = mtx_trylock(&mtx);
    check(res == thrd_error || res == thrd_busy, "mtx_trylock past the lock count limit");
    ts = deadline(1000);
    check(mtx_timedlock(&mtx, &ts) == thrd_error, "mtx_timedlock past the lock count limit");
    mtx.__count = 0;
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(other_trylock() == thrd_success, "available again");
    mtx_destroy(&mtx);
#endif

    check(mtx_init(&mtx, mtx_timed) == thrd_success, "mtx_init timed");
    /* Never a timeout when the mutex can be locked at once */
    ts = deadline(-1000);
    check(mtx_timedlock(&mtx, &ts) == thrd_success, "mtx_timedlock free, past deadline");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    ts = deadline(1000);
    ts.tv_nsec = -1;
    res = mtx_timedlock(&mtx, &ts);
    check(res == thrd_success || res == thrd_error, "mtx_timedlock free, invalid deadline");
    if (res == thrd_success)
        check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");

    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    check(run(timed_other) == thrd_timedout, "mtx_timedlock times out");
    check(run(past_other) == thrd_timedout, "mtx_timedlock with a past deadline");
    check(run(invalid_other) == thrd_error, "mtx_timedlock with an invalid deadline");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(run(timed_other) == thrd_success, "mtx_timedlock once unlocked");

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check_signals(0);
    check_signals(1);
    mtx_destroy(&mtx);

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
