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
/* setreuid and setregid are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS && TEST_POSIX_USER_GROUPS && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

/* credentials belong to the process: a change made in one thread holds in every thread, whatever
 * each is doing */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NTHREADS 4

static pthread_mutex_t       lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t        cond = PTHREAD_COND_INITIALIZER;
static volatile sig_atomic_t stop;
static int                   started;
static int                   fds[2];
static uid_t                 uids[NTHREADS], euids[NTHREADS];

static void
note_started(void)
{
    pthread_mutex_lock(&lock);
    started++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

/* Each thread waits a different way, then reports its own credentials */
static void *
waiter(void *arg)
{
    int             i = (int)(long)arg;
    char            c;
    struct timespec nap = { 0, 1000000 };

    note_started();
    switch (i) {
    case 0:
        (void)read(fds[0], &c, 1);
        break;
    case 1:
        pthread_mutex_lock(&lock);
        while (!stop)
            pthread_cond_wait(&cond, &lock);
        pthread_mutex_unlock(&lock);
        break;
    case 2:
        while (!stop)
            ;
        break;
    default:
        while (!stop)
            nanosleep(&nap, NULL);
        break;
    }
    uids[i] = getuid();
    euids[i] = geteuid();
    return NULL;
}

/* Starts and ends short-lived threads, so changes race with both */
static void *
nothing(void *arg)
{
    return arg;
}

/* Spins until cancelled, so it exits from inside the cancellation signal */
static void *
spin(void *arg)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    for (;;)
        ;
    return arg;
}

static void *
churn(void *arg)
{
    pthread_t t;
    int       n = 0;

    (void)arg;
    while (!stop) {
        if (n++ & 1) {
            if (pthread_create(&t, NULL, spin, NULL) == 0) {
                pthread_cancel(t);
                pthread_join(t, NULL);
            }
        } else if (pthread_create(&t, NULL, nothing, NULL) == 0) {
            pthread_join(t, NULL);
        }
    }
    return NULL;
}

/* Changes credentials with the other threads busy; returns 0 if all agree */
static int
run(uid_t target, int real_change)
{
    pthread_t threads[NTHREADS], churner;
    int       i;

    if (pipe(fds) != 0)
        return 2;
    for (i = 0; i < NTHREADS; i++)
        if (pthread_create(&threads[i], NULL, waiter, (void *)(long)i) != 0)
            return 3;
    if (pthread_create(&churner, NULL, churn, NULL) != 0)
        return 4;
    pthread_mutex_lock(&lock);
    while (started < NTHREADS)
        pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);

    if (real_change) {
        if (setuid(target) != 0)
            return 5;
    } else {
        /* Changes to the credentials already held, many times over */
        for (i = 0; i < 200; i++) {
            if (setuid(target) != 0 || seteuid(target) != 0 || setreuid(-1, target) != 0)
                return 6;
            if (setgid(getgid()) != 0 || setegid(getegid()) != 0 || setregid(-1, getegid()) != 0)
                return 7;
        }
    }

    pthread_mutex_lock(&lock);
    stop = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
    if (write(fds[1], "x", 1) != 1)
        return 8;
    for (i = 0; i < NTHREADS; i++)
        pthread_join(threads[i], NULL);
    pthread_join(churner, NULL);
    for (i = 0; i < NTHREADS; i++)
        if (uids[i] != target || euids[i] != target)
            return 10 + i;
    return 0;
}

int
main(void)
{
    pid_t pid;
    int   status;

    if (geteuid() != 0) {
        /* Without privilege nothing can visibly change; check nothing hangs or breaks */
        check(run(getuid(), 0) == 0, "unchanged credentials");
        printf("not root: only unchanged credentials checked\n");
        printf("success\n");
        return 0;
    }

    /* Drop privilege in a child, leaving this process as it was */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(run(65534, 1));
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    if (WEXITSTATUS(status) != 0) {
        printf("child check %d failed\n", WEXITSTATUS(status));
        return 1;
    }
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
