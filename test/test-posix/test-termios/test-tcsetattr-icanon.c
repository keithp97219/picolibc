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
 * Canonical input (XBD 11.1.6 and 11.1.9): lines, ERASE, KILL, EOF and
 * EOL, _POSIX_VDISABLE, and ECHO, ECHOE, ECHOK and ECHONL
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

/* Waits until fd can be read, which a pseudo-terminal allows a little later */
static void
wait_input(int fd)
{
    struct pollfd pfd = { .fd = fd, .events = POLLIN };

    check(poll(&pfd, 1, 10000) == 1, "input arrives");
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

static void
put(int fd, const char *s, size_t len)
{
    check(write(fd, s, len) == (ssize_t)len, "write");
}

#define PUT(fd, s) put(fd, s, sizeof(s) - 1)

/* Reads a line, or as much as one read returns, and compares it */
static int
got(int fd, const char *want, size_t len)
{
    char    buf[64];
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n != (ssize_t)len || memcmp(buf, want, len) != 0) {
        printf("read %zd bytes:", n);
        while (n > 0)
            printf(" %02x", (unsigned char)buf[--n]);
        printf(" (reversed)\n");
        return 0;
    }
    return 1;
}

#define GOT(fd, s) got(fd, s, sizeof(s) - 1)

/*
 * Collects what the manager receives up to and including the sentinel
 * character, which the test sends last, and compares the rest
 */
static int
manager_got(int manager, char sentinel, const char *want, size_t len)
{
    char   buf[256];
    size_t n = 0;

    for (;;) {
        ssize_t r;

        wait_input(manager);
        r = read(manager, buf + n, sizeof(buf) - n);
        check(r > 0, "read manager");
        n += (size_t)r;
        if (memchr(buf, sentinel, n) != NULL)
            break;
        check(n < sizeof(buf), "manager output too long");
    }
    n = (size_t)((char *)memchr(buf, sentinel, n) - buf);
    if (n != len || memcmp(buf, want, len) != 0) {
        size_t i;

        printf("manager got %zu bytes:", n);
        for (i = 0; i < n; i++)
            printf(" %02x", (unsigned char)buf[i]);
        printf("\n");
        return 0;
    }
    return 1;
}

#define MANAGER_GOT(m, c, s) manager_got(m, c, s, sizeof(s) - 1)

static void
set(int fd, struct termios *t)
{
    check(tcsetattr(fd, TCSANOW, t) == 0, "tcsetattr");
}

