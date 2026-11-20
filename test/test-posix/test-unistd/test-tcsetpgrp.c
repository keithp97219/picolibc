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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                  \
    || ((defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC) && __PICOLIBC_POSIX_DEVICE_SPECIFIC > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/* tcgetpgrp and tcsetpgrp report and set the foreground process group of a controlling terminal */

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

/* Runs in a new session, with the subsidiary as its controlling terminal */
static int
child(void)
{
    int   subsidiary, manager;
    int   fds[2];
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
