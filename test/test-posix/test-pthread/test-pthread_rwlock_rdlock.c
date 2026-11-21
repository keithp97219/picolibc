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
/* glibc declares pthread_rwlock_clockwrlock and pthread_rwlock_clockrdlock only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if _POSIX_READER_WRITER_LOCKS > 0 && _POSIX_THREADS > 0

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Lock states after unlocking, relocking by the writer, deadlines,
 * and signals while waiting
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

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

/* What another thread can take: 0 free, 1 read locked, 2 write locked */
static void *
probe(void *arg)
{
    intptr_t state;

    (void)arg;
    if (pthread_rwlock_trywrlock(&rwlock) == 0) {
        pthread_rwlock_unlock(&rwlock);
        return (void *)0;
    }
    if (pthread_rwlock_tryrdlock(&rwlock) == 0) {
        pthread_rwlock_unlock(&rwlock);
        state = 1;
    } else {
        state = 2;
    }
    return (void *)state;
}

/* Deadlines while the main thread holds a write lock */
static void *
busy_locks(void *arg)
{
    struct timespec ts;

    (void)arg;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (pthread_rwlock_timedrdlock(&rwlock, &ts) != ETIMEDOUT)
        return (void *)1;
    if (pthread_rwlock_timedwrlock(&rwlock, &ts) != ETIMEDOUT)
        return (void *)2;
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    if (pthread_rwlock_timedrdlock(&rwlock, &ts) != ETIMEDOUT)
        return (void *)3;
    if (pthread_rwlock_clockwrlock(&rwlock, CLOCK_MONOTONIC, &ts) != ETIMEDOUT)
        return (void *)4;

    /* The wait lasts until the clock reaches the deadline */
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_rwlock_timedrdlock(&rwlock, &ts) != ETIMEDOUT || !reached(CLOCK_REALTIME, &ts))
        return (void *)5;
    timeout_in(CLOCK_MONOTONIC, &ts, 30);
    if (pthread_rwlock_clockrdlock(&rwlock, CLOCK_MONOTONIC, &ts) != ETIMEDOUT
        || !reached(CLOCK_MONOTONIC, &ts))
        return (void *)6;
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_rwlock_timedwrlock(&rwlock, &ts) != ETIMEDOUT || !reached(CLOCK_REALTIME, &ts))
        return (void *)7;
    timeout_in(CLOCK_REALTIME, &ts, 30);
    if (pthread_rwlock_clockwrlock(&rwlock, CLOCK_REALTIME, &ts) != ETIMEDOUT
        || !reached(CLOCK_REALTIME, &ts))
        return (void *)8;
    return NULL;
}

/* With the lock free, an invalid deadline is never a timeout */
static void *
free_locks(void *arg)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
    int             ret;

    (void)arg;
    if (pthread_rwlock_timedrdlock(&rwlock, &ts) != 0)
        return (void *)1;
    pthread_rwlock_unlock(&rwlock);
    if (pthread_rwlock_clockwrlock(&rwlock, CLOCK_MONOTONIC, &ts) != 0)
        return (void *)2;
    pthread_rwlock_unlock(&rwlock);
    ts.tv_nsec = 1000000000L;
    ret = pthread_rwlock_timedwrlock(&rwlock, &ts);
    if (ret == 0)
        pthread_rwlock_unlock(&rwlock);
    else if (ret != EINVAL)
        return (void *)3;
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

/* Blocks until the main thread releases its lock */
static void *
lock_through_signal(void *arg)
{
    int ret;

    __atomic_store_n(&locking, 1, __ATOMIC_SEQ_CST);
    if (arg)
        ret = pthread_rwlock_wrlock(&rwlock);
    else
        ret = pthread_rwlock_rdlock(&rwlock);
    __atomic_store_n(&locked, 1, __ATOMIC_SEQ_CST);
    if (ret == 0)
        pthread_rwlock_unlock(&rwlock);
    return (void *)(intptr_t)ret;
}

static void
signal_while_waiting(int write)
{
    pthread_t thread;
    void     *result;

    locking = locked = 0;
    caught = 0;
    check(pthread_rwlock_wrlock(&rwlock) == 0, "wrlock for signal");
    check(pthread_create(&thread, NULL, lock_through_signal, (void *)(intptr_t)write) == 0,
          "pthread_create");
    while (!__atomic_load_n(&locking, __ATOMIC_SEQ_CST))
        pause_ms(1);
    pause_ms(20);
    check(pthread_kill(thread, SIGUSR1) == 0, "pthread_kill");
    while (!caught)
        pause_ms(1);
    pause_ms(20);
    check(!__atomic_load_n(&locked, __ATOMIC_SEQ_CST), "lock returned after a signal");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock for signal");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == NULL, "lock after a signal");
}

int
main(void)
{
    struct sigaction act = { 0 };
    struct timespec  ts;
    int              ret;

    /* The statically initialized lock starts unlocked */
    check(in_thread(probe, NULL) == 0, "initializer unlocked");

    /* Releasing one of several read locks leaves it read locked */
    check(pthread_rwlock_rdlock(&rwlock) == 0, "rdlock 1");
    check(pthread_rwlock_rdlock(&rwlock) == 0, "rdlock 2");
    check(in_thread(probe, NULL) == 1, "read locked");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock 1");
    check(in_thread(probe, NULL) == 1, "still read locked");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock 2");
    check(in_thread(probe, NULL) == 0, "unlocked after the last reader");

    /* Releasing the write lock unlocks it */
    check(pthread_rwlock_wrlock(&rwlock) == 0, "wrlock");
    check(in_thread(probe, NULL) == 2, "write locked");

    /* The writer can't take the lock again: EDEADLK or deadlock */
    check(pthread_rwlock_trywrlock(&rwlock) != 0, "trywrlock by the writer");
    check(pthread_rwlock_tryrdlock(&rwlock) != 0, "tryrdlock by the writer");
    timeout_in(CLOCK_REALTIME, &ts, 30);
    ret = pthread_rwlock_timedwrlock(&rwlock, &ts);
    check(ret == EDEADLK || ret == ETIMEDOUT, "timedwrlock by the writer");
    ret = pthread_rwlock_timedrdlock(&rwlock, &ts);
    check(ret == EDEADLK || ret == ETIMEDOUT, "timedrdlock by the writer");

    /* Deadlines while it is busy */
    check(in_thread(busy_locks, NULL) == 0, "busy lock deadlines");
    check(pthread_rwlock_unlock(&rwlock) == 0, "unlock write");
    check(in_thread(probe, NULL) == 0, "unlocked after the writer");

    /* Without waiting, there is no timeout */
    check(in_thread(free_locks, NULL) == 0, "free lock deadlines");

    /* A signal doesn't end the wait for the lock */
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    signal_while_waiting(0);
    signal_while_waiting(1);

    /* A destroyed lock can be initialized again */
    check(pthread_rwlock_destroy(&rwlock) == 0, "pthread_rwlock_destroy");
    check(pthread_rwlock_init(&rwlock, NULL) == 0, "pthread_rwlock_init");
    check(in_thread(probe, NULL) == 0, "reinitialized unlocked");
    check(pthread_rwlock_destroy(&rwlock) == 0, "pthread_rwlock_destroy");

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
