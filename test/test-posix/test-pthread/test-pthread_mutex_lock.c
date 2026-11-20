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

#define _GNU_SOURCE /* glibc declares pthread_*_clock* only for _GNU_SOURCE */
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                    \
    || ((defined(_POSIX_THREADS) && _POSIX_THREADS > 0)       \
        && (defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS > 0))

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Relocking, deadlines, unsupported clocks, signals while waiting,
 * attribute independence and the priority ceiling range
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t mutex;

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
init_mutex(pthread_mutex_t *m, int type, int robust)
{
    pthread_mutexattr_t attr;

    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&attr, type) == 0, "pthread_mutexattr_settype");
    if (robust)
        check(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) == 0, "setrobust");
    check(pthread_mutex_init(m, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
}

/* Lock attempts on a mutex another thread holds */
static void *
busy_locks(void *arg)
{
    struct timespec ts;

    (void)arg;

    /* A deadline already passed, or before the epoch, times out at once */
    clock_gettime(CLOCK_REALTIME, &ts);
    if (pthread_mutex_timedlock(&mutex, &ts) != ETIMEDOUT)
        return (void *)1;
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    if (pthread_mutex_timedlock(&mutex, &ts) != ETIMEDOUT)
        return (void *)2;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (pthread_mutex_clocklock(&mutex, CLOCK_MONOTONIC, &ts) != ETIMEDOUT)
        return (void *)3;

    /* The wait lasts until the clock reaches the deadline */
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_mutex_timedlock(&mutex, &ts) != ETIMEDOUT || !reached(CLOCK_REALTIME, &ts))
        return (void *)4;
    timeout_in(CLOCK_MONOTONIC, &ts, 30);
    if (pthread_mutex_clocklock(&mutex, CLOCK_MONOTONIC, &ts) != ETIMEDOUT
        || !reached(CLOCK_MONOTONIC, &ts))
        return (void *)5;
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_mutex_clocklock(&mutex, CLOCK_REALTIME, &ts) != ETIMEDOUT
        || !reached(CLOCK_REALTIME, &ts))
        return (void *)6;

    /* Clocks other than CLOCK_REALTIME and CLOCK_MONOTONIC aren't supported */
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_mutex_clocklock(&mutex, CLOCK_PROCESS_CPUTIME_ID, &ts) != EINVAL)
        return (void *)7;

    /* A negative nanoseconds field is invalid too */
    ts.tv_nsec = -1;
    if (pthread_mutex_timedlock(&mutex, &ts) != EINVAL)
        return (void *)8;
    return NULL;
}

static volatile sig_atomic_t caught;
static int                   locking, locked;

static void
handler(int sig)
{
    (void)sig;
    caught = 1;
}

