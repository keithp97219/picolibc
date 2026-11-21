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
/* These are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>

/*
 * getrusage reports CPU time used by the process, and by the children
 * (and their children) that have been waited for, but not by children
 * that weren't, such as those discarded with SIGCHLD ignored
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* CPU time in microseconds */
static long long
usec(int who)
{
    struct rusage ru;

    check(getrusage(who, &ru) == 0, "getrusage");
    return (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000000LL + ru.ru_utime.tv_usec
        + ru.ru_stime.tv_usec;
}

#define BURN 100000 /* 100ms */

/* Use at least BURN microseconds of CPU time */
static void
burn(void)
{
    while (usec(RUSAGE_SELF) < BURN)
        ;
}

static pid_t
burner(int grandchild)
{
    pid_t pid = fork();

    check(pid >= 0, "fork");
    if (pid == 0) {
        if (grandchild) {
            /* The time of a grandchild waited for by its parent counts too */
            pid_t g = burner(0);
            _exit(waitpid(g, NULL, 0) == g ? 0 : 1);
        }
        burn();
        _exit(0);
    }
    return pid;
}

static void
children(void)
{
    struct sigaction sa;
    siginfo_t        info;
    pid_t            pid;
    long long        before;
    int              status;

    /* A new process has no children's time */
    check(usec(RUSAGE_CHILDREN) == 0, "no children's time yet");

    /* A terminated child counts only once it is waited for */
    pid = burner(0);
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "waitid WNOWAIT");
    check(usec(RUSAGE_CHILDREN) == 0, "not counted before the wait");
    check(waitpid(pid, &status, 0) == pid && status == 0, "waitpid");
    before = usec(RUSAGE_CHILDREN);
    check(before >= BURN, "counted after the wait");

    /* So does a grandchild its parent waited for */
    pid = burner(1);
    check(waitpid(pid, &status, 0) == pid && status == 0, "waitpid");
    check(usec(RUSAGE_CHILDREN) >= before + BURN, "grandchild counted");
    before = usec(RUSAGE_CHILDREN);

    /* A child discarded with SIGCHLD ignored never counts */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGCHLD, &sa, NULL) == 0, "ignore SIGCHLD");
    pid = burner(0);
    /* This returns once the child is gone */
    errno = 0;
    check(waitpid(pid, &status, 0) == -1 && errno == ECHILD, "discarded");
    check(usec(RUSAGE_CHILDREN) == before, "a discarded child isn't counted");
    exit(0);
}

int
main(void)
{
    struct rusage ru;
    volatile long i, sum = 0;
    pid_t         pid;
    int           status;

    for (i = 0; i < 20000000; i++)
        sum += i;
    check(sum > 0, "computed");
    check(getrusage(RUSAGE_SELF, &ru) == 0, "RUSAGE_SELF");
    check(ru.ru_utime.tv_usec >= 0 && ru.ru_utime.tv_usec < 1000000, "user time");
    check(ru.ru_stime.tv_usec >= 0 && ru.ru_stime.tv_usec < 1000000, "system time");
    check(ru.ru_utime.tv_sec > 0 || ru.ru_utime.tv_usec > 0 || ru.ru_stime.tv_usec > 0,
          "some time used");
    check(getrusage(RUSAGE_CHILDREN, &ru) == 0, "RUSAGE_CHILDREN");
    check(getrusage(42, &ru) == -1 && errno == EINVAL, "EINVAL");

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        children();
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "children's time");
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
