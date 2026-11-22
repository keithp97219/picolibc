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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_SIGNALS && TEST_POSIX_PIPE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/*
 * wait collects any child's status, fails when there are none, is
 * interrupted by signal handlers, and children of a process that
 * exits get another parent
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t got_alarm;

static void
on_alarm(int sig)
{
    (void)sig;
    got_alarm = 1;
}

static void
interrupted(void)
{
    struct sigaction sa;
    int              fds[2], status;
    pid_t            pid;
    char             c;

    check(pipe(fds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(fds[1]);
        (void)read(fds[0], &c, 1);
        _exit(4);
    }
    close(fds[0]);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    alarm(1);
    errno = 0;
    check(wait(&status) == -1 && errno == EINTR && got_alarm, "EINTR");
    close(fds[1]);
    check(wait(&status) == pid && WEXITSTATUS(status) == 4, "reaped after EINTR");
}

/* When a process exits, its children get a new parent */
static void
orphaned(void)
{
    int   go[2], report[2], status;
    pid_t pid, parent;
    char  c;

    check(pipe(go) == 0 && pipe(report) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        /* Taken before the fork: getppid in the grandchild may already show the new parent */
        parent = getpid();
        if (fork() == 0) {
            if (read(go[0], &c, 1) != 1)
                _exit(1);
            c = getppid() != parent;
            (void)write(report[1], &c, 1);
            _exit(0);
        }
        _exit(0);
    }
    close(go[0]);
    close(report[1]);
    check(waitpid(pid, &status, 0) == pid && status == 0, "parent exited");
    /* Once the parent is reaped, the grandchild has been given to another */
    check(write(go[1], "x", 1) == 1, "go");
    check(read(report[0], &c, 1) == 1 && c == 1, "the orphan has a new parent");
    close(go[1]);
    close(report[0]);
}

int
main(void)
{
    pid_t pid, a, b, got;
    int   status, seen = 0;

    errno = 0;
    check(wait(&status) == -1 && errno == ECHILD, "ECHILD");

    a = fork();
    check(a >= 0, "fork");
    if (a == 0)
        _exit(7);
    b = fork();
    check(b >= 0, "fork");
    if (b == 0) {
        for (;;)
            pause();
    }
    check(kill(b, SIGTERM) == 0, "kill");

    /* Both children are reported, in whatever order they finish */
    while (seen != 3) {
        got = wait(&status);
        if (got == a) {
            check(WIFEXITED(status) && WEXITSTATUS(status) == 7, "exit status");
            seen |= 1;
        } else if (got == b) {
            check(WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM, "signal status");
            seen |= 2;
        } else {
            check(0, "unexpected child");
        }
    }

    /* A null status pointer is allowed */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0);
    check(wait(NULL) == pid, "wait(NULL)");
    errno = 0;
    check(wait(NULL) == -1 && errno == ECHILD, "ECHILD again");

    interrupted();
    orphaned();
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
