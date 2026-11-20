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

/* unlockpt lets the subsidiary be opened */

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
    int   fds[2];
    int   manager = posix_openpt(O_RDWR | O_NOCTTY);
    char *name;

    if (manager < 0) {
        printf("no pseudo-terminals, skipping\n");
        return 77;
    }
    check(grantpt(manager) == 0, "grantpt");
    name = ptsname(manager);
    check(name != NULL, "ptsname");
    /* Locked until unlockpt */
    check(open(name, O_RDWR | O_NOCTTY) == -1, "locked");
    check(unlockpt(manager) == 0, "unlockpt");
    fds[0] = open(name, O_RDWR | O_NOCTTY);
    check(fds[0] >= 0, "open after unlockpt");
    check(close(fds[0]) == 0, "close");
    check(pipe(fds) == 0, "pipe");
    check(unlockpt(fds[0]) == -1 && errno == EINVAL, "EINVAL");
    check(unlockpt(-1) == -1 && errno == EBADF, "EBADF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0 && close(manager) == 0, "close");
    (void)open_pty;
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
