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
#if TEST_POSIX_DEVICE_SPECIFIC && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_SIGNALS

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
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
 * The signal characters (XBD 11.1.9 and 11.2.5): with ISIG, INTR, QUIT
 * and SUSP send SIGINT, SIGQUIT and SIGTSTP to the foreground process
 * group of the controlling terminal, are discarded, and flush the
 * queues unless NOFLSH is set. The test runs in a child that makes a
 * pseudo-terminal its controlling terminal; the signals are blocked
 * and taken with sigtimedwait
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

static void
set(int fd, struct termios *t)
{
    check(tcsetattr(fd, TCSANOW, t) == 0, "tcsetattr");
}

/*
 * Makes the pseudo-terminal the controlling terminal of a new session
 * led by the calling process, returning a descriptor for it, or exits
 * with 77 when there can be no new session
 */
static int
take_ctty(const char *name)
{
    int fd;

    if (setsid() == (pid_t)-1) {
        printf("setsid failed, skipping\n");
        exit(77);
    }
    fd = open(name, O_RDWR);
#if TEST_VARIANCE_CTTY_TIOCSCTTY
    /* Opening it doesn't make it the controlling terminal; TIOCSCTTY does */
    check(fd >= 0 && ioctl(fd, TIOCSCTTY, 0) == 0, "TIOCSCTTY");
#endif
    check(fd >= 0 && tcgetsid(fd) == getpid(), "the controlling terminal");
    return fd;
}

/* Waits for a blocked signal */
static int
got_signal(int sig)
{
    sigset_t        set;
    struct timespec ts = { .tv_sec = 10 };

    sigemptyset(&set);
    sigaddset(&set, sig);
    return sigtimedwait(&set, NULL, &ts) == sig;
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

/* Runs fn in a child and returns its exit status, passing 77 on */
static int
in_child(int (*fn)(void))
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int ret = fn();

        fflush(stdout);
        _exit(ret);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    return WEXITSTATUS(status);
}

static int  manager;
static char name[64];

static void
on_tstp(int sig)
{
    (void)sig;
}

/* A process in the foreground group, which waits for SIGINT */
static int
foreground_member(void)
{
    return got_signal(SIGINT) ? 0 : 1;
}

