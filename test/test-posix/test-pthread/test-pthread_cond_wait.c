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
#if !defined(__PICOLIBC__)                                    \
    || ((defined(_POSIX_THREADS) && _POSIX_THREADS > 0)       \
        && (defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS > 0))

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Errors checked before the mutex is touched, deadlines, the mutex
 * held again after a timeout, signals while waiting, and a robust
 * mutex whose owner dies while another thread waits
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t errorcheck;
static int             predicate;

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

/* Whether the clock has reached ts */
static int
reached(clockid_t clock, const struct timespec *ts)
{
    struct timespec now;

    clock_gettime(clock, &now);
    return now.tv_sec > ts->tv_sec || (now.tv_sec == ts->tv_sec && now.tv_nsec >= ts->tv_nsec);
}

static void
pause_ms(long ms)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ms * 1000 * 1000 };

    while (nanosleep(&ts, &ts) != 0)
        ;
}

static intptr_t
in_thread(void *(*func)(void *), void *arg)
{
    pthread_t thread;
    void     *result;

    check(pthread_create(&thread, NULL, func, arg) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    return (intptr_t)result;
}

/* Wait on a mutex another thread holds */
static void *
wait_not_owner(void *arg)
{
    struct timespec  ts;
    pthread_mutex_t *m = arg;

    if (pthread_cond_wait(&cond, m) != EPERM)
        return (void *)1;
    timeout_in(CLOCK_REALTIME, &ts, 10);
    if (pthread_cond_timedwait(&cond, m, &ts) != EPERM)
        return (void *)2;
    /* The owner still holds it */
    if (pthread_mutex_trylock(m) != EBUSY)
        return (void *)3;
    return NULL;
}

static volatile sig_atomic_t caught;
static int                   waiting, woken;

static void
handler(int sig)
{
    (void)sig;
    caught = 1;
}

/* Waits for the predicate; every return must be 0 */
static void *
wait_through_signal(void *arg)
{
    int ret = 0;

    (void)arg;
    pthread_mutex_lock(&errorcheck);
    __atomic_store_n(&waiting, 1, __ATOMIC_SEQ_CST);
    while (!predicate && ret == 0)
        ret = pthread_cond_wait(&cond, &errorcheck);
    __atomic_store_n(&woken, 1, __ATOMIC_SEQ_CST);
    if (pthread_mutex_unlock(&errorcheck) != 0)
        return (void *)100;
    return (void *)(intptr_t)ret;
}

#ifdef __PICOLIBC__
static pthread_mutex_t robust;
static int             died, relocking;

#define NRELOCKS 1000000

/*
 * Holds the robust recursive mutex and relocks it repeatedly while
 * another thread's waits fail with EPERM; those must not disturb the
 * lock count
 */
static void *
relock_robust(void *arg)
{
    (void)arg;
    if (pthread_mutex_lock(&robust) != 0)
        return (void *)1;
    __atomic_store_n(&relocking, 1, __ATOMIC_SEQ_CST);
    for (int i = 0; i < NRELOCKS; i++) {
        if (pthread_mutex_lock(&robust) != 0)
            return (void *)2;
        if (pthread_mutex_unlock(&robust) != 0)
            return (void *)3;
    }
    __atomic_store_n(&relocking, 2, __ATOMIC_SEQ_CST);
    /* Still held once */
    if (pthread_mutex_trylock(&robust) != 0 || pthread_mutex_unlock(&robust) != 0)
        return (void *)4;
    if (pthread_mutex_unlock(&robust) != 0)
        return (void *)5;
    if (pthread_mutex_unlock(&robust) != EPERM)
        return (void *)6;
    return NULL;
}

/* Takes the robust mutex once the waiter has released it, then exits holding it */
static void *
die_after_signal(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&robust);
    died = 1;
    pthread_cond_signal(&cond);
    return NULL;
}
#endif

