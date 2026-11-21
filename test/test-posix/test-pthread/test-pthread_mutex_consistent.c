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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && _POSIX_MAPPED_FILES > 0

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

/* robust mutexes report owners that die, and pthread_mutex_consistent recovers them */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t *shared;
static int              protocol;
static volatile int     holding;

/* Locks the mutex and exits still holding it */
static void *
die_holding(void *arg)
{
    pthread_mutex_lock(arg);
    return NULL;
}

/* Locks the recursive mutex twice and exits still holding it */
static void *
die_holding_twice(void *arg)
{
    pthread_mutex_lock(arg);
    pthread_mutex_lock(arg);
    return NULL;
}

static pthread_mutex_t several[4];

/*
 * Locks several mutexes, unlocks the middle, newest and oldest ones,
 * and exits holding the rest; the thread's list must still name them
 */
static void *
die_holding_some(void *arg)
{
    (void)arg;
    for (int i = 0; i < 4; i++)
        pthread_mutex_lock(&several[i]);
    pthread_mutex_unlock(&several[1]);
    pthread_mutex_unlock(&several[3]);
    pthread_mutex_unlock(&several[0]);
    pthread_mutex_lock(&several[3]);
    return NULL;
}

/* Locks the mutex, waits for another thread to block on it, and exits */
static void *
die_with_waiter(void *arg)
{
    struct timespec pause = { 0, 100 * 1000 * 1000 };

    pthread_mutex_lock(arg);
    holding = 1;
    nanosleep(&pause, NULL);
    return NULL;
}

static void *
timed_lock(void *arg)
{
    struct timespec abstime;
    int             ret;

    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_nsec += 10 * 1000 * 1000;
    if (abstime.tv_nsec >= 1000000000L) {
        abstime.tv_nsec -= 1000000000L;
        abstime.tv_sec++;
    }
    ret = pthread_mutex_timedlock(arg, &abstime);
    if (ret == 0)
        pthread_mutex_unlock(arg);
    return (void *)(long)ret;
}

static void *
try_lock(void *arg)
{
    int ret = pthread_mutex_trylock(arg);

    if (ret == 0)
        pthread_mutex_unlock(arg);
    return (void *)(long)ret;
}

static void
init_robust_type(pthread_mutex_t *mutex, int pshared, int type)
{
    pthread_mutexattr_t attr;
    int                 robust = -1, ret;

    check(pthread_mutexattr_init(&attr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_getrobust(&attr, &robust) == 0 && robust == PTHREAD_MUTEX_STALLED,
          "default");
    check(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) == 0, "setrobust");
    check(pthread_mutexattr_getrobust(&attr, &robust) == 0 && robust == PTHREAD_MUTEX_ROBUST,
          "getrobust");
    check(pthread_mutexattr_setrobust(&attr, 42) == EINVAL, "bad value");
    if (pshared)
        check(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) == 0, "pshared");
    check(pthread_mutexattr_settype(&attr, type) == 0, "settype");
#if _POSIX_THREAD_PRIO_INHERIT > 0
    if (protocol != PTHREAD_PRIO_NONE)
        check(pthread_mutexattr_setprotocol(&attr, protocol) == 0, "setprotocol");
#endif
    ret = pthread_mutex_init(mutex, &attr);
    if (ret == ENOTSUP) {
        printf("robust mutexes not supported here, skipping\n");
        exit(77);
    }
    check(ret == 0, "pthread_mutex_init");
    check(pthread_mutexattr_destroy(&attr) == 0, "pthread_mutexattr_destroy");
}

static void
init_robust(pthread_mutex_t *mutex, int pshared)
{
    init_robust_type(mutex, pshared, PTHREAD_MUTEX_DEFAULT);
}