static int
session(void)
{
    int            fd = take_ctty(name), ready[2], go[2];
    struct termios t;
    sigset_t       mask;
    char           intr, quit, susp, c;
    pid_t          member, outsider;
    int            status;

    /*
     * The session leader's process group is orphaned, and there a
     * SIGTSTP whose default action would stop the process may be
     * discarded even while blocked (FreeBSD's is), so catch it
     */
    check(signal(SIGTSTP, on_tstp) != SIG_ERR, "catch SIGTSTP");
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTSTP);
    check(sigprocmask(SIG_BLOCK, &mask, NULL) == 0, "block signals");

    check(tcgetattr(fd, &t) == 0, "tcgetattr");
    t.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | ISTRIP);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ICANON | ECHO | IEXTEN | NOFLSH);
    t.c_lflag |= ISIG;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    set(fd, &t);
    intr = (char)t.c_cc[VINTR];
    quit = (char)t.c_cc[VQUIT];
    susp = (char)t.c_cc[VSUSP];
    /* As cc_t, as _POSIX_VDISABLE may not fit in a char */
    check(t.c_cc[VINTR] != _POSIX_VDISABLE && t.c_cc[VQUIT] != _POSIX_VDISABLE
              && t.c_cc[VSUSP] != _POSIX_VDISABLE,
          "signal characters set");

    /* Each character sends its signal and isn't read */
    put(manager, &intr, 1);
    check(got_signal(SIGINT), "INTR sends SIGINT");
    put(manager, &quit, 1);
    check(got_signal(SIGQUIT), "QUIT sends SIGQUIT");
    put(manager, &susp, 1);
    check(got_signal(SIGTSTP), "SUSP sends SIGTSTP");
    PUT(manager, "k");
    check(GOT(fd, "k"), "signal characters discarded");

    /* Every process in the foreground process group gets the signal; others don't */
    fflush(stdout);
    member = fork();
    check(member >= 0, "fork member");
    if (member == 0)
        _exit(foreground_member());
    check(pipe(ready) == 0 && pipe(go) == 0, "pipes");
    outsider = fork();
    check(outsider >= 0, "fork outsider");
    if (outsider == 0) {
        /* A background group of the same session */
        if (setpgid(0, 0) != 0)
            _exit(2);
        if (write(ready[1], "r", 1) != 1)
            _exit(3);
        if (read(go[0], &c, 1) != 1)
            _exit(4);
        _exit(pending(SIGINT) ? 5 : 0);
    }
    check(read(ready[0], &c, 1) == 1, "outsider ready");
    put(manager, &intr, 1);
    check(got_signal(SIGINT), "INTR with more processes");
    check(waitpid(member, &status, 0) == member && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "SIGINT to the other foreground process");
    check(write(go[1], "c", 1) == 1, "tell the outsider");
    check(waitpid(outsider, &status, 0) == outsider && WIFEXITED(status)
              && WEXITSTATUS(status) == 0,
          "no SIGINT to the background group");
    check(close(ready[0]) == 0 && close(ready[1]) == 0 && close(go[0]) == 0 && close(go[1]) == 0,
          "close pipes");

    /*
     * Without NOFLSH the signal characters flush the input queue. Output
     * reaches a pseudo-terminal's manager at once, so there is no output
     * queue to see flushed
     */
    PUT(manager, "ab");
    put(manager, &intr, 1);
    PUT(manager, "c");
    check(got_signal(SIGINT), "SIGINT before the flush");
    wait_input(fd);
    check(GOT(fd, "c"), "input before INTR flushed");

    /* With NOFLSH nothing is flushed */
    t.c_lflag |= NOFLSH;
    set(fd, &t);
    PUT(manager, "ab");
    put(manager, &quit, 1);
    PUT(manager, "c");
    check(got_signal(SIGQUIT), "SIGQUIT with NOFLSH");
    PUT(manager, "~");
    {
        char   buf[16];
        size_t n = 0;

        while (memchr(buf, '~', n) == NULL) {
            ssize_t r = read(fd, buf + n, sizeof(buf) - n);

            check(r > 0, "read with NOFLSH");
            n += (size_t)r;
        }
        check(n == 4 && memcmp(buf, "abc~", 4) == 0, "NOFLSH keeps input");
    }

    /* A disabled signal character is plain data */
    t.c_cc[VINTR] = _POSIX_VDISABLE;
    set(fd, &t);
    put(manager, &intr, 1);
    check(got(fd, &intr, 1), "disabled INTR read");
    check(!pending(SIGINT), "disabled INTR sends nothing");
    t.c_cc[VINTR] = (cc_t)intr;

    /* Without ISIG the characters are data */
    t.c_lflag &= ~ISIG;
    set(fd, &t);
    put(manager, &intr, 1);
    put(manager, &quit, 1);
    put(manager, &susp, 1);
    wait_input(fd);
    {
        char   want[3] = { intr, quit, susp }, buf[3];
        size_t n = 0;

        while (n < 3) {
            ssize_t r = read(fd, buf + n, sizeof(buf) - n);

            check(r > 0, "read without ISIG");
            n += (size_t)r;
        }
        check(memcmp(buf, want, 3) == 0, "characters read without ISIG");
    }
    check(!pending(SIGINT) && !pending(SIGQUIT) && !pending(SIGTSTP), "no signals without ISIG");

    check(close(fd) == 0, "close");
    return 0;
}

int
main(void)
{
    int   subsidiary;
    char *n;
    int   ret;

    manager = open_pty(&subsidiary);
    n = ptsname(manager);
    check(n != NULL && strlen(n) < sizeof(name), "ptsname");
    strcpy(name, n);
    check(close(subsidiary) == 0, "close subsidiary");

    ret = in_child(session);
    if (ret == 77)
        return 77;
    check(ret == 0, "session checks");
    check(close(manager) == 0, "close");
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
