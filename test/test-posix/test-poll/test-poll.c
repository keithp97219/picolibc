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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_DEVICE_IO) && defined(__PICOLIBC_POSIX_MULTI_PROCESS) \
    && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The socket checks need networking too */
#if defined(__PICOLIBC_POSIX_NETWORKING)
#define HAVE_SOCKETS 1
#include <sys/socket.h>
#else
#define HAVE_SOCKETS 0
#endif

/* poll waits forever with a negative timeout and reports every entry */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/*
 * A child that sends SIGUSR1 every 10ms to a handler installed without
 * SA_RESTART, so a call that blocks is interrupted whenever it starts
 */
static volatile sig_atomic_t interrupts;

static void
on_interrupt(int sig)
{
    (void)sig;
    interrupts++;
}

static pid_t
start_interrupts(void)
{
    struct sigaction act;
    pid_t            parent = getpid(), pid;

    memset(&act, 0, sizeof(act));
    act.sa_handler = on_interrupt;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct timespec ts = { 0, 10 * 1000 * 1000 };

        for (;;) {
            kill(parent, SIGUSR1);
            nanosleep(&ts, NULL);
        }
    }
    return pid;
}

static void
stop_interrupts(pid_t pid)
{
    check(kill(pid, SIGKILL) == 0 && waitpid(pid, NULL, 0) == pid, "stop the interrupter");
    check(signal(SIGUSR1, SIG_IGN) != SIG_ERR, "ignore SIGUSR1");
}

