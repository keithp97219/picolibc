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
#if TEST_POSIX_C_LANG_THREADS && TEST_POSIX_THREADS && TEST_POSIX_SIGNALS

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
 * call_once runs its function exactly once across threads. A
 * function cancelled at a cancellation point leaves the flag as if
 * call_once had never been called, callers using other flags don't
 * wait for it, and a signal handler doesn't disturb it.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static once_flag    flag = ONCE_FLAG_INIT;
static volatile int count;

static void
init(void)
{
    count++;
}

static int
child(void *arg)
{
    (void)arg;
    call_once(&flag, init);
    return count;
}

static once_flag    cancel_flag = ONCE_FLAG_INIT;
static once_flag    outer_flag = ONCE_FLAG_INIT;
static once_flag    inner_flag = ONCE_FLAG_INIT;
static once_flag    signal_flag = ONCE_FLAG_INIT;
static sem_t        in_init, waiter_go;
static volatile int cancel_count, inner_count, outer_ok, signalled, signal_count;
static volatile int returned;

/* Blocks at a cancellation point until cancelled */
static void
init_blocking(void)
{
    sem_post(&in_init);
    for (;;)
        pause();
}

static void
init_count(void)
{
    cancel_count++;
}

static void *
cancelled_caller(void *arg)
{
    (void)arg;
    call_once(&cancel_flag, init_blocking);
    returned = 1;
    return NULL;
}

static void *
waiting_caller(void *arg)
{
    (void)arg;
    while (sem_wait(&waiter_go) != 0)
        ;
    call_once(&cancel_flag, init_count);
    return NULL;
}

static void
init_inner(void)
{
    inner_count++;
}

static int
inner_caller(void *arg)
{
    (void)arg;
    call_once(&inner_flag, init_inner);
    return inner_count;
}

/* Waits for another thread using a different flag */
static void
init_outer(void)
{
    thrd_t thr;
    int    res = 0;

    if (thrd_create(&thr, inner_caller, NULL) == thrd_success
        && thrd_join(thr, &res) == thrd_success && res == 1)
        outer_ok = 1;
}

static void
handler(int sig)
{
    (void)sig;
    signalled++;
}

static void
init_signal(void)
{
    raise(SIGUSR1);
    signal_count++;
}

int
main(void)
{
    struct sigaction act;
    struct timespec  ts = { 0, 20000000 };
    pthread_t        cancelled, waiter;
    void            *pres = NULL;
    thrd_t           thr[4];
    int              i, res;

    alarm(60);

    for (i = 0; i < 4; i++)
        check(thrd_create(&thr[i], child, NULL) == thrd_success, "thrd_create");
    call_once(&flag, init);
    for (i = 0; i < 4; i++)
        check(thrd_join(thr[i], &res) == thrd_success && res == 1, "ran before return");
    check(count == 1, "once");

    /* A cancelled function leaves the flag unused; a waiting caller runs its own */
    check(sem_init(&in_init, 0, 0) == 0 && sem_init(&waiter_go, 0, 0) == 0, "sem_init");
    check(pthread_create(&cancelled, NULL, cancelled_caller, NULL) == 0, "pthread_create");
    check(sem_wait(&in_init) == 0, "sem_wait");
    check(pthread_create(&waiter, NULL, waiting_caller, NULL) == 0, "pthread_create");
    check(sem_post(&waiter_go) == 0, "sem_post");
    /* Give the waiter time to start waiting; under load it may only call later */
    thrd_sleep(&ts, NULL);
    check(pthread_cancel(cancelled) == 0, "pthread_cancel");
    check(pthread_join(cancelled, &pres) == 0, "pthread_join");
    check(pres == PTHREAD_CANCELED && !returned, "function cancelled");
    check(pthread_join(waiter, NULL) == 0, "pthread_join");
    check(cancel_count == 1, "waiting caller ran its function");
    call_once(&cancel_flag, init_count);
    check(cancel_count == 1, "flag used once the function returned");

    /* Callers using another flag don't wait for a function that is running */
    call_once(&outer_flag, init_outer);
    check(outer_ok, "call_once on another flag from within a function");

    /* A signal handler run during the call doesn't affect it */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    call_once(&signal_flag, init_signal);
    call_once(&signal_flag, init_signal);
    check(signalled == 1 && signal_count == 1, "signal during call_once");
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
