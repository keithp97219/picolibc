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
#if TEST_POSIX_THREADS && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_MAPPED_FILES

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
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

/*
 * Waits for the mutex; the first to get it after its owner died
 * unlocks it without making it consistent, which makes it
 * unrecoverable for the others, waiting or not
 */
static void *
abandon(void *arg)
{
    int ret = pthread_mutex_lock(arg);

    if (ret == EOWNERDEAD || ret == 0)
        pthread_mutex_unlock(arg);
    return (void *)(long)ret;
}

/* Waits for the mutex, recovering it when its owner died */
static void *
recover(void *arg)
{
    int ret = pthread_mutex_lock(arg);

    if (ret == EOWNERDEAD && pthread_mutex_consistent(arg) != 0)
        ret = -1;
    if (ret == EOWNERDEAD || ret == 0)
        pthread_mutex_unlock(arg);
    return (void *)(long)ret;
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
#if TEST_POSIX_THREAD_PRIO_INHERIT
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

#define NWAITERS 4

/* Threads waiting when the owner dies: one gets EOWNERDEAD, the rest ENOTRECOVERABLE */
static void
unrecoverable_waiters(pthread_mutex_t *mutex)
{
    pthread_t thread, waiters[NWAITERS];
    void     *result;
    int       dead = 0, unrecoverable = 0;

    holding = 0;
    check(pthread_create(&thread, NULL, die_with_waiter, mutex) == 0, "pthread_create");
    while (!holding)
        sched_yield();
    for (int i = 0; i < NWAITERS; i++)
        check(pthread_create(&waiters[i], NULL, abandon, mutex) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    for (int i = 0; i < NWAITERS; i++) {
        check(pthread_join(waiters[i], &result) == 0, "pthread_join");
        if ((long)result == EOWNERDEAD)
            dead++;
        else if ((long)result == ENOTRECOVERABLE)
            unrecoverable++;
    }
    check(dead == 1 && unrecoverable == NWAITERS - 1, "waiters after an abandoned mutex");
    check(pthread_mutex_lock(mutex) == ENOTRECOVERABLE, "lock after waiters");
    check(pthread_mutex_destroy(mutex) == 0, "destroy");
}

/*
 * A process-shared mutex whose owning process is killed while a thread
 * of another process waits for it
 */
static void
killed_owner(void)
{
    struct timespec delay = { 0, 50 * 1000 * 1000 };
    pthread_t       thread;
    void           *result;
    pid_t           pid;
    int             ready[2], status;
    char            c;

    check(pipe(ready) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (pthread_mutex_lock(shared) != 0)
            _exit(1);
        if (write(ready[1], "x", 1) != 1)
            _exit(1);
        for (;;)
            pause();
    }
    check(read(ready[0], &c, 1) == 1, "child locked");
    check(pthread_create(&thread, NULL, recover, shared) == 0, "pthread_create");
    nanosleep(&delay, NULL);
    check(kill(pid, SIGKILL) == 0, "kill");
    check(waitpid(pid, &status, 0) == pid && WIFSIGNALED(status), "child killed");
    check(pthread_join(thread, &result) == 0 && (long)result == EOWNERDEAD, "waiter EOWNERDEAD");
    close(ready[0]);
    close(ready[1]);
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

    /* Waiters learn of it too */
    init_robust(&mutex, 0);
    unrecoverable_waiters(&mutex);

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
    killed_owner();
    check(pthread_mutex_lock(shared) == 0, "recovered across processes");
    check(pthread_mutex_unlock(shared) == 0, "unlock");

    /* A process-shared mutex whose owning thread exits */
    orphan(shared);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (pthread_mutex_lock(shared) != EOWNERDEAD)
            _exit(1);
        if (pthread_mutex_unlock(shared) != 0)
            _exit(2);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child EOWNERDEAD");
    check(pthread_mutex_lock(shared) == ENOTRECOVERABLE, "process ENOTRECOVERABLE");
    check(pthread_mutex_destroy(shared) == 0, "destroy");

    /* Waiters in this process, on a process-shared mutex */
    init_robust(shared, 1);
    unrecoverable_waiters(shared);

    /* A new mutex in the same place works */
    init_robust(shared, 1);
    check(pthread_mutex_lock(shared) == 0 && pthread_mutex_unlock(shared) == 0, "reinitialized");
    check(pthread_mutex_destroy(shared) == 0 && munmap(shared, sizeof(*shared)) == 0, "cleanup");

    check(pthread_mutex_consistent(&plain) == EINVAL, "not robust");
}

/*
 * pthread_mutex_init may need memory for a robust mutex, and must then
 * report ENOMEM, not fail later
 */
static void
out_of_memory(int pshared)
{
    struct rlimit   limit = { 64 * 1024 * 1024, 64 * 1024 * 1024 };
    pthread_mutex_t mutex, *m = &mutex;
    pid_t           pid;
    int             status;

    if (pshared) {
        m = mmap(NULL, sizeof(*m), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        check(m != MAP_FAILED, "mmap");
    }
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pthread_mutexattr_t attr;
        void               *list = NULL, *p;
        size_t              size = 1024 * 1024;
        int                 ret;

        if (pthread_mutexattr_init(&attr) != 0
            || pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != 0
            || pthread_mutexattr_setpshared(
                   &attr, pshared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE)
                != 0)
            _exit(1);
        if (setrlimit(RLIMIT_AS, &limit) != 0)
            _exit(77);
        /* Use up the memory */
        while (size >= sizeof(void *)) {
            p = malloc(size);
            if (p) {
                *(void **)p = list;
                list = p;
            } else {
                size /= 2;
            }
        }
        ret = pthread_mutex_init(m, &attr);
        if (ret == ENOMEM)
            _exit(0);
        if (ret != 0)
            _exit(2);
        /* It needed no memory */
        if (pthread_mutex_lock(m) != 0 || pthread_mutex_unlock(m) != 0
            || pthread_mutex_destroy(m) != 0)
            _exit(3);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "out of memory child");
    if (WEXITSTATUS(status) == 77)
        printf("can't limit the address space; not checking ENOMEM\n");
    else if (WEXITSTATUS(status) != 0) {
        printf("out of memory, %s: child status %d\n", pshared ? "shared" : "private",
               WEXITSTATUS(status));
        exit(1);
    }
    if (pshared)
        munmap(m, sizeof(*m));
}

int
main(void)
{
    protocol = PTHREAD_PRIO_NONE;
    run();
#if TEST_POSIX_THREAD_PRIO_INHERIT
    /* Priority inheritance mutexes are robust in a different way */
    protocol = PTHREAD_PRIO_INHERIT;
    run();
#endif
    out_of_memory(0);
    out_of_memory(1);
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
