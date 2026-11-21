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
/* posix_openpt, grantpt, unlockpt and ptsname are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*
 * tcgetpgrp and tcsetpgrp report and set the foreground process group of a controlling terminal,
 * with SIGTTOU for background callers, EIO for orphaned ones, and none at all after setsid
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Opens a pseudo-terminal pair, or skips the test when there are none */
static int
open_pty(int *subsidiary, int flags)
{
    char *name;
    int   manager = posix_openpt(O_RDWR | O_NOCTTY);

    if (manager < 0) {
        printf("no pseudo-terminals, skipping\n");
        exit(77);
    }
    check(grantpt(manager) == 0, "grantpt");
    check(unlockpt(manager) == 0, "unlockpt");
    name = ptsname(manager);
    check(name != NULL, "ptsname");
    *subsidiary = open(name, O_RDWR | flags);
    check(*subsidiary >= 0, "open subsidiary");
    return manager;
}

static volatile sig_atomic_t ttou;

static void
on_ttou(int sig)
{
    (void)sig;
    ttou = 1;
}

/* Runs fn in a child process, returning its exit status */
static int
in_child(int (*fn)(int, pid_t), int subsidiary, pid_t fg)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    if (pid < 0)
        return 100;
    if (pid == 0)
        _exit(fn(subsidiary, fg));
    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return 101;
    return WEXITSTATUS(status);
}

/* In a background group of the session: SIGTTOU unless it is blocked or ignored */
static int
background(int subsidiary, pid_t fg)
{
    struct sigaction sa;
    sigset_t         set, old;
    pid_t            self;

    if (setpgid(0, 0) != 0)
        return 20;
    self = getpgrp();
    /* A background process may read the foreground group */
    if (tcgetpgrp(subsidiary) != fg)
        return 21;
    /* Changing it sends SIGTTOU; caught, without SA_RESTART, the call fails with EINTR */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_ttou;
    if (sigaction(SIGTTOU, &sa, NULL) != 0)
        return 22;
    errno = 0;
    if (tcsetpgrp(subsidiary, self) != -1 || errno != EINTR || !ttou)
        return 23;
    if (tcgetpgrp(subsidiary) != fg)
        return 24;
    /* With SIGTTOU blocked, it is allowed and no signal comes */
    sigemptyset(&set);
    sigaddset(&set, SIGTTOU);
    if (sigprocmask(SIG_BLOCK, &set, &old) != 0)
        return 25;
    ttou = 0;
    if (tcsetpgrp(subsidiary, self) != 0 || tcgetpgrp(subsidiary) != self)
        return 26;
    if (sigpending(&set) != 0 || sigismember(&set, SIGTTOU))
        return 27;
    if (sigprocmask(SIG_SETMASK, &old, NULL) != 0)
        return 28;
    /* In the foreground, nothing stops it giving the terminal back */
    if (tcsetpgrp(subsidiary, fg) != 0 || tcgetpgrp(subsidiary) != fg || ttou)
        return 29;
    /* With SIGTTOU ignored, it is allowed */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGTTOU, &sa, NULL) != 0)
        return 30;
    if (tcsetpgrp(subsidiary, self) != 0 || tcgetpgrp(subsidiary) != self || ttou)
        return 31;
    if (tcsetpgrp(subsidiary, fg) != 0)
        return 32;
    return 0;
}

/*
 * In a background group that is orphaned: EIO. The group's leader
 * makes a member and exits, so the member's parent is outside the
 * session. The member reports what tcsetpgrp did through a pipe.
 */
static int
orphaned(int subsidiary, pid_t fg)
{
    int           fds[2];
    pid_t         pid;
    int           status;
    char          result;
    struct pollfd pfd;

    (void)fg;
    if (pipe(fds) != 0)
        return 40;
    fflush(stdout);
    pid = fork();
    if (pid < 0)
        return 41;
    if (pid == 0) {
        /* The member must look for this pid, not ask getppid before the fork */
        pid_t           leader = getpid();
        struct timespec nap = { 0, 1000000 };

        if (setpgid(0, 0) != 0)
            _exit(1);
        pid = fork();
        if (pid < 0)
            _exit(1);
        if (pid == 0) {
            while (getppid() == leader)
                nanosleep(&nap, NULL);
            errno = 0;
            result = tcsetpgrp(subsidiary, getpgrp()) == -1 ? (char)errno : 0;
            (void)write(fds[1], &result, 1);
        }
        _exit(0);
    }
    (void)close(fds[1]);
    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 42;
    pfd.fd = fds[0];
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 10000) != 1 || read(fds[0], &result, 1) != 1)
        return 43;
    (void)close(fds[0]);
    /* Linux reports ENOTTY, and glibc passes that on */