int
main(void)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;
    struct sigaction    act = { 0 };
    struct timespec     ts;
    pthread_t           thread;
    void               *result;
    clockid_t           clock;

    check(pthread_mutexattr_init(&mattr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK) == 0, "settype");
    check(pthread_mutex_init(&errorcheck, &mattr) == 0, "pthread_mutex_init");

    /* An error checking mutex the caller doesn't hold is EPERM */
    timeout_in(CLOCK_REALTIME, &ts, 10);
    check(pthread_cond_wait(&cond, &errorcheck) == EPERM, "wait on free mutex");
    check(pthread_cond_timedwait(&cond, &errorcheck, &ts) == EPERM, "timedwait on free mutex");
    check(pthread_cond_clockwait(&cond, &errorcheck, CLOCK_MONOTONIC, &ts) == EPERM,
          "clockwait on free mutex");
    check(pthread_mutex_unlock(&errorcheck) == EPERM, "mutex still free");
    check(pthread_mutex_lock(&errorcheck) == 0, "lock errorcheck");
    check(in_thread(wait_not_owner, &errorcheck) == 0, "wait on mutex another thread holds");

    /* Invalid deadlines and clocks fail before the mutex is released */
    ts.tv_nsec = -1;
    check(pthread_cond_timedwait(&cond, &errorcheck, &ts) == EINVAL, "negative nanoseconds");
    check(pthread_cond_clockwait(&cond, &errorcheck, CLOCK_MONOTONIC, &ts) == EINVAL,
          "clockwait negative nanoseconds");
    ts.tv_nsec = 1000000000L;
    check(pthread_cond_clockwait(&cond, &errorcheck, CLOCK_REALTIME, &ts) == EINVAL,
          "clockwait nanoseconds too large");
    timeout_in(CLOCK_REALTIME, &ts, 10);
    check(pthread_cond_clockwait(&cond, &errorcheck, CLOCK_PROCESS_CPUTIME_ID, &ts) == EINVAL,
          "unsupported clock");
    check(pthread_mutex_lock(&errorcheck) == EDEADLK, "mutex still held after EINVAL");

    /* Deadlines already passed time out, and the mutex is held again */
    clock_gettime(CLOCK_REALTIME, &ts);
    check(pthread_cond_timedwait(&cond, &errorcheck, &ts) == ETIMEDOUT, "passed deadline");
    check(pthread_mutex_lock(&errorcheck) == EDEADLK, "mutex held after ETIMEDOUT");
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    check(pthread_cond_timedwait(&cond, &errorcheck, &ts) == ETIMEDOUT, "deadline before epoch");
    check(pthread_cond_clockwait(&cond, &errorcheck, CLOCK_MONOTONIC, &ts) == ETIMEDOUT,
          "clockwait deadline before epoch");
    check(in_thread(wait_not_owner, &errorcheck) == 0, "mutex held by the waiter");

    /* The wait lasts until the clock reaches the deadline; the default is CLOCK_REALTIME */
    timeout_in(CLOCK_REALTIME, &ts, 30);
    while (pthread_cond_timedwait(&cond, &errorcheck, &ts) == 0)
        ;
    check(reached(CLOCK_REALTIME, &ts), "timedwait waited for the deadline");
    timeout_in(CLOCK_MONOTONIC, &ts, 30);
    while (pthread_cond_clockwait(&cond, &errorcheck, CLOCK_MONOTONIC, &ts) == 0)
        ;
    check(reached(CLOCK_MONOTONIC, &ts), "clockwait waited for the monotonic deadline");
    timeout_in(CLOCK_REALTIME, &ts, 30);
    while (pthread_cond_clockwait(&cond, &errorcheck, CLOCK_REALTIME, &ts) == 0)
        ;
    check(reached(CLOCK_REALTIME, &ts), "clockwait waited for the realtime deadline");

    /* A condition variable using CLOCK_MONOTONIC */
    check(pthread_condattr_init(&cattr) == 0, "pthread_condattr_init");
    check(pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC) == 0, "setclock");
    check(pthread_condattr_setclock(&cattr, CLOCK_PROCESS_CPUTIME_ID) == EINVAL,
          "setclock CPU-time clock");
    check(pthread_condattr_getclock(&cattr, &clock) == 0 && clock == CLOCK_MONOTONIC,
          "failed setclock left the clock alone");
    {
        pthread_cond_t mono;

        check(pthread_cond_init(&mono, &cattr) == 0, "pthread_cond_init");
        /* Destroying the attributes leaves the condition variable alone */
        check(pthread_condattr_destroy(&cattr) == 0, "pthread_condattr_destroy");
        timeout_in(CLOCK_MONOTONIC, &ts, 30);
        while (pthread_cond_timedwait(&mono, &errorcheck, &ts) == 0)
            ;
        check(reached(CLOCK_MONOTONIC, &ts), "monotonic timedwait waited for the deadline");
        check(pthread_cond_destroy(&mono) == 0, "pthread_cond_destroy");
    }
    check(pthread_mutex_unlock(&errorcheck) == 0, "unlock errorcheck");

    /* A signal doesn't end the wait with an error */
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(pthread_create(&thread, NULL, wait_through_signal, NULL) == 0, "pthread_create");
    while (!__atomic_load_n(&waiting, __ATOMIC_SEQ_CST))
        pause_ms(1);
    /* Once the waiter has released the mutex, it is waiting */
    check(pthread_mutex_lock(&errorcheck) == 0, "lock for signal");
    check(pthread_mutex_unlock(&errorcheck) == 0, "unlock for signal");
    check(pthread_kill(thread, SIGUSR1) == 0, "pthread_kill");
    while (!caught)
        pause_ms(1);
    pause_ms(20);
    check(!__atomic_load_n(&woken, __ATOMIC_SEQ_CST), "wait ended after a signal");
    check(pthread_mutex_lock(&errorcheck) == 0, "lock to set predicate");
    predicate = 1;
    check(pthread_cond_signal(&cond) == 0, "pthread_cond_signal");
    check(pthread_mutex_unlock(&errorcheck) == 0, "unlock after signal");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == NULL, "every wait returned 0");

