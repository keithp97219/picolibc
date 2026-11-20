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
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0)  \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*
 * Terminal access control (XBD 11.1.4): a background process group
 * that reads its controlling terminal gets SIGTTIN, and one that writes
 * with TOSTOP set, or calls tcsetattr, tcdrain, tcflow, tcflush or
 * tcsendbreak, gets SIGTTOU, unless the signal is ignored or blocked;
 * in an orphaned process group these fail with EIO instead. The test
 * runs in a child that makes a pseudo-terminal its controlling terminal
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

/*
 * Makes the pseudo-terminal the controlling terminal of a new session
 * led by the calling process, returning a descriptor for it, or exits
 * with 77 when the system won't do that
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
    if (fd < 0 || tcgetsid(fd) != getpid()) {
        printf("no controlling terminal, skipping\n");
        exit(77);
    }
    return fd;
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

static char                  name[64];
static int                   fd, report[2];
static volatile sig_atomic_t ttou, ttin;

static void
on_signal(int sig)
{
    if (sig == SIGTTOU)
        ttou++;
    else if (sig == SIGTTIN)
        ttin++;
}

static void
disposition(int sig, void (*handler)(int))
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART, so the call fails with EINTR */
    check(sigaction(sig, &sa, NULL) == 0, "sigaction");
}

static void
mask(int how, int sig)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, sig);
    check(sigprocmask(how, &set, NULL) == 0, "sigprocmask");
}

static void
tostop(int on)
{
    struct termios t;

    mask(SIG_BLOCK, SIGTTOU);
    check(tcgetattr(fd, &t) == 0, "tcgetattr");
    if (on)
        t.c_lflag |= TOSTOP;
    else
        t.c_lflag &= ~TOSTOP;
    check(tcsetattr(fd, TCSANOW, &t) == 0, "set TOSTOP");
    mask(SIG_UNBLOCK, SIGTTOU);
}

/* The calls that are treated like a write, ignoring TOSTOP */
enum { SETATTR, DRAIN, FLOW, FLUSH, BREAK, NCALLS };

static const char * const call_names[NCALLS]
    = { "tcsetattr", "tcdrain", "tcflow", "tcflush", "tcsendbreak" };

static int
call(int which)
{
    struct termios t;

    switch (which) {
    case SETATTR:
        if (tcgetattr(fd, &t) != 0)
            return -2;
        return tcsetattr(fd, TCSANOW, &t);
    case DRAIN:
        return tcdrain(fd);
    case FLOW:
        return tcflow(fd, TCOON);
    case FLUSH:
        return tcflush(fd, TCIFLUSH);
    default:
        return tcsendbreak(fd, 0);
    }
}

static void
say(const char *msg)
{
    (void)!write(report[1], msg, strlen(msg));
}

/* A process in an orphaned background process group */
static void
orphan(pid_t parent)
{
    char    buf[64];
    int     i;
    ssize_t r;

    if (setpgid(0, 0) != 0) {
        say("setpgid");
        _exit(1);
    }
    snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
    say(buf);

    /* The group is orphaned once the parent, its only link to the session, exits */
    for (i = 0; getppid() == parent; i++) {
        struct timespec ts = { .tv_nsec = 10 * 1000 * 1000 };

        if (i == 2000) {
            say("parent still there");
            _exit(1);
        }
        nanosleep(&ts, NULL);
    }

    for (i = 0; i < NCALLS; i++) {
        errno = 0;
        if (call(i) != -1 || errno != EIO) {
            snprintf(buf, sizeof(buf), "orphaned %s: errno %d", call_names[i], errno);
            say(buf);
            _exit(1);
        }
    }
    errno = 0;
    if (write(fd, "o", 1) != -1 || errno != EIO) {
        say("orphaned write with TOSTOP");
        _exit(1);
    }
    errno = 0;
    r = read(fd, buf, 1);
    if (r != -1 || errno != EIO) {
        say("orphaned read");
        _exit(1);
    }
    say("ok");
    _exit(0);
}

