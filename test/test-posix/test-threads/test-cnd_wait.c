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
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/*
 * cnd_ condition variables: which result each wait gives, the mutex
 * after timeouts and errors, waits that signals interrupt, and
 * cnd_wait and cnd_timedwait as cancellation points
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static mtx_t        mtx;
static cnd_t        cnd;
static sem_t        started;
static volatile int ready;
static volatile int signalled;
static volatile int owned;
static volatile int returned;

static void
handler(int sig)
{
    (void)sig;
    signalled++;
}

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

static int
try_other(void *arg)
{
    int ret = mtx_trylock(&mtx);

    (void)arg;
    if (ret == thrd_success && mtx_unlock(&mtx) != thrd_success)
        return -1;
    return ret;
}

/* Whether another thread finds mtx busy */
static int
held(void)
{
    thrd_t thr;
    int    res = -2;

    check(thrd_create(&thr, try_other, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    return res == thrd_busy;
}

/*
 * Waits until ready while main sends signals. Each return is
 * thrd_success: a signal either doesn't end the wait or ends it as a
 * spurious wakeup.
 */
static int
signal_waiter(void *arg)
{
    struct timespec ts = deadline(60000);
    int             ret = thrd_success;

    mtx_lock(&mtx);
    sem_post(&started);
    while (!ready && ret == thrd_success)
        ret = arg ? cnd_timedwait(&cnd, &mtx, &ts) : cnd_wait(&cnd, &mtx);
    mtx_unlock(&mtx);
    return ret;
}

static void
check_signals(int timed)
{
    struct timespec ts = { 0, 10000000 };
    thrd_t          thr;
    int             res = -2, i;

    ready = 0;
    signalled = 0;
    check(thrd_create(&thr, signal_waiter, timed ? &mtx : NULL) == thrd_success, "thrd_create");
    check(sem_wait(&started) == 0, "sem_wait");
    /* The waiter has released the mutex in its wait */
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    for (i = 0; i < 5; i++) {
        check(pthread_kill(thr, SIGUSR1) == 0, "pthread_kill");
        thrd_sleep(&ts, NULL);
    }
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    ready = 1;
    check(cnd_broadcast(&cnd) == thrd_success, "cnd_broadcast");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    if (timed)
        check(res == thrd_success, "cnd_timedwait interrupted by signals");
    else
        check(res == thrd_success, "cnd_wait interrupted by signals");
    check(signalled > 0, "handler ran");
}

/*
 * The cancellation tests use a recursive mutex, which the owner can
 * relock with mtx_trylock: that shows the cleanup handler runs with
 * the mutex held.
 */
static void
cleanup(void *arg)
{
    (void)arg;
    owned = mtx_trylock(&mtx) == thrd_success;
    if (owned) {
        mtx_unlock(&mtx);
        mtx_unlock(&mtx);
    }
}

/* Blocks in cnd_wait until cancelled */
static void *
blocked_waiter(void *arg)
{
    (void)arg;
    mtx_lock(&mtx);
    pthread_cleanup_push(cleanup, NULL);
    sem_post(&started);
    for (;;)
        cnd_wait(&cnd, &mtx);
    pthread_cleanup_pop(0);
    return NULL;
}

/* Calls a wait with a cancellation request pending */
static void *
pending_waiter(void *arg)
{
    struct timespec ts = deadline(60000);

    pthread_cancel(pthread_self());
    mtx_lock(&mtx);
    pthread_cleanup_push(cleanup, NULL);
    if (arg)
        (void)cnd_timedwait(&cnd, &mtx, &ts);
    else
        (void)cnd_wait(&cnd, &mtx);
    returned = 1;
    pthread_cleanup_pop(1);
    return NULL;
}

static void
check_pending(int timed)
{
    pthread_t thr;
    void     *res = NULL;

    owned = 0;
    returned = 0;
    check(pthread_create(&thr, NULL, pending_waiter, timed ? &mtx : NULL) == 0, "pthread_create");
    check(pthread_join(thr, &res) == 0, "pthread_join");
    if (timed)
        check(res == PTHREAD_CANCELED && !returned, "cnd_timedwait is a cancellation point");
    else
        check(res == PTHREAD_CANCELED && !returned, "cnd_wait is a cancellation point");
    check(owned, "mutex held in the cleanup handler");
    check(!held(), "mutex released");
}

int
main(void)
{
    struct sigaction act;
    struct timespec  ts, now;
    pthread_t        pthr;
    void            *pres = NULL;

    alarm(60);
    check(sem_init(&started, 0, 0) == 0, "sem_init");
    check(mtx_init(&mtx, mtx_timed) == thrd_success, "mtx_init");
    check(cnd_init(&cnd) == thrd_success, "cnd_init");

    /* With nothing waiting, these have no effect and succeed */
    check(cnd_signal(&cnd) == thrd_success, "cnd_signal");
    check(cnd_broadcast(&cnd) == thrd_success, "cnd_broadcast");

    /* A timed out wait releases and reacquires the mutex */
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    ts = deadline(50);
    check(cnd_timedwait(&cnd, &mtx, &ts) == thrd_timedout, "cnd_timedwait times out");
    check(timespec_get(&now, TIME_UTC) == TIME_UTC && after(&now, &ts),
          "no timeout before the deadline");
    check(held(), "mutex held after a timeout");
    ts = deadline(-1000);
    check(cnd_timedwait(&cnd, &mtx, &ts) == thrd_timedout, "cnd_timedwait past deadline");
    check(held(), "mutex held after a past deadline");
    ts = deadline(1000);
    ts.tv_nsec = 1000000000L;
    check(cnd_timedwait(&cnd, &mtx, &ts) == thrd_error, "cnd_timedwait invalid deadline");
    check(held(), "mutex held after an error");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(!held(), "mutex released");

    /* A destroyed condition variable can be initialized again */
    cnd_destroy(&cnd);
    check(cnd_init(&cnd) == thrd_success, "cnd_init again");

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check_signals(0);
    check_signals(1);
    mtx_destroy(&mtx);

    check(mtx_init(&mtx, mtx_plain | mtx_recursive) == thrd_success, "mtx_init recursive");

    /* Cancelling a thread blocked in cnd_wait */
    owned = 0;
    check(pthread_create(&pthr, NULL, blocked_waiter, NULL) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait");
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    check(pthread_cancel(pthr) == 0, "pthread_cancel");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(pthread_join(pthr, &pres) == 0, "pthread_join");
    check(pres == PTHREAD_CANCELED, "cnd_wait cancelled while blocked");
    check(owned, "mutex reacquired before the cleanup handler");
    check(!held(), "mutex released");

    check_pending(0);
    check_pending(1);

    cnd_destroy(&cnd);
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
