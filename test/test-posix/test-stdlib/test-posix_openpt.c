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
#if defined(__PICOLIBC_POSIX_DEVICE_SPECIFIC) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/*
 * posix_openpt opens a pseudo-terminal manager with the flags it is
 * given, and fails with EMFILE when no descriptor is free
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
open_pty(int *subsidiary)
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
    *subsidiary = open(name, O_RDWR | O_NOCTTY);
    check(*subsidiary >= 0, "open subsidiary");
    return manager;
}

int
main(void)
{
    int   subsidiary, manager = open_pty(&subsidiary);
    int   other, next, status;
    pid_t pid;

    check(isatty(manager) && isatty(subsidiary), "terminals");
    check(write(manager, "hi\n", 3) == 3, "write manager");
#ifdef __PICOLIBC__
    /* POSIX allows, but doesn't require, rejecting other flags */
    check(posix_openpt(O_RDWR | O_TRUNC) == -1 && errno == EINVAL, "bad flag");
    errno = 0;
    check(posix_openpt(O_RDWR | O_SEARCH) == -1 && errno == EINVAL, "O_SEARCH");
    errno = 0;
    check(posix_openpt(O_EXEC) == -1 && errno == EINVAL, "O_EXEC");
#endif

    /* The access mode and FD_CLOEXEC follow oflag */
    check((fcntl(manager, F_GETFL) & O_ACCMODE) == O_RDWR, "O_RDWR");
    check(!(fcntl(manager, F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC");
    other = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    check(other >= 0, "O_CLOEXEC");
    check(fcntl(other, F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC set");
    check(isatty(other), "a terminal with O_CLOEXEC");
    check(close(other) == 0, "close");

    /* The lowest free descriptor */
    other = dup(0);
    check(other >= 0 && close(other) == 0, "lowest free descriptor");
    next = posix_openpt(O_RDWR | O_NOCTTY);
    check(next == other, "lowest descriptor used");
    check(close(next) == 0, "close");

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl;

        /* With O_NOCTTY a session leader gets no controlling terminal */
        if (setsid() == (pid_t)-1)
            _exit(1);
        other = posix_openpt(O_RDWR | O_NOCTTY);
        if (other < 0)
            _exit(2);
        errno = 0;
        if (tcgetsid(other) != (pid_t)-1 || errno != ENOTTY)
            _exit(3);

        /* EMFILE once the descriptors allowed are all open */
        if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
            _exit(4);
        rl.rlim_cur = (rlim_t)other + 1;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
            _exit(5);
        errno = 0;
        if (posix_openpt(O_RDWR | O_NOCTTY) != -1 || errno != EMFILE)
            _exit(6);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    if (WEXITSTATUS(status) != 0)
        printf("child check %d\n", WEXITSTATUS(status));
    check(WEXITSTATUS(status) == 0, "O_NOCTTY and EMFILE");
    check(close(subsidiary) == 0 && close(manager) == 0, "close");
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
