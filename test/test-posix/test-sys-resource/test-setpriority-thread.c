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
/* getpriority and setpriority are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * setpriority(PRIO_PROCESS) changes the whole process, so every
 * thread sees the new value, whether who is 0 or the process ID and
 * whichever thread calls it. The changes are made in a child.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int go[2], done[2];
static int base, seen;

static void *
waiter(void *arg)
{
    char c;

    (void)arg;
    /* First report the value main set, then change it from here by process ID */
    if (read(go[0], &c, 1) != 1)
        return NULL;
    seen = getpriority(PRIO_PROCESS, 0);
    if (setpriority(PRIO_PROCESS, (id_t)getpid(), base + 2) != 0)
        seen = -100;
    (void)write(done[1], "x", 1);
    return NULL;
}

static void
child(void)
{
    pthread_t t;
    char      c;

    check(pipe(go) == 0 && pipe(done) == 0, "pipe");
    check(pthread_create(&t, NULL, waiter, NULL) == 0, "pthread_create");

    check(setpriority(PRIO_PROCESS, 0, base + 1) == 0, "setpriority");
    check(getpriority(PRIO_PROCESS, 0) == base + 1, "the caller has the new value");
    check(write(go[1], "x", 1) == 1 && read(done[0], &c, 1) == 1, "handshake");
    check(pthread_join(t, NULL) == 0, "pthread_join");
    check(seen != -100, "setpriority in the thread");
    /* glibc's setpriority changes only one thread */
#ifndef __GLIBC__
    check(seen == base + 1, "the other thread has the new value");
    check(getpriority(PRIO_PROCESS, 0) == base + 2, "the main thread has the thread's value");
#endif
    exit(0);
}

int
main(void)
{
    pid_t pid;
    int   status;

    errno = 0;
    base = getpriority(PRIO_PROCESS, 0);
    check(base != -1 || errno == 0, "getpriority");
    if (base > 17) {
        printf("nice value %d leaves no room, skipping\n", base);
        return 77;
    }
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        child();
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child checks");
    check(getpriority(PRIO_PROCESS, 0) == base, "the parent is unchanged");

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
