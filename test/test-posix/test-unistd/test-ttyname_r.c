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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/* ttyname and ttyname_r name the terminal a descriptor refers to */

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

int
main(void)
{
    int  subsidiary, manager = open_pty(&subsidiary, O_NOCTTY);
    char name[256], small[4], *p, *pts;
    int  fds[2];

    pts = strdup(ptsname(manager));
    check(pts != NULL, "ptsname");
    check(ttyname_r(subsidiary, name, sizeof(name)) == 0, "ttyname_r");
    check(strcmp(name, pts) == 0, "names the subsidiary");
    p = ttyname(subsidiary);
    check(p != NULL && strcmp(p, pts) == 0, "ttyname");

    /* The name fits in TTY_NAME_MAX, including its null */
    check(sysconf(_SC_TTY_NAME_MAX) < 0 || strlen(name) < (size_t)sysconf(_SC_TTY_NAME_MAX),
          "TTY_NAME_MAX");
#ifdef TTY_NAME_MAX
    check(strlen(name) < TTY_NAME_MAX, "TTY_NAME_MAX constant");
#endif
#ifndef __GLIBC__
    /* glibc reports ENODEV here */
    check(ttyname_r(subsidiary, name, strlen(pts)) == ERANGE, "no room for the null");
#endif
    check(ttyname_r(subsidiary, name, strlen(pts) + 1) == 0 && strcmp(name, pts) == 0,
          "just enough room");

    /* Too small a buffer */
    memset(small, 'x', sizeof(small));
    check(ttyname_r(subsidiary, small, sizeof(small)) == ERANGE, "ERANGE");

    /* Not a terminal, or not open */
    check(pipe(fds) == 0, "pipe");
    check(ttyname_r(fds[0], name, sizeof(name)) == ENOTTY, "ttyname_r ENOTTY");
    errno = 0;
    check(ttyname(fds[0]) == NULL && errno == ENOTTY, "ttyname ENOTTY");
    check(ttyname_r(-1, name, sizeof(name)) == EBADF, "ttyname_r EBADF");
    errno = 0;
    check(ttyname(-1) == NULL && errno == EBADF, "ttyname EBADF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");

    free(pts);
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