/* A process in a background process group of the session */
static int
background(pid_t leader)
{
    struct termios t;
    sigset_t       set;
    char           buf[4];
    int            i;
    pid_t          pid, self;

    check(getpgrp() != tcgetpgrp(fd), "in the background");
    check(tcgetsid(fd) == leader, "tcgetsid in a background group");

    /* tcgetattr is allowed from any process */
    disposition(SIGTTOU, on_signal);
    disposition(SIGTTIN, on_signal);
    check(tcgetattr(fd, &t) == 0 && ttou == 0, "tcgetattr from the background");

    /* With TOSTOP, a write sends SIGTTOU */
    tostop(1);
    ttou = 0;
    errno = 0;
    check(write(fd, "a", 1) == -1 && errno == EINTR && ttou == 1, "write with TOSTOP");

    /* Unless SIGTTOU is ignored */
    disposition(SIGTTOU, SIG_IGN);
    check(write(fd, "b", 1) == 1, "write with TOSTOP and SIGTTOU ignored");
    for (i = 0; i < NCALLS; i++)
        check(call(i) == 0, call_names[i]);
    disposition(SIGTTOU, on_signal);

    /* Or blocked, in which case it isn't sent either */
    ttou = 0;
    mask(SIG_BLOCK, SIGTTOU);
    check(write(fd, "c", 1) == 1, "write with TOSTOP and SIGTTOU blocked");
    for (i = 0; i < NCALLS; i++)
        check(call(i) == 0, call_names[i]);
    check(sigpending(&set) == 0 && !sigismember(&set, SIGTTOU), "no SIGTTOU while blocked");
    mask(SIG_UNBLOCK, SIGTTOU);
    check(ttou == 0, "no SIGTTOU after unblocking");

    /* Without TOSTOP writes are allowed, but the other calls still send SIGTTOU */
    tostop(0);
    check(write(fd, "d", 1) == 1 && ttou == 0, "write without TOSTOP");
    for (i = 0; i < NCALLS; i++) {
        ttou = 0;
        errno = 0;
        if (call(i) != -1 || errno != EINTR || ttou != 1) {
            printf("%s from the background: errno %d, %d SIGTTOU\n", call_names[i], errno,
                   (int)ttou);
            return 1;
        }
    }

    /* A read sends SIGTTIN */
    ttin = 0;
    errno = 0;
    check(read(fd, buf, 1) == -1 && errno == EINTR && ttin == 1, "read sends SIGTTIN");

    /* Unless it is ignored or blocked; then the read fails with EIO */
    disposition(SIGTTIN, SIG_IGN);
    errno = 0;
    check(read(fd, buf, 1) == -1 && errno == EIO, "read with SIGTTIN ignored");
    disposition(SIGTTIN, on_signal);
    ttin = 0;
    mask(SIG_BLOCK, SIGTTIN);
    errno = 0;
    check(read(fd, buf, 1) == -1 && errno == EIO, "read with SIGTTIN blocked");
    check(sigpending(&set) == 0 && !sigismember(&set, SIGTTIN), "no SIGTTIN while blocked");
    mask(SIG_UNBLOCK, SIGTTIN);
    check(ttin == 0, "no SIGTTIN after unblocking");

    /* An orphaned group gets EIO rather than a signal that would stop it */
    tostop(1);
    disposition(SIGTTOU, SIG_DFL);
    disposition(SIGTTIN, SIG_DFL);
    /* This process's ID, as it may have exited before the child asks for its parent */
    self = getpid();
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork orphan");
    if (pid == 0)
        orphan(self);
    return 0;
}

static int
session(void)
{
    pid_t         leader = getpid(), pid, orphan_pid;
    int           status;
    char          msg[128], *result;
    size_t        n = 0;
    struct pollfd pfd;

    fd = take_ctty(name);
    check(pipe(report) == 0, "pipe");

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (setpgid(0, 0) != 0)
            _exit(2);
        fflush(stdout);
        _exit(background(leader));
    }
    (void)setpgid(pid, pid);
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "background child");
    check(WEXITSTATUS(status) == 0, "background checks");

    /* The orphan sends its process ID and then its result, and closes the pipe as it exits */
    check(close(report[1]) == 0, "close");
    pfd.fd = report[0];
    pfd.events = POLLIN;
    for (;;) {
        ssize_t r;

        if (poll(&pfd, 1, 30000) != 1) {
            msg[n] = '\0';
            orphan_pid = (pid_t)atol(msg);
            if (orphan_pid > 0)
                kill(orphan_pid, SIGKILL);
            check(0, "orphan reports");
        }
        r = read(report[0], msg + n, sizeof(msg) - 1 - n);
        check(r >= 0, "orphan report");
        if (r == 0)
            break;
        n += (size_t)r;
        check(n < sizeof(msg) - 1, "orphan report too long");
    }
    msg[n] = '\0';
    result = strchr(msg, '\n');
    if (result == NULL || strcmp(result + 1, "ok") != 0) {
        printf("orphan: %s\n", result ? result + 1 : msg);
        return 1;
    }
    return 0;
}

int
main(void)
{
    int   subsidiary, manager;
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