static void
orphan(pthread_mutex_t *mutex)
{
    pthread_t thread;

    check(pthread_create(&thread, NULL, die_holding, mutex) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
}

static void *
not_owner(void *arg)
{
    return (void *)(long)pthread_mutex_unlock(arg);
}

static void
run(void)
{
    pthread_mutex_t mutex, plain = PTHREAD_MUTEX_INITIALIZER;
    pthread_t       thread;
    void           *result;
    pid_t           pid;
    int             status;

    init_robust(&mutex, 0);

    /* Other threads wait for, and time out on, a held mutex */
    check(pthread_mutex_lock(&mutex) == 0, "lock");
    check(pthread_create(&thread, NULL, try_lock, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && (long)result == EBUSY, "trylock EBUSY");
    check(pthread_create(&thread, NULL, timed_lock, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && (long)result == ETIMEDOUT, "ETIMEDOUT");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");
    check(pthread_create(&thread, NULL, timed_lock, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && (long)result == 0, "timedlock");

    /* A thread waiting when the owner dies is woken with the lock */
    holding = 0;
    check(pthread_create(&thread, NULL, die_with_waiter, &mutex) == 0, "pthread_create");
    while (!holding)
        sched_yield();
    check(pthread_mutex_lock(&mutex) == EOWNERDEAD, "waiter EOWNERDEAD");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(pthread_mutex_consistent(&mutex) == 0 && pthread_mutex_unlock(&mutex) == 0, "recover");

    /* Recovered */
    orphan(&mutex);
    check(pthread_mutex_lock(&mutex) == EOWNERDEAD, "EOWNERDEAD");
    check(pthread_mutex_consistent(&mutex) == 0, "pthread_mutex_consistent");
    check(pthread_mutex_consistent(&mutex) == EINVAL, "already consistent");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");
    check(pthread_mutex_lock(&mutex) == 0, "usable again");

    /* Only the owner may unlock a robust mutex */
    check(pthread_create(&thread, NULL, not_owner, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && (long)result == EPERM, "EPERM");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock");

    /* trylock also reports the dead owner */
    orphan(&mutex);
    check(pthread_mutex_trylock(&mutex) == EOWNERDEAD, "trylock EOWNERDEAD");
    check(pthread_mutex_consistent(&mutex) == 0 && pthread_mutex_unlock(&mutex) == 0, "recover");

    /* Not recovered */
    orphan(&mutex);
    check(pthread_mutex_lock(&mutex) == EOWNERDEAD, "EOWNERDEAD again");
    check(pthread_mutex_unlock(&mutex) == 0, "unlock without recovering");
    check(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE, "ENOTRECOVERABLE");
    check(pthread_mutex_trylock(&mutex) == ENOTRECOVERABLE, "trylock ENOTRECOVERABLE");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy");

    /* A recursive mutex's new owner doesn't inherit the dead owner's depth */
    init_robust_type(&mutex, 0, PTHREAD_MUTEX_RECURSIVE);
    check(pthread_create(&thread, NULL, die_holding_twice, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(pthread_mutex_lock(&mutex) == EOWNERDEAD, "recursive EOWNERDEAD");
    check(pthread_mutex_consistent(&mutex) == 0 && pthread_mutex_unlock(&mutex) == 0, "recover");
    check(pthread_create(&thread, NULL, try_lock, &mutex) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && (long)result == 0, "released by one unlock");
    check(pthread_mutex_destroy(&mutex) == 0, "destroy");

    /* Unlocking in any order leaves the other held mutexes on the list */
    for (int i = 0; i < 4; i++)
        init_robust(&several[i], 0);
    check(pthread_create(&thread, NULL, die_holding_some, NULL) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(pthread_mutex_lock(&several[0]) == 0, "oldest released");
    check(pthread_mutex_lock(&several[1]) == 0, "middle released");
    check(pthread_mutex_lock(&several[2]) == EOWNERDEAD, "held EOWNERDEAD");
    check(pthread_mutex_lock(&several[3]) == EOWNERDEAD, "relocked EOWNERDEAD");
    for (int i = 0; i < 4; i++) {
        if (i >= 2)
            check(pthread_mutex_consistent(&several[i]) == 0, "recover");
        check(pthread_mutex_unlock(&several[i]) == 0, "unlock");
        check(pthread_mutex_destroy(&several[i]) == 0, "destroy");
    }

    /* A process-shared mutex whose owning process exits */
    shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(shared != MAP_FAILED, "mmap");
    init_robust(shared, 1);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (pthread_mutex_lock(shared) != 0)
            _exit(1);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    check(pthread_mutex_lock(shared) == EOWNERDEAD, "process EOWNERDEAD");
    check(pthread_mutex_consistent(shared) == 0 && pthread_mutex_unlock(shared) == 0, "recover");
    check(pthread_mutex_destroy(shared) == 0 && munmap(shared, sizeof(*shared)) == 0, "cleanup");

    check(pthread_mutex_consistent(&plain) == EINVAL, "not robust");
}

int
main(void)
{
    protocol = PTHREAD_PRIO_NONE;
    run();
#if _POSIX_THREAD_PRIO_INHERIT > 0
    /* Priority inheritance mutexes are robust in a different way */
    protocol = PTHREAD_PRIO_INHERIT;
    run();
#endif
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
