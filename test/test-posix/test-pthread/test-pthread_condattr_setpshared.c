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
#if TEST_POSIX_THREADS && TEST_POSIX_THREAD_PROCESS_SHARED && TEST_POSIX_BARRIERS   \
    && TEST_POSIX_SPIN_LOCKS && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_MAPPED_FILES

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>

/*
 * Mutexes, condition variables, reader-writer locks, barriers and
 * spin locks with the process-shared attribute, used by two processes
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NLOOPS 20000

struct shared {
    pthread_mutex_t    mutex;
    pthread_mutex_t    errorcheck;
    pthread_cond_t     cond;
    pthread_rwlock_t   rwlock;
    pthread_barrier_t  barrier;
    pthread_spinlock_t spin;
    long               by_mutex, by_spin, by_rwlock_a, by_rwlock_b;
    int                child_ready, parent_ready;
};

static struct shared *shared;

/* What each process does; returns 0 or the number of the failed step */
static int
work(int child)
{
    int ret;

    if ((ret = pthread_barrier_wait(&shared->barrier)) != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
        return 1;

    /* Both count under each kind of lock */
    for (int i = 0; i < NLOOPS; i++) {
        if (pthread_mutex_lock(&shared->mutex) != 0)
            return 2;
        shared->by_mutex++;
        if (pthread_mutex_unlock(&shared->mutex) != 0)
            return 3;
        if (pthread_spin_lock(&shared->spin) != 0)
            return 4;
        shared->by_spin++;
        if (pthread_spin_unlock(&shared->spin) != 0)
            return 5;
        if (pthread_rwlock_wrlock(&shared->rwlock) != 0)
            return 6;
        shared->by_rwlock_a++;
        shared->by_rwlock_b++;
        if (pthread_rwlock_unlock(&shared->rwlock) != 0)
            return 7;
        if (pthread_rwlock_rdlock(&shared->rwlock) != 0)
            return 8;
        if (shared->by_rwlock_a != shared->by_rwlock_b)
            return 9;
        if (pthread_rwlock_unlock(&shared->rwlock) != 0)
            return 10;
    }
    if ((ret = pthread_barrier_wait(&shared->barrier)) != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
        return 11;

    /* Each waits for the other to set its flag */
    if (pthread_mutex_lock(&shared->mutex) != 0)
        return 12;
    if (child)
        shared->child_ready = 1;
    else
        shared->parent_ready = 1;
    if (pthread_cond_broadcast(&shared->cond) != 0)
        return 13;
    while (!(child ? shared->parent_ready : shared->child_ready))
        if (pthread_cond_wait(&shared->cond, &shared->mutex) != 0)
            return 14;
    if (pthread_mutex_unlock(&shared->mutex) != 0)
        return 15;

    /* An error checking mutex held by the other process */
    if (child && pthread_mutex_lock(&shared->errorcheck) != 0)
        return 16;
    if ((ret = pthread_barrier_wait(&shared->barrier)) != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
        return 17;
    if (!child) {
        if (pthread_mutex_trylock(&shared->errorcheck) != EBUSY)
            return 18;
        if (pthread_mutex_unlock(&shared->errorcheck) != EPERM)
            return 19;
        if (pthread_spin_lock(&shared->spin) != 0)
            return 20;
    }
    if ((ret = pthread_barrier_wait(&shared->barrier)) != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
        return 21;
    if (child) {
        /* The parent holds the spin lock now */
        if (pthread_spin_trylock(&shared->spin) != EBUSY)
            return 22;
        if (pthread_mutex_unlock(&shared->errorcheck) != 0)
            return 23;
    }
    if ((ret = pthread_barrier_wait(&shared->barrier)) != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
        return 24;
    if (!child) {
        if (pthread_spin_unlock(&shared->spin) != 0)
            return 25;
        if (pthread_mutex_trylock(&shared->errorcheck) != 0)
            return 26;
        if (pthread_mutex_unlock(&shared->errorcheck) != 0)
            return 27;
    }
    return 0;
}

int
main(void)
{
    pthread_mutexattr_t   mattr;
    pthread_condattr_t    cattr;
    pthread_rwlockattr_t  rattr;
    pthread_barrierattr_t battr;
    pid_t                 pid;
    int                   status, ret, value;

    shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(shared != MAP_FAILED, "mmap");

    check(pthread_mutexattr_init(&mattr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED) == 0, "mutex pshared");
    check(pthread_mutex_init(&shared->mutex, &mattr) == 0, "pthread_mutex_init");
    check(pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK) == 0, "settype");
    check(pthread_mutex_init(&shared->errorcheck, &mattr) == 0, "pthread_mutex_init errorcheck");
    check(pthread_mutexattr_destroy(&mattr) == 0, "pthread_mutexattr_destroy");

    check(pthread_condattr_init(&cattr) == 0, "pthread_condattr_init");
    check(pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED) == 0, "cond pshared");
    check(pthread_condattr_getpshared(&cattr, &value) == 0 && value == PTHREAD_PROCESS_SHARED,
          "cond getpshared");
    check(pthread_cond_init(&shared->cond, &cattr) == 0, "pthread_cond_init");
    check(pthread_condattr_destroy(&cattr) == 0, "pthread_condattr_destroy");

    check(pthread_rwlockattr_init(&rattr) == 0, "pthread_rwlockattr_init");
    check(pthread_rwlockattr_setpshared(&rattr, PTHREAD_PROCESS_SHARED) == 0, "rwlock pshared");
    check(pthread_rwlock_init(&shared->rwlock, &rattr) == 0, "pthread_rwlock_init");
    check(pthread_rwlockattr_destroy(&rattr) == 0, "pthread_rwlockattr_destroy");

    check(pthread_barrierattr_init(&battr) == 0, "pthread_barrierattr_init");
    check(pthread_barrierattr_setpshared(&battr, PTHREAD_PROCESS_SHARED) == 0, "barrier pshared");
    check(pthread_barrier_init(&shared->barrier, &battr, 2) == 0, "pthread_barrier_init");
    check(pthread_barrierattr_destroy(&battr) == 0, "pthread_barrierattr_destroy");

    check(pthread_spin_init(&shared->spin, PTHREAD_PROCESS_SHARED) == 0, "pthread_spin_init");

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(work(1));
    ret = work(0);
    if (ret != 0) {
        /* The child may be waiting at the barrier for the parent */
        printf("parent failed at step %d\n", ret);
        kill(pid, SIGKILL);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        printf("child failed at step %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    check(ret == 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0, "both processes");
    check(shared->by_mutex == 2L * NLOOPS, "mutex counter");
    check(shared->by_spin == 2L * NLOOPS, "spin lock counter");
    check(shared->by_rwlock_a == 2L * NLOOPS, "rwlock counter");

    check(pthread_spin_destroy(&shared->spin) == 0, "pthread_spin_destroy");
    check(pthread_barrier_destroy(&shared->barrier) == 0, "pthread_barrier_destroy");
    check(pthread_rwlock_destroy(&shared->rwlock) == 0, "pthread_rwlock_destroy");
    check(pthread_cond_destroy(&shared->cond) == 0, "pthread_cond_destroy");
    check(pthread_mutex_destroy(&shared->errorcheck) == 0, "pthread_mutex_destroy errorcheck");
    check(pthread_mutex_destroy(&shared->mutex) == 0, "pthread_mutex_destroy");
    check(munmap(shared, sizeof(*shared)) == 0, "munmap");

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
