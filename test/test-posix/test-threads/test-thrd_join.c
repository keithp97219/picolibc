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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

/*
 * thrd_join: exit statuses, waits that signals interrupt, and
 * thrd_join as a cancellation point. The other <threads.h>
 * functions are not cancellation points.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sem_t        release, started;
static volatile int signalled;
static volatile int returned;
static volatile int survived;
static thrd_t       target;

static void
handler(int sig)
{
    (void)sig;
    signalled++;
}

static int
status(void *arg)
{
    return *(int *)arg;
}

/* Waits for main's go-ahead, then returns 99 */
static int
waiting(void *arg)
{
    (void)arg;
    while (sem_wait(&release) != 0)
        ;
    return 99;
}

static int
joiner(void *arg)
{
    int res = -1, ret;

    (void)arg;
    sem_post(&started);
    ret = thrd_join(target, &res);
    if (ret != thrd_success)
        return 1000 + ret;
    return res;
}

static void *
cancelled_joiner(void *arg)
{
    int res;

    (void)arg;
    pthread_cancel(pthread_self());
    (void)thrd_join(target, &res);
    returned = 1;
    return NULL;
}

static once_flag flag = ONCE_FLAG_INIT;

static void
init(void)
{
}

static int
nothing(void *arg)
{
    (void)arg;
    return 0;
}

/* Calls every other <threads.h> function with a cancellation request pending */
static void *
not_points(void *arg)
{
    struct timespec ts;
    mtx_t           mtx;
    cnd_t           cnd;
    thrd_t          thr;
    tss_t           key;

    (void)arg;
    pthread_cancel(pthread_self());
    call_once(&flag, init);
    if (cnd_init(&cnd) != thrd_success || cnd_signal(&cnd) != thrd_success
        || cnd_broadcast(&cnd) != thrd_success)
        return NULL;
    cnd_destroy(&cnd);
    if (mtx_init(&mtx, mtx_timed) != thrd_success || mtx_lock(&mtx) != thrd_success
        || mtx_unlock(&mtx) != thrd_success || mtx_trylock(&mtx) != thrd_success
        || mtx_unlock(&mtx) != thrd_success)
        return NULL;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC)
        return NULL;
    ts.tv_sec += 10;
    if (mtx_timedlock(&mtx, &ts) != thrd_success || mtx_unlock(&mtx) != thrd_success)
        return NULL;
    mtx_destroy(&mtx);
    if (thrd_create(&thr, nothing, NULL) != thrd_success || thrd_detach(thr) != thrd_success)
        return NULL;
    if (!thrd_equal(thrd_current(), thrd_current()))
        return NULL;
    thrd_yield();
    if (tss_create(&key, NULL) != thrd_success || tss_set(key, &key) != thrd_success
        || tss_get(key) != &key)
        return NULL;
    tss_delete(key);
    survived = 1;
    pthread_testcancel();
    returned = 1;
    return NULL;
}

int
main(void)
{
    static const int values[] = { 0, 1, -1, -12345, INT_MAX, INT_MIN };
    struct sigaction act;
    struct timespec  ts = { 0, 10000000 };
    thrd_t           thr;
    pthread_t        pthr;
    void            *pres = NULL;
    unsigned         i;
    int              res;

    alarm(60);
    check(sem_init(&release, 0, 0) == 0 && sem_init(&started, 0, 0) == 0, "sem_init");

    /* The whole int range comes back as the exit status */
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        res = 42;
        check(thrd_create(&thr, status, (void *)&values[i]) == thrd_success, "thrd_create");
        check(thrd_join(thr, &res) == thrd_success, "thrd_join");
        check(res == values[i], "exit status");
    }
    check(thrd_create(&thr, status, (void *)&values[1]) == thrd_success, "thrd_create");
    check(thrd_join(thr, NULL) == thrd_success, "thrd_join with no status");

#ifdef __PICOLIBC__
    /* picolibc detects joining the calling thread, which must then be thrd_error */
    check(thrd_join(thrd_current(), &res) == thrd_error, "thrd_join of itself");
#endif

    /* Signals to a thread blocked in thrd_join don't end the join */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(thrd_create(&target, waiting, NULL) == thrd_success, "thrd_create");
    check(thrd_create(&thr, joiner, NULL) == thrd_success, "thrd_create");
    check(sem_wait(&started) == 0, "sem_wait");
    for (i = 0; i < 5; i++) {
        check(pthread_kill(thr, SIGUSR1) == 0, "pthread_kill");
        thrd_sleep(&ts, NULL);
    }
    check(sem_post(&release) == 0, "sem_post");
    check(thrd_join(thr, &res) == thrd_success, "thrd_join");
    check(res == 99, "thrd_join interrupted by signals");
    check(signalled > 0, "handler ran");

    /* A cancellation point; the target stays joinable */
    check(thrd_create(&target, waiting, NULL) == thrd_success, "thrd_create");
    check(pthread_create(&pthr, NULL, cancelled_joiner, NULL) == 0, "pthread_create");
    check(pthread_join(pthr, &pres) == 0, "pthread_join");
    check(pres == PTHREAD_CANCELED && !returned, "thrd_join is a cancellation point");
    check(sem_post(&release) == 0, "sem_post");
    check(thrd_join(target, &res) == thrd_success && res == 99, "target still joinable");

    /* No other <threads.h> function is a cancellation point */
    returned = 0;
    check(pthread_create(&pthr, NULL, not_points, NULL) == 0, "pthread_create");
    check(pthread_join(pthr, &pres) == 0, "pthread_join");
    check(survived, "<threads.h> functions with a cancellation request pending");
    check(pres == PTHREAD_CANCELED && !returned, "request still pending");

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
