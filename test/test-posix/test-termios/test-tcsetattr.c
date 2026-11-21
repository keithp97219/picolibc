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
 * tcsetattr sets what tcgetattr reports and the terminal then behaves
 * that way; the speed functions set the speeds it holds
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
    struct termios t, got;
    char           buf[16];
    int            fds[2];

    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");

    /* Each flag set is reported back; a pseudo-terminal keeps its own hardware settings */
    t.c_iflag &= ~(IXON | INLCR | IGNCR);
    t.c_iflag |= ICRNL;
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ICANON | ECHO | ISIG);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    t.c_cc[VINTR] = 'C' & 0x1f;
    check(cfsetispeed(&t, B9600) == 0 && cfsetospeed(&t, B9600) == 0, "set speeds");
    check(cfgetispeed(&t) == B9600 && cfgetospeed(&t) == B9600, "get speeds");
    check(tcsetattr(subsidiary, TCSANOW, &t) == 0, "TCSANOW");
    memset(&got, 0, sizeof(got));
    check(tcgetattr(subsidiary, &got) == 0, "tcgetattr again");
    check((got.c_iflag & (IXON | INLCR | IGNCR | ICRNL)) == ICRNL, "c_iflag");
    check(!(got.c_oflag & OPOST), "c_oflag");
    check(!(got.c_lflag & (ICANON | ECHO | ISIG)), "c_lflag");
    check(got.c_cc[VMIN] == 1 && got.c_cc[VTIME] == 0, "VMIN and VTIME");
    check(got.c_cc[VINTR] == ('C' & 0x1f), "VINTR");

    /* Without ICANON input is available at once; ICRNL maps carriage return */
    check(write(manager, "a\r", 2) == 2, "write input");
    check(read(subsidiary, buf, sizeof(buf)) == 2 && memcmp(buf, "a\n", 2) == 0, "raw input");

    /* With ICANON, a line waits for its end */
    got.c_lflag |= ICANON;
    check(tcsetattr(subsidiary, TCSADRAIN, &got) == 0, "TCSADRAIN");
    check(write(manager, "bc", 2) == 2, "partial line");
    check(read_now(subsidiary, buf, sizeof(buf)) == -1 && errno == EAGAIN, "line pending");
    check(write(manager, "\r", 1) == 1, "end of line");
    check(read(subsidiary, buf, sizeof(buf)) == 3 && memcmp(buf, "bc\n", 3) == 0, "line");

    /* TCSAFLUSH discards pending input */
    check(write(manager, "fg\r", 3) == 3, "input");
    wait_input(subsidiary);
    check(tcsetattr(subsidiary, TCSAFLUSH, &got) == 0, "TCSAFLUSH");
    check(read_now(subsidiary, buf, sizeof(buf)) == -1 && errno == EAGAIN, "TCSAFLUSH flushed");

    /* Errors */
    errno = 0;
    check(tcsetattr(subsidiary, 42, &got) == -1 && errno == EINVAL, "bad optional_actions");
    check(pipe(fds) == 0, "pipe");
    check(tcgetattr(fds[0], &got) == -1 && errno == ENOTTY, "tcgetattr ENOTTY");
    check(tcsetattr(fds[0], TCSANOW, &t) == -1 && errno == ENOTTY, "tcsetattr ENOTTY");
    check(tcgetattr(-1, &got) == -1 && errno == EBADF, "tcgetattr EBADF");
    check(tcsetattr(-1, TCSANOW, &t) == -1 && errno == EBADF, "tcsetattr EBADF");
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
