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
#include <unistd.h>

/* waitid reports exited, killed, stopped and continued children */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pid_t
child(int status)
{
    pid_t pid = fork();

    check(pid >= 0, "fork");
    if (pid == 0) {
        if (status < 0)
            for (;;)
                pause();
        _exit(status);
    }
    return pid;
}

/* A child that exits with status once a byte arrives on the pipe, or at EOF */
static pid_t
gated_child(int fds[2], int status)
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
    return pid;
}

static void
release(int fds[2])
{
    check(write(fds[1], "x", 1) == 1, "release child");
    close(fds[1]);
}

static volatile sig_atomic_t got_alarm;

static void
on_alarm(int sig)
{
    (void)sig;
    got_alarm = 1;
}

int
main(void)
{
    siginfo_t info;
    pid_t     pid;
    int       status;

    /* A child that exits */
    pid = child(7);
    memset(&info, 0xff, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WEXITED) == 0, "waitid exited");
    check(info.si_pid == pid, "si_pid");
    check(info.si_uid == getuid(), "si_uid");
    check(info.si_signo == SIGCHLD, "si_signo");
    check(info.si_code == CLD_EXITED, "si_code exited");
    check(info.si_status == 7, "si_status exited");

    /* WNOWAIT leaves the child waitable */
    pid = child(9);
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "waitid WNOWAIT");
    check(info.si_pid == pid && info.si_status == 9, "WNOWAIT info");
    check(waitpid(pid, &status, 0) == pid, "waitpid after WNOWAIT");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 9, "status after WNOWAIT");

    /* A child that is stopped, continued and killed */
    pid = child(-1);
    memset(&info, 0xff, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOHANG) == 0, "waitid WNOHANG");
    check(info.si_pid == 0 && info.si_signo == 0, "WNOHANG with no change");

    check(kill(pid, SIGSTOP) == 0, "SIGSTOP");
    check(waitid(P_PID, (id_t)pid, &info, WSTOPPED | WNOWAIT) == 0, "waitid stopped, WNOWAIT");
    check(info.si_code == CLD_STOPPED && info.si_status == SIGSTOP, "stopped info, WNOWAIT");
    /* WNOWAIT left the stop to be reported again */
    check(waitid(P_PID, (id_t)pid, &info, WSTOPPED) == 0, "waitid stopped");
    check(info.si_code == CLD_STOPPED && info.si_status == SIGSTOP, "stopped info");
    /* Once reported, it isn't reported again */
    memset(&info, 0xff, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WSTOPPED | WNOHANG) == 0 && info.si_pid == 0,
          "stop reported once");

    check(kill(pid, SIGCONT) == 0, "SIGCONT");
    check(waitid(P_PID, (id_t)pid, &info, WCONTINUED) == 0, "waitid continued");
    check(info.si_code == CLD_CONTINUED && info.si_status == SIGCONT, "continued info");

    check(kill(pid, SIGTERM) == 0, "SIGTERM");
    check(waitid(P_ALL, 0, &info, WEXITED) == 0, "waitid killed");
    check(info.si_pid == pid, "killed si_pid");
    check(info.si_code == CLD_KILLED && info.si_status == SIGTERM, "killed info");

    /* No children left */
    check(waitid(P_ALL, 0, &info, WEXITED) == -1 && errno == ECHILD, "ECHILD");

    /* Some state to wait for is required */
    check(waitid(P_ALL, 0, &info, WNOHANG) == -1 && errno == EINVAL, "EINVAL");

    {
        int   fds[2];
        pid_t grouped;

        /* With a child to wait for, bad arguments still give EINVAL */
        pid = gated_child(fds, 3);
        errno = 0;
        check(waitid(P_ALL, 0, &info, WEXITED | WNOHANG | 0x100) == -1 && errno == EINVAL,
              "unknown option bit");
#ifndef __GLIBC__
        /* glibc passes 3 to Linux, which takes it as P_PIDFD */
        errno = 0;
        check(waitid((idtype_t)3, 9999, &info, WEXITED | WNOHANG) == -1 && errno == EINVAL,
              "unknown idtype");
        /* glibc passes 0 to Linux, which takes it as the caller's group */
        errno = 0;
        check(waitid(P_PGID, 0, &info, WEXITED | WNOHANG) == -1 && errno == EINVAL,
              "process group 0");
#endif
        /* Not a child */
        errno = 0;
        check(waitid(P_PID, (id_t)getpid(), &info, WEXITED | WNOHANG) == -1 && errno == ECHILD,
              "P_PID of a non-child");

        /* A signal handler interrupts the wait */
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_alarm;
        sigemptyset(&sa.sa_mask);
        check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
        alarm(1);
        errno = 0;
        check(waitid(P_PID, (id_t)pid, &info, WEXITED) == -1 && errno == EINTR && got_alarm,
              "EINTR");
        release(fds);
        check(waitid(P_PID, (id_t)pid, &info, WEXITED) == 0 && info.si_status == 3,
              "reaped after EINTR");

        /* P_PGID selects the children in one process group */
        grouped = gated_child(fds, 4);
        check(setpgid(grouped, grouped) == 0, "setpgid");
        pid = child(5);
        check(waitid(P_PGID, (id_t)getpgrp(), &info, WEXITED) == 0 && info.si_pid == pid,
              "P_PGID own group");
        memset(&info, 0xff, sizeof(info));
        check(waitid(P_PGID, (id_t)getpgrp(), &info, WEXITED | WNOHANG) == -1 && errno == ECHILD,
              "no child left in own group");
        release(fds);
        check(waitid(P_PGID, (id_t)grouped, &info, WEXITED) == 0 && info.si_pid == grouped
                  && info.si_status == 4,
              "P_PGID other group");
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
