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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_SEMAPHORES && TEST_POSIX_THREADS && TEST_POSIX_PIPE

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Thread cancellation and cleanup handlers */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t lock;
static pthread_cond_t  never = PTHREAD_COND_INITIALIZER;
static sem_t           started, blocked;
static int             pipe_fds[2];
static volatile int    trace[16];
static volatile int    ntrace;
static pthread_key_t   key;

static void
record(void *arg)
{
    trace[ntrace++] = (int)(intptr_t)arg;
}

/* The mutex is held again before cleanup handlers run */
static void
cond_cleanup(void *arg)
{
    (void)arg;
    trace[ntrace++] = pthread_mutex_unlock(&lock) == 0 ? 1 : -1;
}

static void *
cond_waiter(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&lock);
    pthread_cleanup_push(cond_cleanup, NULL);
    sem_post(&started);
    for (;;)
        pthread_cond_wait(&never, &lock);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
sleeper(void *arg)
{
    struct timespec ts = { .tv_sec = 10, .tv_nsec = 0 };

    (void)arg;
    pthread_cleanup_push(record, (void *)2);
    sem_post(&started);
    for (;;)
        nanosleep(&ts, NULL);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
reader(void *arg)
{
    char c;

    (void)arg;
    pthread_cleanup_push(record, (void *)3);
    sem_post(&started);
    for (;;)
        (void)read(pipe_fds[0], &c, 1);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
sem_waiter(void *arg)
{
    (void)arg;
    pthread_cleanup_push(record, (void *)4);
    sem_post(&started);
    for (;;)
        sem_wait(&blocked);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
joiner(void *arg)
{
    pthread_cleanup_push(record, (void *)5);
    sem_post(&started);
    pthread_join(*(pthread_t *)arg, NULL);
    pthread_cleanup_pop(0);
    return NULL;
}

static volatile int disabled_done;

static void *
disabled(void *arg)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 50 * 1000 * 1000 };
    int             old;

    (void)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    if (old != PTHREAD_CANCEL_ENABLE)
        return (void *)1;
    pthread_cleanup_push(record, (void *)6);
    sem_post(&started);
    /* The request stays pending through these cancellation points */
    sem_wait(&blocked);
    nanosleep(&ts, NULL);
    disabled_done = 1;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_cleanup_pop(0);
    return (void *)2;
}

static volatile unsigned long spins;

static void *
spinner(void *arg)
{
    (void)arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    sem_post(&started);
    for (;;)
        spins++;
    return NULL;
}

static void
destructor(void *value)
{
    trace[ntrace++] = (int)(intptr_t)value;
}

static void *
exiting(void *arg)
{
    (void)arg;
    pthread_setspecific(key, (void *)13);
    pthread_cleanup_push(record, (void *)10);
    pthread_cleanup_push(record, (void *)11);
    pthread_cleanup_push(record, (void *)12);
    pthread_cleanup_pop(1); /* runs 12 */
    pthread_cancel(pthread_self());
    pthread_testcancel(); /* runs 11, 10, then the destructor with 13 */
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *
popped(void *arg)
{
    (void)arg;
    pthread_cleanup_push(record, (void *)20);
    pthread_cleanup_pop(0);
    pthread_cleanup_push(record, (void *)21);
    pthread_exit((void *)22);
    pthread_cleanup_pop(0);
    return NULL;
}

static void
cancel_blocked(void *(*func)(void *), void *arg, int expect)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    pthread_t       thread;
    void           *result;

    ntrace = 0;
    check(pthread_create(&thread, NULL, func, arg) == 0, "pthread_create");
    check(sem_wait(&started) == 0, "sem_wait started");
    nanosleep(&ts, NULL); /* let the thread block */
    check(pthread_cancel(thread) == 0, "pthread_cancel");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == PTHREAD_CANCELED, "cancelled result");
    check(ntrace == 1 && trace[0] == expect, "cleanup handler ran");
}

static sem_t never_posted;