#ifdef __GLIBC__
    if (result == ENOTTY)
        result = EIO;
#endif
    if (result != EIO)
        return 44;
    return 0;
}

/* When the foreground group has gone, tcgetpgrp reports a group that doesn't exist */
static int
no_foreground(int subsidiary, pid_t fg)
{
    int   fds[2];
    pid_t pid, value;
    int   status;
    char  c;

    if (pipe(fds) != 0)
        return 50;
    fflush(stdout);
    pid = fork();
    if (pid < 0)
        return 51;
    if (pid == 0) {
        (void)close(fds[1]);
        (void)setpgid(0, 0);
        (void)read(fds[0], &c, 1);
        _exit(0);
    }
    (void)close(fds[0]);
    /* Both set the group, so it exists whichever runs first */
    (void)setpgid(pid, pid);
    if (tcsetpgrp(subsidiary, pid) != 0)
        return 52;
    (void)close(fds[1]);
    if (waitpid(pid, &status, 0) != pid)
        return 53;
    value = tcgetpgrp(subsidiary);
    errno = 0;
    if (value <= 1 || kill(-value, 0) != -1 || errno != ESRCH)
        return 54;
    /* This process is now in the background; take the terminal back */
    signal(SIGTTOU, SIG_IGN);
    if (tcsetpgrp(subsidiary, fg) != 0 || tcgetpgrp(subsidiary) != fg)
        return 55;
    signal(SIGTTOU, SIG_DFL);
    return 0;
}

/* setsid leaves the process with no controlling terminal */
static int
new_session(int subsidiary, pid_t fg)
{
    (void)fg;
    if (setsid() < 0)
        return 60;
    errno = 0;
    if (tcgetpgrp(subsidiary) != -1 || errno != ENOTTY)
        return 61;
    errno = 0;
    if (open("/dev/tty", O_RDWR) != -1)
        return 62;
    return 0;
}

/* Runs in a new session, with the subsidiary as its controlling terminal */
static int
child(void)
{
    int   subsidiary, manager;
    int   fds[2], fd, ret;
    pid_t pgrp;

    if (setsid() < 0)
        return 2;
    manager = open_pty(&subsidiary, 0);
    pgrp = getpgrp();
    if (tcgetpgrp(subsidiary) != pgrp)
        return 3;
    if (tcsetpgrp(subsidiary, pgrp) != 0)
        return 4;

    /* A process group in another session */
    errno = 0;
    if (tcsetpgrp(subsidiary, getppid()) != -1 || errno != EPERM)
        return 5;
    errno = 0;
    if (tcsetpgrp(subsidiary, -1) != -1 || errno != EINVAL)
        return 6;
    /* A group that doesn't exist at all; Linux and glibc say ESRCH */
    errno = 0;
    if (tcsetpgrp(subsidiary, INT_MAX) != -1)
        return 11;
#ifndef __GLIBC__
    if (errno != EPERM)
        return 12;
#endif

    /* Not a terminal */
    if (pipe(fds) != 0)
        return 7;
    errno = 0;
    if (tcgetpgrp(fds[0]) != -1 || errno != ENOTTY)
        return 8;
    errno = 0;
    if (tcsetpgrp(fds[0], pgrp) != -1 || errno != ENOTTY)
        return 9;
    errno = 0;
    if (tcgetpgrp(-1) != -1 || errno != EBADF)
        return 10;
    if ((ret = in_child(background, subsidiary, pgrp)) != 0)
        return ret;
    if ((ret = orphaned(subsidiary, pgrp)) != 0)
        return ret;
    if ((ret = no_foreground(subsidiary, pgrp)) != 0)
        return ret;
    /* The session leader has a controlling terminal; a new session doesn't */
    if ((fd = open("/dev/tty", O_RDWR)) < 0 || close(fd) != 0)
        return 13;
    if ((ret = in_child(new_session, subsidiary, pgrp)) != 0)
        return ret;
    (void)manager;
    return 0;
}

int
main(void)
{
    int   subsidiary, manager = open_pty(&subsidiary, O_NOCTTY);
    pid_t pid;
    int   status;

    /* A terminal that isn't the caller's controlling terminal */
    errno = 0;
    check(tcgetpgrp(subsidiary) == -1 && errno == ENOTTY, "not controlling");
    check(close(subsidiary) == 0 && close(manager) == 0, "close");

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(child());
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
