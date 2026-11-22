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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * abort, in children: the status is that of termination by SIGABRT even
 * when SIGABRT is blocked, ignored, or caught by a handler that
 * returns; a handler that doesn't return keeps the process alive.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

enum how { PLAIN, BLOCKED, IGNORED, RETURNS, EXITS };

static int report[2];

static void
handler_returns(int sig)
{
    char c = 'h';

    (void)sig;
    if (write(report[1], &c, 1) != 1)
        _exit(3);
}

static void
handler_exits(int sig)
{
    (void)sig;
    _exit(42);
}

/* Abort in a child set up as 'how'; return its status and the handler calls */
static int
abort_child(enum how how, int *calls)
{
    pid_t pid;
    int   status;
    char  buf[8];

    check(pipe(report) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit    rl;
        struct sigaction sa = { 0 };
        sigset_t         set;

        /* No core file */
        if (getrlimit(RLIMIT_CORE, &rl) == 0) {
            rl.rlim_cur = 0;
            (void)setrlimit(RLIMIT_CORE, &rl);
        }
        close(report[0]);
        sigemptyset(&sa.sa_mask);
        switch (how) {
        case PLAIN:
            break;
        case BLOCKED:
            sigemptyset(&set);
            sigaddset(&set, SIGABRT);
            if (sigprocmask(SIG_BLOCK, &set, NULL) != 0)
                _exit(2);
            break;
        case IGNORED:
            sa.sa_handler = SIG_IGN;
            if (sigaction(SIGABRT, &sa, NULL) != 0)
                _exit(2);
            break;
        case RETURNS:
            sa.sa_handler = handler_returns;
            if (sigaction(SIGABRT, &sa, NULL) != 0)
                _exit(2);
            break;
        case EXITS:
            sa.sa_handler = handler_exits;
            if (sigaction(SIGABRT, &sa, NULL) != 0)
                _exit(2);
            break;
        }
        abort();
    }
    close(report[1]);
    *calls = 0;
    while (read(report[0], buf, 1) == 1)
        ++*calls;
    close(report[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

int
main(void)
{
    int status;
    int calls;

    status = abort_child(PLAIN, &calls);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "abort");

    status = abort_child(BLOCKED, &calls);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "abort with SIGABRT blocked");

    status = abort_child(IGNORED, &calls);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "abort with SIGABRT ignored");

    status = abort_child(RETURNS, &calls);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "abort with a SIGABRT handler that returns");
    check(calls >= 1, "the handler was called");

    status = abort_child(EXITS, &calls);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 42, "a handler that doesn't return");

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