int
main(void)
{
    struct pollfd pfd[3], *many;
    struct rlimit rl, low;
    int           fds[2], fd, rw;
    int           status;
    pid_t         pid, interrupter;
    char          c;
    char          path[] = "/tmp/test-poll-XXXXXX";
    const short all = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLOUT | POLLWRNORM | POLLWRBAND
        | POLLERR | POLLHUP | POLLNVAL;
    nfds_t n;

    check(pipe(fds) == 0, "pipe");

    /* Nothing ready: a short timeout expires */
    pfd[0].fd = fds[0];
    pfd[0].events = POLLIN;
    check(poll(pfd, 1, 20) == 0 && pfd[0].revents == 0, "poll times out");

    /*
     * With an infinite timeout, poll waits until the pipe is readable,
     * here after more than a second. One entry is ready, after one
     * with no events and one ignored.
     */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct timespec ts = { .tv_sec = 1, .tv_nsec = 500 * 1000 * 1000 };

        nanosleep(&ts, NULL);
        _exit(write(fds[1], "x", 1) == 1 ? 0 : 1);
    }
    pfd[0].fd = fds[1];
    pfd[0].events = POLLIN;
    pfd[1].fd = -1;
    pfd[1].events = POLLIN;
    pfd[2].fd = fds[0];
    pfd[2].events = POLLIN;
    check(poll(pfd, 3, -1) == 1, "poll waiting forever");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "writer");
    check(pfd[0].revents == 0 && pfd[1].revents == 0, "entries without events");
    check(pfd[2].revents == POLLIN, "ready entry");
    check(pfd[0].events == POLLIN && pfd[2].events == POLLIN, "events kept");

    check(read(fds[0], &c, 1) == 1 && c == 'x', "read");
    check(close(fds[1]) == 0, "close write end");
    pfd[2].revents = 0;
    check(poll(&pfd[2], 1, -1) == 1 && (pfd[2].revents & POLLHUP), "hang up reported");
    check(close(fds[0]) == 0, "close read end");

    /* fd and events stay as they were; POLLHUP is reported unasked */
    check(pipe(fds) == 0, "pipe");
    check(close(fds[1]) == 0, "close write end");
    pfd[0].fd = fds[0];
    pfd[0].events = 0;
    check(poll(pfd, 1, 0) == 1 && pfd[0].revents == POLLHUP, "unrequested POLLHUP");
    pfd[0].events = all;
    check(poll(pfd, 1, 0) == 1 && pfd[0].fd == fds[0] && pfd[0].events == all, "unchanged");
    check(close(fds[0]) == 0, "close");
    /* POLLNVAL for a descriptor that isn't open */
    pfd[0].fd = fds[0];
    pfd[0].events = POLLIN;
    check(poll(pfd, 1, 0) == 1 && pfd[0].revents == POLLNVAL, "POLLNVAL");
    /* POLLERR (or POLLHUP) for a pipe nobody reads; POLLWRNORM is POLLOUT */
    check(pipe(fds) == 0, "pipe");
    pfd[0].fd = fds[1];
    pfd[0].events = POLLWRNORM;
    pfd[1].fd = fds[0];
    pfd[1].events = POLLOUT;
    /* The read end may be open for writing too (FreeBSD's is) */
    rw = (fcntl(fds[0], F_GETFL) & O_ACCMODE) == O_RDWR;
    check(poll(pfd, 2, 0) == 1 + rw && pfd[0].revents == POLLWRNORM
              && pfd[1].revents == (rw ? POLLOUT : 0),
          "POLLWRNORM");
    check(close(fds[0]) == 0, "close read end");
    pfd[0].events = POLLOUT;
    /* POSIX names no event for this; Linux reports POLLERR, FreeBSD POLLHUP */
    check(poll(pfd, 1, 0) == 1 && (pfd[0].revents & (POLLERR | POLLHUP)), "POLLERR");
    /* O_NONBLOCK doesn't change poll, which still waits */
    check(close(fds[1]) == 0 && pipe(fds) == 0, "pipe");
    check(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0, "O_NONBLOCK");
    pfd[0].fd = fds[0];
    pfd[0].events = POLLIN;
    check(poll(pfd, 1, 20) == 0 && pfd[0].revents == 0, "times out");
    /* A long timeout, with something ready */
    check(write(fds[1], "y", 1) == 1, "write");
    pfd[1].fd = fds[1];
    pfd[1].events = POLLOUT;
    check(poll(pfd, 2, INT_MAX) == 2 && pfd[0].revents == POLLIN && pfd[1].revents == POLLOUT,
          "two ready");
    check(read(fds[0], &c, 1) == 1, "read");
    /* Interrupted by a signal */
    interrupter = start_interrupts();
    errno = 0;
    check(poll(pfd, 1, -1) == -1 && errno == EINTR, "EINTR");
    stop_interrupts(interrupter);
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* Regular files are always ready */
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp");
    pfd[0].fd = fd;
    pfd[0].events = POLLIN | POLLOUT;
    check(poll(pfd, 1, -1) == 1 && pfd[0].revents == (POLLIN | POLLOUT), "regular file");
    check(close(fd) == 0 && unlink(path) == 0, "clean up");

    /* More entries than OPEN_MAX */
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    low = rl;
    low.rlim_cur = 64;
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    n = (nfds_t)sysconf(_SC_OPEN_MAX) + 1;
    many = calloc(n, sizeof(*many));
    check(many != NULL, "calloc");
    for (nfds_t i = 0; i < n; i++)
        many[i].fd = -1;
    errno = 0;
    check(poll(many, n, 0) == -1 && errno == EINVAL, "EINVAL");
    check(poll(many, n - 1, 0) == 0, "OPEN_MAX entries");
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "restore the limit");
    free(many);

#if HAVE_SOCKETS
    /* A hangup excludes POLLOUT; glibc passes on Linux, which reports both */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");
    check(close(fds[1]) == 0, "close peer");
    pfd[0].fd = fds[0];
    pfd[0].events = POLLIN | POLLOUT;
    check(poll(pfd, 1, 0) == 1 && (pfd[0].revents & POLLHUP) && (pfd[0].revents & POLLIN),
          "hangup");
#ifndef __GLIBC__
    check((pfd[0].revents & POLLOUT) == 0, "no POLLOUT with POLLHUP");
#endif
    check(close(fds[0]) == 0, "close");
#endif

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
