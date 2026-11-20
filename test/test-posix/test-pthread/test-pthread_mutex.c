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
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Mutex types, try and timed locking, and mutex attributes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t mutex;

static void *
try_lock(void *arg)
{
    int ret;

    (void)arg;
    ret = pthread_mutex_trylock(&mutex);
    if (ret == 0)
        pthread_mutex_unlock(&mutex);
    return (void *)(intptr_t)ret;
}

static void *
unlock(void *arg)
{
    (void)arg;
    return (void *)(intptr_t)pthread_mutex_unlock(&mutex);
}

static void *
timed_lock(void *arg)
{
    struct timespec ts;
    clockid_t       clock = (clockid_t)(intptr_t)arg;
    int             ret;

    clock_gettime(clock, &ts);
    ts.tv_nsec += 50 * 1000 * 1000;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    }
    if (clock == CLOCK_REALTIME)
        ret = pthread_mutex_timedlock(&mutex, &ts);
    else
        ret = pthread_mutex_clocklock(&mutex, clock, &ts);
    if (ret == 0)
        pthread_mutex_unlock(&mutex);
    return (void *)(intptr_t)ret;
}

static void *
bad_timed_lock(void *arg)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000000L };

    (void)arg;
    return (void *)(intptr_t)pthread_mutex_timedlock(&mutex, &ts);
}

static int
in_thread(void *(*func)(void *), void *arg)
{
    pthread_t thread;
    void     *result;

    check(pthread_create(&thread, NULL, func, arg) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    return (int)(intptr_t)result;
}

static void
init_mutex(int type)
{
    pthread_mutexattr_t attr;
    int                 get;

    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_settype(&attr, type) == 0, "pthread_mutexattr_settype");
    check(pthread_mutexattr_gettype(&attr, &get) == 0 && get == type, "pthread_mutexattr_gettype");
    check(pthread_mutex_init(&mutex, &attr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
}

int
main(void)
{
    pthread_mutexattr_t attr;
    struct timespec     ts;
    int                 value;

    /* Attribute defaults and validation */
    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_gettype(&attr, &value) == 0, "pthread_mutexattr_gettype");
    check(value == PTHREAD_MUTEX_DEFAULT, "default type");
    check(pthread_mutexattr_getpshared(&attr, &value) == 0, "pthread_mutexattr_getpshared");
    check(value == PTHREAD_PROCESS_PRIVATE, "default pshared");
    check(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) == 0, "setpshared");
    check(pthread_mutexattr_getpshared(&attr, &value) == 0 && value == PTHREAD_PROCESS_SHARED,
          "getpshared");
    check(pthread_mutexattr_settype(&attr, -1) == EINVAL, "bad mutex type");
    check(pthread_mutexattr_setpshared(&attr, -1) == EINVAL, "bad pshared");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");

    /* Static initializer, trylock from another thread */
    {
        pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
        mutex = init;
    }
    check(pthread_mutex_lock(&mutex) == 0, "lock default");
    check(in_thread(try_lock, NULL) == EBUSY, "trylock held default mutex");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock default");
    check(in_thread(try_lock, NULL) == 0, "trylock free default mutex");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy default");

    /* Recursive mutexes */
    init_mutex(PTHREAD_MUTEX_RECURSIVE);
    check(pthread_mutex_lock(&mutex) == 0, "recursive lock 1");
    check(pthread_mutex_lock(&mutex) == 0, "recursive lock 2");
    check(pthread_mutex_trylock(&mutex) == 0, "recursive trylock");
    check(in_thread(try_lock, NULL) == EBUSY, "recursive mutex held by another thread");
    check(in_thread(unlock, NULL) == EPERM, "unlock recursive mutex owned by another thread");
    check(pthread_mutex_unlock(&mutex) == 0, "recursive unlock 1");
    check(pthread_mutex_unlock(&mutex) == 0, "recursive unlock 2");
    check(in_thread(try_lock, NULL) == EBUSY, "recursive mutex still held");
    check(pthread_mutex_unlock(&mutex) == 0, "recursive unlock 3");
    check(pthread_mutex_unlock(&mutex) == EPERM, "recursive unlock of free mutex");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy recursive");

    /* Error checking mutexes */
    init_mutex(PTHREAD_MUTEX_ERRORCHECK);
    check(pthread_mutex_unlock(&mutex) == EPERM, "errorcheck unlock of free mutex");
    check(pthread_mutex_lock(&mutex) == 0, "errorcheck lock");
    check(pthread_mutex_lock(&mutex) == EDEADLK, "errorcheck relock");
    check(in_thread(unlock, NULL) == EPERM, "unlock errorcheck mutex owned by another thread");
    check(pthread_mutex_unlock(&mutex) == 0, "errorcheck unlock");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy errorcheck");

    /* Timed locks time out while another thread holds the mutex */
    init_mutex(PTHREAD_MUTEX_NORMAL);
    check(pthread_mutex_lock(&mutex) == 0, "lock normal");
    check(in_thread(timed_lock, (void *)(intptr_t)CLOCK_REALTIME) == ETIMEDOUT,
          "timedlock times out");
    check(in_thread(timed_lock, (void *)(intptr_t)CLOCK_MONOTONIC) == ETIMEDOUT,
          "clocklock times out");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock normal");
    check(in_thread(timed_lock, (void *)(intptr_t)CLOCK_REALTIME) == 0, "timedlock free mutex");
    check(in_thread(timed_lock, (void *)(intptr_t)CLOCK_MONOTONIC) == 0, "clocklock free mutex");

    /* An invalid timeout is only an error when the lock is busy */
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000000L;
    check(pthread_mutex_timedlock(&mutex, &ts) == 0, "timedlock free mutex, bad timeout");
    check(in_thread(bad_timed_lock, NULL) == EINVAL, "timedlock busy mutex, bad timeout");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock normal");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy normal");

    printf("success\n");
    return 0;
}
