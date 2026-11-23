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
#if TEST_POSIX_SHELL_FUNC && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_SIGNALS \
    && TEST_POSIX_FILE_SYSTEM

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * system: a null command, "--" before the command, SIGINT and SIGQUIT
 * ignored and SIGCHLD blocked while it waits (and restored after),
 * other children left alone, and an interrupted wait. It needs /bin/sh.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t usr1_seen;
static volatile sig_atomic_t chld_blocked;
static volatile sig_atomic_t int_ignored;
static volatile sig_atomic_t quit_ignored;
static volatile sig_atomic_t chld_seen;

/* The command signals the test while system waits for it */
static void
on_usr1(int sig)
{
    sigset_t         mask;
    struct sigaction sa;

    (void)sig;
    usr1_seen = 1;
    if (sigprocmask(SIG_BLOCK, NULL, &mask) == 0)
        chld_blocked = sigismember(&mask, SIGCHLD) == 1;
    if (sigaction(SIGINT, NULL, &sa) == 0)
        int_ignored = sa.sa_handler == SIG_IGN;
    if (sigaction(SIGQUIT, NULL, &sa) == 0)
        quit_ignored = sa.sa_handler == SIG_IGN;
}

static void
on_chld(int sig)
{
    (void)sig;
    chld_seen = 1;
}

static void
on_int(int sig)
{
    (void)sig;
}

int
main(void)
{
    struct sigaction sa;
    sigset_t         mask;
    pid_t            other;
    int              status;
    int              rc;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }

    /* No core file if SIGQUIT isn't ignored */
    {
        struct rlimit rl;

        check(getrlimit(RLIMIT_CORE, &rl) == 0, "getrlimit");
        rl.rlim_cur = 0;
        check(setrlimit(RLIMIT_CORE, &rl) == 0, "setrlimit");
    }

    /* There is a command processor */
    check(system(NULL) != 0, "system(NULL)");

    /* A command starting with '-' is a command, not a shell option */
    rc = system("-no-such-command-for-system");
    check(rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 127,
          "a command starting with '-' (sh -c -- command)");

    /* The status is that of the shell */
    rc = system("exit 3");
    check(rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 3, "exit status");

    /* While system waits: SIGINT and SIGQUIT ignored, SIGCHLD blocked */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_usr1;
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction SIGUSR1");
    sa.sa_handler = on_chld;
    check(sigaction(SIGCHLD, &sa, NULL) == 0, "sigaction SIGCHLD");
    sa.sa_handler = on_int;
    check(sigaction(SIGINT, &sa, NULL) == 0, "sigaction SIGINT");
    /* A shell starts background jobs with SIGQUIT ignored; start from the default */
    sa.sa_handler = SIG_DFL;
    check(sigaction(SIGQUIT, &sa, NULL) == 0, "sigaction SIGQUIT");

    /* SIGUSR1 interrupts system's wait (no SA_RESTART); it must go on waiting */
    rc = system("kill -USR1 $PPID; kill -INT $PPID; kill -QUIT $PPID; sleep 1; exit 5");
    check(rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 5,
          "the status after SIGUSR1, SIGINT and SIGQUIT");
    check(usr1_seen, "the command signalled the caller");
    check(chld_blocked, "SIGCHLD blocked while waiting");
    check(int_ignored, "SIGINT ignored while waiting");
    check(quit_ignored, "SIGQUIT ignored while waiting");

    /* And restored afterwards */
    check(sigaction(SIGINT, NULL, &sa) == 0 && sa.sa_handler == on_int, "SIGINT restored");
    check(sigaction(SIGQUIT, NULL, &sa) == 0 && sa.sa_handler == SIG_DFL, "SIGQUIT restored");
    check(sigprocmask(SIG_BLOCK, NULL, &mask) == 0 && !sigismember(&mask, SIGCHLD),
          "SIGCHLD unblocked again");

    /* The command gets the caller's dispositions: SIGINT kills it */
    sa.sa_handler = SIG_DFL;
    check(sigaction(SIGINT, &sa, NULL) == 0, "default SIGINT");
    rc = system("kill -INT $$");
    check(rc != -1 && WIFSIGNALED(rc) && WTERMSIG(rc) == SIGINT, "the command's SIGINT");

    /* system doesn't collect other children */
    sa.sa_handler = SIG_DFL;
    check(sigaction(SIGCHLD, &sa, NULL) == 0, "default SIGCHLD");
    other = fork();
    check(other >= 0, "fork");
    if (other == 0)
        _exit(9);
    rc = system("sleep 1; exit 0");
    check(rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0, "system with another child");
    check(waitpid(other, &status, 0) == other && WIFEXITED(status) && WEXITSTATUS(status) == 9,
          "the other child's status is still there");

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
