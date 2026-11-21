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
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../is-qemu.h"

/* Fork handlers, and threads in the child of a multi-threaded process */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char order[16];
static int  norder;

#define HANDLER(name, c)     \
    static void name(void)   \
    {                        \
        order[norder++] = c; \
    }

HANDLER(prepare1, 'a')
HANDLER(prepare2, 'b')
HANDLER(parent1, 'c')
HANDLER(parent2, 'd')
HANDLER(child1, 'e')
HANDLER(child2, 'f')

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int    stop;

/* Keep the C library busy while forking */
static void *
busy(void *arg)
{
    (void)arg;
    while (!stop) {
        void *p = malloc(64);
        free(p);
        printf("%s", "");
    }
    return NULL;
}

static void
lock_prepare(void)
{
    pthread_mutex_lock(&lock);
}

static void
lock_release(void)
{
    pthread_mutex_unlock(&lock);
}

static void *
return_arg(void *arg)
{
    return arg;
}

static int
child_main(void)
{
    pthread_t thread;
    void     *result;
    void     *p;

    /* Handlers: prepare in reverse order, then child in order */
    if (norder != 4 || order[0] != 'b' || order[1] != 'a' || order[2] != 'e' || order[3] != 'f')
        return 1;
    /* Locks released by the child handlers can be taken */
    if (pthread_mutex_trylock(&lock) != 0)
        return 2;
    pthread_mutex_unlock(&lock);
    /* The C library still works */
    p = malloc(100);
    if (!p)
        return 3;
    free(p);
    printf("child output\n");
    fflush(stdout);
    /* Threads work in the child, and signals reach this thread */
    if (pthread_create(&thread, NULL, return_arg, (void *)7) != 0)
        return 4;
    if (pthread_join(thread, &result) != 0 || result != (void *)7)
        return 5;
    if (pthread_kill(pthread_self(), 0) != 0)
        return 6;
    /* With one thread left, pthread_exit ends the child with status 0 */
    pthread_exit(NULL);
}

int
main(void)
{
    pthread_t threads[4];
    pid_t     pid;
    int       status;

    check(pthread_atfork(prepare1, parent1, child1) == 0, "pthread_atfork 1");
    check(pthread_atfork(prepare2, parent2, child2) == 0, "pthread_atfork 2");
    check(pthread_atfork(lock_prepare, lock_release, lock_release) == 0, "pthread_atfork 3");
    check(pthread_atfork(NULL, NULL, NULL) == 0, "pthread_atfork with no handlers");

    for (int i = 0; i < 4; i++)
        check(pthread_create(&threads[i], NULL, busy, NULL) == 0, "pthread_create");

    for (int i = 0; i < 20; i++) {
        norder = 0;
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0)
            _exit(child_main());
        check(norder == 4 && order[0] == 'b' && order[1] == 'a' && order[2] == 'c'
                  && order[3] == 'd',
              "parent handler order");
        check(waitpid(pid, &status, 0) == pid, "waitpid");
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            printf("child status %#x\n", status);
        /* qemu-hexagon aborts itself when a forked child creates a thread */
        if (is_qemu() && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT)
            printf("QEMU aborted the child, not checking its status\n");
        else
            check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
    }

    stop = 1;
    for (int i = 0; i < 4; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join");

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
