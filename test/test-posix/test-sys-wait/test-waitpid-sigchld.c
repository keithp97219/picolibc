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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_SIGNALS

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 * Linux doesn't queue SIGCHLD. Then, with SIGCHLD blocked, a wait or
 * waitpid that returns a child's status clears a pending SIGCHLD
 * unless another child's status is available.
 *
 * Whether a blocked signal whose action is to ignore it stays pending
 * is unspecified, and SIGCHLD's default action is to ignore it
 * (FreeBSD discards it), so the test catches it, with a handler that
 * never runs while it is blocked.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* A child that exits at once, waited for without collecting its status */
static pid_t
exited_child(int status)
{
    siginfo_t info;
    pid_t     pid = fork();

    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(status);
    /* The SIGCHLD is pending by the time the status can be seen */
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "waitid WNOWAIT");
    return pid;
}

static void
on_sigchld(int sig)
{
    (void)sig;
}

static int
sigchld_pending(void)
{
    sigset_t set;

    check(sigpending(&set) == 0, "sigpending");
    return sigismember(&set, SIGCHLD);
}

int
main(void)
{
    struct sigaction act = { .sa_handler = on_sigchld };
    sigset_t         block;
    pid_t            a, b;
    int              status;

    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    check(sigprocmask(SIG_BLOCK, &block, NULL) == 0, "block SIGCHLD");
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGCHLD, &act, NULL) == 0, "catch SIGCHLD");

    /* One child: waitpid clears the signal */
    a = exited_child(1);
    check(sigchld_pending() == 1, "SIGCHLD pending");
    check(waitpid(a, &status, 0) == a && WEXITSTATUS(status) == 1, "waitpid");
#ifndef __GLIBC__
    /* glibc leaves it pending, as Linux does */
    check(sigchld_pending() == 0, "cleared by waitpid");
#endif

    /* Two children: the signal stays until the second is collected */
    a = exited_child(2);
    b = exited_child(3);
    check(waitpid(a, &status, 0) == a && WEXITSTATUS(status) == 2, "first waitpid");
    check(sigchld_pending() == 1, "still pending for the other child");
    check(wait(&status) == b && WEXITSTATUS(status) == 3, "wait");
#ifndef __GLIBC__
    check(sigchld_pending() == 0, "cleared by wait");
#endif

    /* With nothing left, a SIGCHLD that is pending without a status stays */
    check(kill(getpid(), SIGCHLD) == 0, "raise SIGCHLD");
    errno = 0;
    check(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD, "ECHILD");
    check(sigchld_pending() == 1, "a failed wait leaves SIGCHLD");

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