int
main(void)
{
    int            subsidiary, manager = open_pty(&subsidiary);
    struct termios t, canon;
    char           erase, kill, eof, buf[16];

    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");
    t.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | ISTRIP);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
    t.c_lflag |= ICANON;
    t.c_cc[VERASE] = erase = 'E';
    t.c_cc[VKILL] = kill = 'K';
    t.c_cc[VEOF] = eof = 'F';
    t.c_cc[VEOL] = '@';
    set(subsidiary, &t);
    canon = t;

    /* A read returns at most one line, and nothing before the line ends */
    PUT(manager, "ab");
    PUT(manager, "\ncd\n");
    check(GOT(subsidiary, "ab\n"), "first line");
    check(GOT(subsidiary, "cd\n"), "second line");
    PUT(manager, "gh");
    check(read_now(subsidiary, buf, sizeof(buf)) == -1 && errno == EAGAIN, "incomplete line");
    PUT(manager, "\n");

    /* Part of a line can be read and the rest is kept */
    check(read(subsidiary, buf, 1) == 1 && buf[0] == 'g', "one byte");
    check(GOT(subsidiary, "h\n"), "rest of the line");

    /* ERASE removes the last character, KILL the line, neither goes past its start */
    PUT(manager, "abcEd\n");
    check(GOT(subsidiary, "abd\n"), "ERASE");
    PUT(manager, "abcKde\n");
    check(GOT(subsidiary, "de\n"), "KILL");
    PUT(manager, "x\nEEy\n");
    check(GOT(subsidiary, "x\n"), "line before ERASE");
    check(GOT(subsidiary, "y\n"), "ERASE stops at the start of the line");
    PUT(manager, "z\nKw\n");
    check(GOT(subsidiary, "z\n"), "line before KILL");
    check(GOT(subsidiary, "w\n"), "KILL stops at the start of the line");

    /* EOF passes the bytes on and is discarded; at the start of a line it reads as 0 */
    PUT(manager, "xyF");
    check(GOT(subsidiary, "xy"), "EOF ends a line");
    PUT(manager, "F");
    check(read(subsidiary, buf, sizeof(buf)) == 0, "EOF at the start of a line");

    /* EOL ends a line too, and stays in it */
    PUT(manager, "pq@");
    check(GOT(subsidiary, "pq@"), "EOL");

    /* _POSIX_VDISABLE turns a special character off */
    t.c_cc[VERASE] = _POSIX_VDISABLE;
    t.c_cc[VEOL] = _POSIX_VDISABLE;
    set(subsidiary, &t);
    PUT(manager, "aE\n");
    check(GOT(subsidiary, "aE\n"), "disabled ERASE");
    put(manager, "a\0b\n", 4);
    check(got(subsidiary, "a\0b\n", 4), "a NUL byte isn't a disabled EOL");
    t = canon;

    /* Without ICANON there are no lines, and ERASE and KILL are data */
    t.c_lflag &= ~ICANON;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    set(subsidiary, &t);
    PUT(manager, "aEK");
    wait_input(subsidiary);
    check(GOT(subsidiary, "aEK"), "no erase or kill without ICANON");
    t = canon;

    /* ECHO echoes the input; without it nothing is echoed */
    t.c_lflag |= ECHO;
    set(subsidiary, &t);
    PUT(manager, "hi\n");
    check(GOT(subsidiary, "hi\n"), "line with ECHO");
    PUT(subsidiary, "~");
    check(MANAGER_GOT(manager, '~', "hi\n"), "ECHO");
    t.c_lflag &= ~ECHO;
    set(subsidiary, &t);
    PUT(manager, "ho\n");
    check(GOT(subsidiary, "ho\n"), "line without ECHO");
    PUT(subsidiary, "~");
    check(MANAGER_GOT(manager, '~', ""), "no ECHO");

    /* ECHONL echoes a newline even without ECHO */
    t.c_lflag |= ECHONL;
    set(subsidiary, &t);
    PUT(manager, "ab\n");
    check(GOT(subsidiary, "ab\n"), "line with ECHONL");
    PUT(subsidiary, "~");
    check(MANAGER_GOT(manager, '~', "\n"), "ECHONL");
    t.c_lflag &= ~ECHONL;

    /* ECHOE erases the character from the display */
    t.c_lflag |= ECHO | ECHOE;
    set(subsidiary, &t);
    PUT(manager, "abE\n");
    check(GOT(subsidiary, "a\n"), "line with ECHOE");
    PUT(subsidiary, "~");
    check(MANAGER_GOT(manager, '~', "ab\b \b\n"), "ECHOE");

    /* ECHOK erases the line from the display or echoes a newline after KILL */
    t.c_lflag &= ~ECHOE;
    t.c_lflag |= ECHOK;
    set(subsidiary, &t);
    PUT(manager, "abK");
    PUT(manager, "c\n");
    check(GOT(subsidiary, "c\n"), "line with ECHOK");
    PUT(subsidiary, "~");
    {
        char   out[64];
        size_t n = 0;

        for (;;) {
            ssize_t r;

            wait_input(manager);
            r = read(manager, out + n, sizeof(out) - n);
            check(r > 0, "read manager");
            n += (size_t)r;
            if (memchr(out, '~', n) != NULL)
                break;
            check(n < sizeof(out), "manager output too long");
        }
        n = (size_t)((char *)memchr(out, '~', n) - out);
        check(n >= 4 && memcmp(out, "ab", 2) == 0 && memcmp(out + n - 2, "c\n", 2) == 0,
              "ECHOK output");
        /* Between "ab" and "c\n": the line erased, or KILL and a newline */
        check((n == 2 + 6 + 2 && memcmp(out + 2, "\b \b\b \b", 6) == 0)
                  || (n >= 2 + 2 + 2 && out[n - 3] == '\n' && memchr(out + 2, kill, n - 4) != NULL),
              "ECHOK erases the line or echoes a newline");
    }
    (void)erase;
    (void)eof;

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
