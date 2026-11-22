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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_SPECIFIC >= 202405L && TEST_POSIX_PIPE && TEST_POSIX_MULTI_PROCESS \
    && TEST_POSIX_SIGNALS

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#if TEST_VARIANCE_CTTY_TIOCSCTTY
#include <sys/ioctl.h>
#endif
#include <time.h>
#include <unistd.h>

/*
 * tcsetwinsize sets the size tcgetwinsize reports, and a change sends
 * SIGWINCH to the foreground process group
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

#ifdef __PICOLIBC__
static void
on_winch(int sig)
{
    (void)sig;
}

/* Whether a blocked signal is pending; it is taken if it is */
static int
pending(int sig)
{
    sigset_t        set;
    struct timespec ts = { 0 };

    sigemptyset(&set);
    sigaddset(&set, sig);
    return sigtimedwait(&set, NULL, &ts) == sig;
}

/*
 * In a new session with the pseudo-terminal as its controlling terminal,
 * so this process is in the foreground process group
 */
static int
session(const char *name)
{
    struct winsize ws, got;
    sigset_t       set;
    int            fd;

    if (setsid() == (pid_t)-1)
        return 77;
    fd = open(name, O_RDWR);
#if TEST_VARIANCE_CTTY_TIOCSCTTY
    /* Opening it doesn't make it the controlling terminal; TIOCSCTTY does */
    check(fd >= 0 && ioctl(fd, TIOCSCTTY, 0) == 0, "TIOCSCTTY");
#endif
    if (fd < 0 || tcgetsid(fd) != getpid())
        return 77;
    /*
     * SIGWINCH is ignored by default, and an ignored signal may be
     * discarded even while blocked (FreeBSD's is), so catch it
     */
    check(signal(SIGWINCH, on_winch) != SIG_ERR, "catch SIGWINCH");
    sigemptyset(&set);
    sigaddset(&set, SIGWINCH);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block SIGWINCH");

    check(tcgetwinsize(fd, &ws) == 0, "tcgetwinsize");
    ws.ws_row = (unsigned short)(ws.ws_row == 33 ? 34 : 33);
    check(tcsetwinsize(fd, &ws) == 0, "change the size");
    check(pending(SIGWINCH), "a change sends SIGWINCH");
    check(tcsetwinsize(fd, &ws) == 0, "the same size again");
    check(!pending(SIGWINCH), "no SIGWINCH for the same size");
    check(tcgetwinsize(fd, &got) == 0 && got.ws_row == ws.ws_row && got.ws_col == ws.ws_col,
          "size set");
    check(close(fd) == 0, "close");
    return 0;
}
#endif

int
main(void)
{
#ifdef __PICOLIBC__
    struct winsize ws = { .ws_row = 24, .ws_col = 80 }, got;
    int            subsidiary, manager = open_pty(&subsidiary, O_NOCTTY);
    int            fds[2];

    check(tcsetwinsize(subsidiary, &ws) == 0, "tcsetwinsize");
    memset(&got, 0, sizeof(got));
    check(tcgetwinsize(subsidiary, &got) == 0, "tcgetwinsize");
    check(got.ws_row == 24 && got.ws_col == 80, "size");

    /* Either end of the pair sees it */
    ws.ws_row = 50;
    ws.ws_col = 132;
    check(tcsetwinsize(manager, &ws) == 0, "tcsetwinsize manager");
    check(tcgetwinsize(subsidiary, &got) == 0 && got.ws_row == 50 && got.ws_col == 132,
          "size from manager");

    check(pipe(fds) == 0, "pipe");
    check(tcgetwinsize(fds[0], &got) == -1 && errno == ENOTTY, "tcgetwinsize ENOTTY");
    check(tcsetwinsize(fds[0], &ws) == -1 && errno == ENOTTY, "tcsetwinsize ENOTTY");
    check(tcgetwinsize(-1, &got) == -1 && errno == EBADF, "EBADF");
    check(tcsetwinsize(-1, &ws) == -1 && errno == EBADF, "tcsetwinsize EBADF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");
    /* SIGWINCH, checked in a child that makes the terminal its controlling terminal */
    {
        char *name = ptsname(manager);
        pid_t pid;
        int   status;

        check(name != NULL, "ptsname");
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            int ret = session(name);

            fflush(stdout);
            _exit(ret);
        }
        check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
        if (WEXITSTATUS(status) == 77)
            printf("no controlling terminal, SIGWINCH not checked\n");
        else
            check(WEXITSTATUS(status) == 0, "SIGWINCH checks");
    }

    check(close(subsidiary) == 0 && close(manager) == 0, "close");
    printf("success\n");
    return 0;
#else
    /* POSIX.1-2024 added these; glibc doesn't have them yet */
    (void)open_pty;
    printf("no tcgetwinsize, skipping\n");
    return 77;
#endif
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
