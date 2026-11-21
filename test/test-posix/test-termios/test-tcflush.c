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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * tcflush discards data written to a terminal but not yet sent, or
 * received but not yet read
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

/* Waits until input reaches the terminal, which a pseudo-terminal passes on later */
static void
wait_input(int fd)
{
    struct pollfd pfd = { .fd = fd, .events = POLLIN };

    check(poll(&pfd, 1, 5000) == 1, "input arrives");
}

/* Reads what's waiting without blocking */
static ssize_t
read_now(int fd, char *buf, size_t size)
{
    int     flags = fcntl(fd, F_GETFL);
    ssize_t ret;

    check(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0, "O_NONBLOCK");
    ret = read(fd, buf, size);
    check(fcntl(fd, F_SETFL, flags) == 0, "restore flags");
    return ret;
}

int
main(void)
{
    int            subsidiary, manager = open_pty(&subsidiary);
    struct termios t;
    char           buf[16];
    int            fds[2];

    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    check(tcsetattr(subsidiary, TCSANOW, &t) == 0, "tcsetattr");

    check(write(manager, "ab", 2) == 2, "input");
    wait_input(subsidiary);
    check(tcflush(subsidiary, TCIFLUSH) == 0, "TCIFLUSH");
    check(read_now(subsidiary, buf, sizeof(buf)) == -1 && errno == EAGAIN, "input flushed");

    check(write(manager, "cd", 2) == 2, "more input");
    wait_input(subsidiary);
    check(tcflush(subsidiary, TCIOFLUSH) == 0, "TCIOFLUSH");
    check(read_now(subsidiary, buf, sizeof(buf)) == -1 && errno == EAGAIN, "both flushed");

    /* Output reaches a pseudo-terminal's other end at once, leaving nothing to see go */
    check(tcflush(subsidiary, TCOFLUSH) == 0, "TCOFLUSH");

    /* Input written after a flush still arrives */
    check(write(manager, "e", 1) == 1, "input after");
    check(read(subsidiary, buf, sizeof(buf)) == 1 && buf[0] == 'e', "read after");

    errno = 0;
    check(tcflush(subsidiary, 42) == -1 && errno == EINVAL, "bad queue_selector");
    check(pipe(fds) == 0, "pipe");
    check(tcflush(fds[0], TCIFLUSH) == -1 && errno == ENOTTY, "ENOTTY");
    check(tcflush(-1, TCIFLUSH) == -1 && errno == EBADF, "EBADF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");
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
