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
#if TEST_POSIX_DEVICE_SPECIFIC && TEST_POSIX_PIPE

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*
 * Input and output processing (XBD 11.2.2 and 11.2.3): the mapping
 * flags, ISTRIP, PARMRK, IXON and IXANY, OPOST and the XSI output
 * flags, and attributes that last until tcsetattr changes them
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

/* Collects what the manager receives before the sentinel */
static size_t
collect(int manager, char sentinel, char *buf, size_t size)
{
    size_t n = 0;

    while (memchr(buf, sentinel, n) == NULL) {
        ssize_t r;

        check(n < size, "manager output too long");
        wait_input(manager);
        r = read(manager, buf + n, size - n);
        check(r > 0, "read manager");
        n += (size_t)r;
    }
    return (size_t)((char *)memchr(buf, sentinel, n) - buf);
}

/* Writes on the subsidiary and checks what the manager receives */
static int
output(int subsidiary, int manager, const char *s, const char *want)
{
    put(subsidiary, s, strlen(s));
    PUT(subsidiary, "~");
    return manager_got(manager, '~', want, strlen(want));
}

/* Writes on the manager and checks what the subsidiary reads */
static int
input(int subsidiary, int manager, const char *s, size_t len, const char *want, size_t wlen)
{
    put(manager, s, len);
    PUT(manager, "~");
    wait_input(subsidiary);
    {
        char   buf[64];
        size_t n = 0;

        while (memchr(buf, '~', n) == NULL) {
            ssize_t r;

            wait_input(subsidiary);
            r = read(subsidiary, buf + n, sizeof(buf) - n);
            check(r > 0, "read subsidiary");
            n += (size_t)r;
        }
        n--;
        if (n != wlen || memcmp(buf, want, wlen) != 0) {
            size_t i;

            printf("read %zu bytes:", n);
            for (i = 0; i < n; i++)
                printf(" %02x", (unsigned char)buf[i]);
            printf("\n");
            return 0;
        }
    }
    return 1;
}

#define INPUT(s, w) input(subsidiary, manager, s, sizeof(s) - 1, w, sizeof(w) - 1)

/* Polls briefly; output that should stay suspended mustn't show up */
static int
quiet(int fd)
{
    struct pollfd pfd = { .fd = fd, .events = POLLIN };

    return poll(&pfd, 1, 200) == 0;
}

/* Writes while output is suspended, which may not take the byte */
static int
write_stopped(int fd, char c)
{
    int     flags = fcntl(fd, F_GETFL);
    ssize_t ret;

    check(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0, "O_NONBLOCK");
    ret = write(fd, &c, 1);
    check(ret == 1 || (ret == -1 && errno == EAGAIN), "write while suspended");
    check(fcntl(fd, F_SETFL, flags) == 0, "restore flags");
    return ret == 1;
}

