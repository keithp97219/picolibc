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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS) \
    && defined(__PICOLIBC_POSIX_PIPE) && _POSIX_JOB_CONTROL > 0

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* waitpid reports the chosen child's exit, stop and continuation, with or without waiting */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Exactly one of the status macros holds */
#define one_of(status)                                                                 \
    ((WIFEXITED(status) != 0) + (WIFSIGNALED(status) != 0) + (WIFSTOPPED(status) != 0) \
         + (WIFCONTINUED(status) != 0)                                                 \
     == 1)

/* A child that exits with status once the pipe is closed */
static pid_t
gated_child(int fds[2], int status, int own_group)
{
    pid_t pid;
    char  c;

    check(pipe(fds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(fds[1]);
        (void)read(fds[0], &c, 1);
        _exit(status);
    }
    close(fds[0]);
    /* Before the child can exit, so there's no race */
    if (own_group)
        check(setpgid(pid, pid) == 0, "setpgid");
    return pid;
}

static void
groups(void)
{
    int   fds_a[2], fds_b[2];
    pid_t a, b;
    int   status;

    /* a is in a group of its own, b in ours */
    a = gated_child(fds_a, 1, 1);
    b = gated_child(fds_b, 2, 0);
    close(fds_a[1]);
    close(fds_b[1]);

    /* pid 0 waits only for children in the caller's group */
    check(waitpid(0, &status, 0) == b && WIFEXITED(status) && WEXITSTATUS(status) == 2,
          "waitpid(0)");
    errno = 0;
    check(waitpid(0, &status, WNOHANG) == -1 && errno == ECHILD, "no child in our group");
    errno = 0;
    check(waitpid(-getpgrp(), &status, WNOHANG) == -1 && errno == ECHILD,
          "no child in our group by number");

    /* pid < -1 waits for children in group -pid */
    check(waitpid(-a, &status, 0) == a && WIFEXITED(status) && WEXITSTATUS(status) == 1,
          "waitpid(-pgid)");
}

static void
statuses(void)
{
    siginfo_t info;
    pid_t     pid;
    int       status;

    /* 0 only for a child that exited with 0; the low 8 bits of the exit value */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0);
    check(waitpid(pid, &status, 0) == pid && status == 0 && one_of(status), "exit 0");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0x1ff);
    check(waitpid(pid, &status, 0) == pid && status != 0 && WIFEXITED(status)
              && WEXITSTATUS(status) == 0xff && one_of(status),
          "exit 0x1ff");

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        for (;;)
            pause();
    }

    /* A stop isn't reported without WUNTRACED */
    check(kill(pid, SIGSTOP) == 0, "SIGSTOP");
    check(waitid(P_PID, (id_t)pid, &info, WSTOPPED | WNOWAIT) == 0, "stopped");
    check(waitpid(pid, &status, WNOHANG) == 0, "stop without WUNTRACED");
    check(waitpid(pid, &status, WNOHANG | WCONTINUED) == 0, "stop with only WCONTINUED");

    /* Continuing replaces the unreported stop */
    check(kill(pid, SIGCONT) == 0, "SIGCONT");
    check(waitid(P_PID, (id_t)pid, &info, WCONTINUED | WNOWAIT) == 0, "continued");
    status = 0;
    check(waitpid(pid, &status, WUNTRACED | WCONTINUED) == pid && WIFCONTINUED(status)
              && one_of(status),
          "only the continuation is reported");
    check(waitpid(pid, &status, WUNTRACED | WCONTINUED | WNOHANG) == 0, "and only once");

    /* Stopped and killed statuses satisfy exactly one macro */
    check(kill(pid, SIGSTOP) == 0, "SIGSTOP again");
    check(waitpid(pid, &status, WUNTRACED | WCONTINUED) == pid && WIFSTOPPED(status)
              && one_of(status),
          "stopped status");
    check(kill(pid, SIGKILL) == 0, "SIGKILL");
    check(waitpid(pid, &status, WUNTRACED | WCONTINUED) == pid && WIFSIGNALED(status)
              && WTERMSIG(status) == SIGKILL && one_of(status),
          "killed status");
#ifdef WCOREDUMP
    /* SIGKILL doesn't dump core (glibc defines WCOREDUMP only with _DEFAULT_SOURCE) */
    check(!WCOREDUMP(status), "no core from SIGKILL");
#endif
}

static volatile sig_atomic_t got_alarm;

static void
on_alarm(int sig)
{
    (void)sig;
    got_alarm = 1;
}

/* A signal handler interrupts waitpid */
static void
interrupted(void)
{
    struct sigaction sa;
    int              fds[2], status;
    pid_t            pid;

    pid = gated_child(fds, 6, 0);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    alarm(1);
    errno = 0;
    check(waitpid(pid, &status, 0) == -1 && errno == EINTR && got_alarm, "EINTR");
    close(fds[1]);
    check(waitpid(pid, &status, 0) == pid && WEXITSTATUS(status) == 6, "reaped after EINTR");
}

int
main(void)
{
    pid_t pid, other;
    int   status = 12345;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        for (;;)
            pause();
    }

    /* WNOHANG with nothing to report returns 0 and leaves status alone */
    check(waitpid(pid, &status, WNOHANG) == 0 && status == 12345, "WNOHANG");

    /* Stopped and continued children, with the signal that stopped them */
    check(kill(pid, SIGSTOP) == 0, "SIGSTOP");
    check(waitpid(pid, &status, WUNTRACED) == pid, "WUNTRACED");
    check(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP, "stopped");
    check(!WIFEXITED(status) && !WIFSIGNALED(status), "only stopped");
    check(kill(pid, SIGCONT) == 0, "SIGCONT");
    check(waitpid(pid, &status, WCONTINUED) == pid && WIFCONTINUED(status), "continued");

    /* Only the child asked for is reported */
    other = fork();
    check(other >= 0, "fork");
    if (other == 0)
        _exit(9);
    check(kill(pid, SIGUSR1) == 0, "SIGUSR1");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1, "killed by SIGUSR1");

    /* A null status pointer is allowed */
    check(waitpid(other, NULL, 0) == other, "null status");

    errno = 0;
    check(waitpid(-1, &status, 0) == -1 && errno == ECHILD, "ECHILD");
    errno = 0;
    check(waitpid(getpid(), &status, 0) == -1 && errno == ECHILD, "not a child");
    errno = 0;
    check(waitpid(-1, &status, 0x100) == -1 && errno == EINVAL, "bad options");

    groups();
    statuses();
    interrupted();
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