/* Blocks in pthread_mutex_lock until the main thread unlocks */
static void *
lock_through_signal(void *arg)
{
    int ret;

    (void)arg;
    __atomic_store_n(&locking, 1, __ATOMIC_SEQ_CST);
    ret = pthread_mutex_lock(&mutex);
    __atomic_store_n(&locked, 1, __ATOMIC_SEQ_CST);
    if (ret == 0)
        pthread_mutex_unlock(&mutex);
    return (void *)(intptr_t)ret;
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

int
main(void)
{
    pthread_mutexattr_t attr;
    struct sigaction    act = { 0 };
    struct timespec     ts;
    pthread_t           thread;
    void               *result;
    int                 value, ret;

    /* trylock fails on a mutex the caller holds, whatever the type */
    init_mutex(&mutex, PTHREAD_MUTEX_NORMAL, 0);
    check(pthread_mutex_lock(&mutex) == 0, "lock normal");
    check(pthread_mutex_trylock(&mutex) == EBUSY, "trylock own normal mutex");

    /*
     * Relocking a normal mutex deadlocks, so a timed relock times out,
     * unless the deadlock is detected
     */
    timeout_in(CLOCK_REALTIME, &ts, 30);
    ret = pthread_mutex_timedlock(&mutex, &ts);
    check(ret == ETIMEDOUT || ret == EDEADLK, "timed relock of normal mutex");
    if (ret == ETIMEDOUT)
        check(reached(CLOCK_REALTIME, &ts), "timed relock waited for the deadline");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock normal");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy normal");

    init_mutex(&mutex, PTHREAD_MUTEX_ERRORCHECK, 0);
    check(pthread_mutex_lock(&mutex) == 0, "lock errorcheck");
    check(pthread_mutex_trylock(&mutex) == EBUSY, "trylock own errorcheck mutex");
    timeout_in(CLOCK_REALTIME, &ts, 30);
    check(pthread_mutex_timedlock(&mutex, &ts) == EDEADLK, "timed relock of errorcheck mutex");
    check(pthread_mutex_clocklock(&mutex, CLOCK_MONOTONIC, &ts) == EDEADLK,
          "clock relock of errorcheck mutex");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock errorcheck");

    /* A destroyed mutex can be initialized again */
    check(pthread_mutex_destroy(&mutex) == 0, "destroy errorcheck");
    check(pthread_mutex_init(&mutex, NULL) == 0, "reinitialize");
    check(pthread_mutex_lock(&mutex) == 0, "lock reinitialized");

    /* Deadlines when another thread holds the mutex */
    check(in_thread(busy_locks, NULL) == 0, "busy mutex deadlines");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock reinitialized");

    /* Without waiting, there is no timeout, even with a deadline already passed */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(pthread_mutex_timedlock(&mutex, &ts) == 0, "timedlock free mutex, passed deadline");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");
    check(pthread_mutex_clocklock(&mutex, CLOCK_MONOTONIC, &ts) == 0,
          "clocklock free mutex, passed deadline");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");

    /* A signal doesn't end the wait for a mutex */
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(pthread_mutex_lock(&mutex) == 0, "lock for signal");
    check(pthread_create(&thread, NULL, lock_through_signal, NULL) == 0, "pthread_create");
    while (!__atomic_load_n(&locking, __ATOMIC_SEQ_CST))
        pause_ms(1);
    pause_ms(20);
    check(pthread_kill(thread, SIGUSR1) == 0, "pthread_kill");
    while (!caught)
        pause_ms(1);
    pause_ms(20);
    check(!__atomic_load_n(&locked, __ATOMIC_SEQ_CST), "lock returned after a signal");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock for signal");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == NULL, "lock after a signal");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy");

    /* Changing or destroying the attributes leaves the mutex alone */
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) == 0, "settype");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0, "settype again");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
    check(pthread_mutex_lock(&mutex) == 0, "lock");
    check(pthread_mutex_lock(&mutex) == EDEADLK, "still errorcheck");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy");

    /* A robust normal mutex still deadlocks on relock */
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    ret = pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (ret == 0) {
        check(pthread_mutex_lock(&mutex) == 0, "lock robust");
        timeout_in(CLOCK_REALTIME, &ts, 30);
        ret = pthread_mutex_timedlock(&mutex, &ts);
        check(ret == ETIMEDOUT || ret == EDEADLK, "timed relock of robust normal mutex");
        if (ret == ETIMEDOUT)
            check(reached(CLOCK_REALTIME, &ts), "robust relock waited for the deadline");
        check(pthread_mutex_unlock(&mutex) == 0, "unlock robust");
        check(pthread_mutex_destroy(&mutex) == 0, "destroy robust");
    } else {
        check(ret == ENOTSUP, "robust mutex init");
    }

    /* Priority ceilings span the SCHED_FIFO priorities */
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT) == 0, "setprotocol");
    value = sched_get_priority_min(SCHED_FIFO);
    check(pthread_mutexattr_setprioceiling(&attr, value) == 0, "lowest FIFO priority ceiling");
    value = sched_get_priority_max(SCHED_FIFO);
    check(pthread_mutexattr_setprioceiling(&attr, value) == 0, "highest FIFO priority ceiling");
    check(pthread_mutexattr_setprioceiling(&attr, value + 1) == EINVAL, "ceiling too high");
    check(pthread_mutexattr_getprioceiling(&attr, &value) == 0
              && value == sched_get_priority_max(SCHED_FIFO),
          "ceiling kept");
    check(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) == 0, "settype");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");

    /* Changing the ceiling of an errorcheck mutex the caller holds is EDEADLK */
    ret = pthread_mutex_lock(&mutex);
    check(ret == 0 || ret == EINVAL, "lock priority protected mutex");
    if (ret == 0) {
        check(pthread_mutex_setprioceiling(&mutex, value - 1, &ret) == EDEADLK,
              "setprioceiling of held errorcheck mutex");
        check(pthread_mutex_getprioceiling(&mutex, &ret) == 0 && ret == value,
              "ceiling unchanged after failure");
        check(pthread_mutex_unlock(&mutex) == 0, "unlock priority protected mutex");
    }
    check(pthread_mutex_destroy(&mutex) == 0, "destroy priority protected mutex");

    /* setprioceiling needs a PTHREAD_PRIO_PROTECT mutex */
    check(pthread_mutex_init(&mutex, NULL) == 0, "pthread_mutex_init");
    check(pthread_mutex_setprioceiling(&mutex, value, &ret) == EINVAL,
          "setprioceiling of PRIO_NONE mutex");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy");

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