int
main(void)
{
    int            subsidiary, manager = open_pty(&subsidiary);
    struct termios t, raw, now;
    char           buf[16], stop, start, *name;
    int            other, written;
    size_t         n;

    check(tcgetattr(subsidiary, &t) == 0, "tcgetattr");
    t.c_iflag &= ~(BRKINT | ICRNL | INLCR | IGNCR | IXON | IXOFF | IXANY | ISTRIP | PARMRK | INPCK);
    t.c_oflag &= ~(OPOST | ONLCR | OCRNL | ONOCR | ONLRET | OFILL | NLDLY | CRDLY | TABDLY | BSDLY
                   | VTDLY | FFDLY);
    t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;
    t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    set(subsidiary, &t);
    raw = t;
    stop = (char)t.c_cc[VSTOP];
    start = (char)t.c_cc[VSTART];

    /* Without OPOST, output is unchanged whatever the other flags say */
    t.c_oflag |= ONLCR | OCRNL;
    set(subsidiary, &t);
    check(output(subsidiary, manager, "a\nb\r\t", "a\nb\r\t"), "no OPOST");

    /* ONLCR maps NL to CR-NL */
    t.c_oflag = (raw.c_oflag | OPOST | ONLCR);
    set(subsidiary, &t);
    check(output(subsidiary, manager, "a\nb", "a\r\nb"), "ONLCR");

    /* OCRNL maps CR to NL */
    t.c_oflag = (raw.c_oflag | OPOST | OCRNL);
    set(subsidiary, &t);
    check(output(subsidiary, manager, "a\rb", "a\nb"), "OCRNL");

    /*
     * ONOCR drops CR at column 0; a transmitted CR returns to column 0.
     * The first CR goes out unless the column is already 0
     */
    t.c_oflag = (raw.c_oflag | OPOST | ONOCR);
    set(subsidiary, &t);
    put(subsidiary, "\r\rab\r\r~", 8);
    n = collect(manager, '~', buf, sizeof(buf));
    check((n == 3 && memcmp(buf, "ab\r", 3) == 0) || (n == 4 && memcmp(buf, "\rab\r", 4) == 0),
          "ONOCR");

    /* NL leaves the column alone, unless ONLRET gives it the CR function */
    check(output(subsidiary, manager, "ab\n\r", "ab\n\r"), "NL without ONLRET");
    t.c_oflag |= ONLRET;
    set(subsidiary, &t);
    check(output(subsidiary, manager, "ab\n\r", "ab\n"), "ONLRET");

    /* TAB3 expands tabs to spaces, to the next multiple of eight columns after CR */
    t.c_oflag = (raw.c_oflag | OPOST | TAB3);
    set(subsidiary, &t);
    check(output(subsidiary, manager, "\ra\tb", "\ra       b"), "TAB3");
    t = raw;
    set(subsidiary, &t);

    /* ICRNL maps CR to NL, INLCR NL to CR, and IGNCR drops CR, even with ICRNL */
    t.c_iflag |= ICRNL;
    set(subsidiary, &t);
    check(INPUT("a\rb", "a\nb"), "ICRNL");
    t.c_iflag = raw.c_iflag | INLCR;
    set(subsidiary, &t);
    check(INPUT("a\nb", "a\rb"), "INLCR");
    t.c_iflag = raw.c_iflag | IGNCR | ICRNL;
    set(subsidiary, &t);
    check(INPUT("a\rb", "ab"), "IGNCR");
    t = raw;
    set(subsidiary, &t);
    check(INPUT("a\r\nb", "a\r\nb"), "no input mapping");

    /* ISTRIP strips input to seven bits */
    check(INPUT("\xc1", "\xc1"), "eight bits without ISTRIP");
    t.c_iflag |= ISTRIP;
    set(subsidiary, &t);
    check(INPUT("\xc1", "A"), "ISTRIP");

    /* With PARMRK and neither IGNPAR nor ISTRIP, a valid 0xff reads as 0xff 0xff */
    t.c_iflag = (raw.c_iflag & ~(IGNPAR | ISTRIP)) | PARMRK;
    set(subsidiary, &t);
    check(INPUT("a\xff", "a\xff\xff"), "PARMRK doubles 0xff");
    t = raw;
    set(subsidiary, &t);

    /* Without IXON, STOP and START are read */
    {
        char both[2] = { stop, start };

        check(input(subsidiary, manager, both, 2, both, 2), "STOP and START read without IXON");
    }
    t.c_iflag |= IXON;
    set(subsidiary, &t);

    /* With IXON, STOP suspends output and START restarts it; neither is read */
    put(manager, &stop, 1);
    check(INPUT("k", "k"), "STOP isn't read");
    written = write_stopped(subsidiary, 'q');
    check(quiet(manager), "STOP suspends output");
    put(manager, &start, 1);
    check(INPUT("m", "m"), "START isn't read");
    if (!written)
        PUT(subsidiary, "q");
    check(GOT(manager, "q"), "START restarts output");

    /* With IXANY, any input restarts output; stop it first, as other input would restart it */
    put(manager, &stop, 1);
    check(INPUT("k", "k"), "STOP before IXANY");
    t.c_iflag |= IXANY;
    set(subsidiary, &t);
    written = write_stopped(subsidiary, 'r');
    check(quiet(manager), "STOP suspends output with IXANY");
    check(INPUT("z", "z"), "IXANY character read");
    if (!written)
        PUT(subsidiary, "r");
    check(GOT(manager, "r"), "IXANY restarts output");
    t = raw;
    set(subsidiary, &t);

    /* Opening the terminal again leaves its attributes alone */
    t.c_iflag |= ICRNL;
    t.c_oflag |= OPOST | ONLCR;
    t.c_cc[VMIN] = 3;
    set(subsidiary, &t);
    name = ptsname(manager);
    check(name != NULL, "ptsname");
    other = open(name, O_RDWR | O_NOCTTY);
    check(other >= 0, "open again");
    check(tcgetattr(other, &now) == 0, "tcgetattr on the new descriptor");
    check((now.c_iflag & ICRNL) && (now.c_oflag & (OPOST | ONLCR)) == (OPOST | ONLCR)
              && now.c_cc[VMIN] == 3 && !(now.c_lflag & ICANON),
          "attributes kept on open");
    check(close(other) == 0, "close the new descriptor");
    check(tcgetattr(subsidiary, &now) == 0 && (now.c_iflag & ICRNL) && now.c_cc[VMIN] == 3,
          "attributes kept on close");
    (void)buf;

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