static void *
forever(void *arg)
{
    (void)arg;
    for (;;)
        sem_wait(&never_posted);
    return NULL;
}

int
main(void)
{
    pthread_mutexattr_t attr;
    pthread_t           thread, other;
    void               *result;
    int                 old;

    check(sem_init(&started, 0, 0) == 0, "sem_init");
    check(sem_init(&blocked, 0, 0) == 0, "sem_init");
    check(sem_init(&never_posted, 0, 0) == 0, "sem_init");
    check(pipe(pipe_fds) == 0, "pipe");
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) == 0, "settype");
    check(pthread_mutex_init(&lock, &attr) == 0, "pthread_mutex_init");

    /* State and type */
    check(pthread_setcancelstate(-1, &old) == EINVAL, "bad cancel state");
    check(pthread_setcanceltype(-1, &old) == EINVAL, "bad cancel type");
    check(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old) == 0 && old == PTHREAD_CANCEL_ENABLE,
          "default cancel state");
    check(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old) == 0
              && old == PTHREAD_CANCEL_DEFERRED,
          "default cancel type");

    /* Cancelling threads blocked at cancellation points */
    cancel_blocked(cond_waiter, NULL, 1);
    cancel_blocked(sleeper, NULL, 2);
    cancel_blocked(reader, NULL, 3);
    cancel_blocked(sem_waiter, NULL, 4);
    check(pthread_create(&other, NULL, forever, NULL) == 0, "pthread_create forever");
    cancel_blocked(joiner, &other, 5);

    /* Cancellation waits while disabled */
    ntrace = 0;
    check(pthread_create(&thread, NULL, disabled, NULL) == 0, "pthread_create disabled");
    check(sem_wait(&started) == 0, "sem_wait started");
    check(pthread_cancel(thread) == 0, "pthread_cancel disabled");
    check(sem_post(&blocked) == 0, "sem_post");
    check(pthread_join(thread, &result) == 0, "pthread_join disabled");
    check(result == PTHREAD_CANCELED, "cancelled once enabled");
    check(disabled_done, "ran while disabled");
    check(ntrace == 1 && trace[0] == 6, "cleanup after enabling");

    /* Asynchronous cancellation stops a thread anywhere */
    check(pthread_create(&thread, NULL, spinner, NULL) == 0, "pthread_create spinner");
    check(sem_wait(&started) == 0, "sem_wait started");
    check(pthread_cancel(thread) == 0, "pthread_cancel spinner");
    check(pthread_join(thread, &result) == 0, "pthread_join spinner");
    check(result == PTHREAD_CANCELED, "asynchronous cancel");

    /* Cleanup handlers run in reverse order, before thread-specific data destructors */
    check(pthread_key_create(&key, destructor) == 0, "pthread_key_create");
    ntrace = 0;
    check(pthread_create(&thread, NULL, exiting, NULL) == 0, "pthread_create exiting");
    check(pthread_join(thread, &result) == 0, "pthread_join exiting");
    check(result == PTHREAD_CANCELED, "self cancel");
    check(ntrace == 4 && trace[0] == 12 && trace[1] == 11 && trace[2] == 10 && trace[3] == 13,
          "cleanup order");

    /* Popped handlers don't run; pthread_exit runs the rest */
    ntrace = 0;
    check(pthread_create(&thread, NULL, popped, NULL) == 0, "pthread_create popped");
    check(pthread_join(thread, &result) == 0, "pthread_join popped");
    check(result == (void *)22, "pthread_exit value");
    check(ntrace == 1 && trace[0] == 21, "pthread_exit cleanup");

    /* Cancelling a thread which has already finished */
    check(pthread_create(&thread, NULL, popped, NULL) == 0, "pthread_create");
    {
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
        nanosleep(&ts, NULL);
    }
    check(pthread_cancel(thread) == 0, "cancel finished thread");
    check(pthread_join(thread, &result) == 0 && result == (void *)22, "finished thread result");

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