#ifdef __PICOLIBC__
    /*
     * A robust mutex whose owner died while this thread waited is
     * acquired with EOWNERDEAD. Picolibc only: glibc can't tell when
     * the kernel has no robust list (QEMU user mode) and then waits
     * forever.
     */
    check(pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST) == 0, "setrobust");
    if (pthread_mutex_init(&robust, &mattr) == 0) {
        int ret = 0;

        check(pthread_mutex_lock(&robust) == 0, "lock robust");
        check(pthread_create(&thread, NULL, die_after_signal, NULL) == 0, "pthread_create");
        while (!died && ret == 0)
            ret = pthread_cond_wait(&cond, &robust);
        check(ret == EOWNERDEAD, "wait returns EOWNERDEAD");
        check(pthread_join(thread, NULL) == 0, "pthread_join");
        check(pthread_mutex_consistent(&robust) == 0, "the waiter owns the mutex");
        check(pthread_mutex_unlock(&robust) == 0, "unlock robust");

        /* Waiting on a robust mutex the caller doesn't hold is EPERM */
        check(pthread_cond_wait(&cond, &robust) == EPERM, "wait on free robust mutex");
        check(pthread_mutex_destroy(&robust) == 0, "destroy robust");
    }

    /* ... without touching the mutex another thread holds */
    check(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) == 0, "settype recursive");
    if (pthread_mutex_init(&robust, &mattr) == 0) {
        check(pthread_create(&thread, NULL, relock_robust, NULL) == 0, "pthread_create");
        while (__atomic_load_n(&relocking, __ATOMIC_SEQ_CST) == 0)
            pause_ms(1);
        while (__atomic_load_n(&relocking, __ATOMIC_SEQ_CST) == 1)
            check(pthread_cond_wait(&cond, &robust) == EPERM,
                  "wait on robust mutex held elsewhere");
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == NULL, "lock count of the owner kept");
        check(pthread_mutex_destroy(&robust) == 0, "destroy robust recursive");
    }
#endif

    check(pthread_mutexattr_destroy(&mattr) == 0, "pthread_mutexattr_destroy");
    check(pthread_mutex_destroy(&errorcheck) == 0, "destroy errorcheck");
    check(pthread_cond_destroy(&cond) == 0, "pthread_cond_destroy");

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
