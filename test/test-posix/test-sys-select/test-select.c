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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                 \
    || ((defined(__PICOLIBC_POSIX_DEVICE_IO) && __PICOLIBC_POSIX_DEVICE_IO > 0)            \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The socket checks need networking too */
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)
#define HAVE_SOCKETS 1
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#define HAVE_SOCKETS 0
#endif

/* select times out, reports ready descriptors and rejects a bad count */

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

#if HAVE_SOCKETS
/* A connected pair of TCP sockets over the loopback interface */
static void
tcp_pair(int *client, int *server)
{
    struct sockaddr_in sin;
    socklen_t          len = sizeof(sin);
    int                listener = socket(AF_INET, SOCK_STREAM, 0);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(listener >= 0, "socket");
    check(bind(listener, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    check(listen(listener, 1) == 0, "listen");
    check(getsockname(listener, (struct sockaddr *)&sin, &len) == 0, "getsockname");
    *client = socket(AF_INET, SOCK_STREAM, 0);
    check(*client >= 0, "socket");
    check(connect(*client, (struct sockaddr *)&sin, sizeof(sin)) == 0, "connect");
    *server = accept(listener, NULL, NULL);
    check(*server >= 0, "accept");
    check(close(listener) == 0, "close listener");
}
#endif

int
main(void)
{
    int            fds[2], fd, n;
    fd_set         rset, wset, eset, saved;
    struct timeval tv;
    pid_t          interrupter;
    char           path[] = "/tmp/test-select-XXXXXX";

    check(pipe(fds) == 0, "pipe");

    /* Nothing to read: select times out and clears the set */
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    tv.tv_sec = 0;
    tv.tv_usec = 20000;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == 0, "select times out");
    check(!FD_ISSET(fds[0], &rset), "select clears the set");

    /* The write end is ready for writing */
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(fds[0], &rset);
    FD_SET(fds[1], &wset);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    check(select(fds[1] + 1, &rset, &wset, NULL, &tv) == 1, "select write ready");
    check(!FD_ISSET(fds[0], &rset) && FD_ISSET(fds[1], &wset), "select ready set");

    check(write(fds[1], "x", 1) == 1, "write");

    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    check(select(fds[0] + 1, &rset, NULL, NULL, NULL) == 1, "select read ready");
    check(FD_ISSET(fds[0], &rset), "select read set");

    errno = 0;
    check(select(-1, NULL, NULL, NULL, NULL) == -1 && errno == EINVAL, "select bad nfds");
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    /* glibc passes on Linux, which accepts any count */
#ifndef __GLIBC__
    errno = 0;
    check(select(FD_SETSIZE + 1, NULL, NULL, NULL, &tv) == -1 && errno == EINVAL,
          "more than FD_SETSIZE");
#endif
    check(select(FD_SETSIZE, NULL, NULL, NULL, &tv) == 0, "FD_SETSIZE");

    /* Invalid timeouts; the sets are left alone */
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    saved = rset;
    tv.tv_sec = 0;
    /* picolibc rejects a whole second in tv_usec; glibc carries it into tv_sec */
#ifdef __PICOLIBC__
    tv.tv_usec = 1000000;
    errno = 0;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == -1 && errno == EINVAL, "tv_usec too big");
#endif
    tv.tv_usec = -1;
    errno = 0;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == -1 && errno == EINVAL, "tv_usec negative");
    tv.tv_sec = -1;
    tv.tv_usec = 0;
    errno = 0;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == -1 && errno == EINVAL, "tv_sec negative");
    check(memcmp(&rset, &saved, sizeof(rset)) == 0, "unchanged after EINVAL");
    /* A long timeout is limited, not rejected */
    tv.tv_sec = (time_t)1 << (sizeof(time_t) * 8 - 2);
    tv.tv_usec = 999999;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == 1 && FD_ISSET(fds[0], &rset),
          "long timeout");
    /* A descriptor that isn't open; the sets are left alone */
    check(pipe(fds) == 0, "pipe");
    fd = fds[1];
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    saved = rset;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    errno = 0;
    check(select(fd + 1, &rset, NULL, NULL, &tv) == -1 && errno == EBADF, "EBADF");
    check(memcmp(&rset, &saved, sizeof(rset)) == 0, "unchanged after EBADF");

    /* A timeout clears every set */
    check(pipe(fds) == 0, "pipe");
    FD_ZERO(&rset);
    FD_ZERO(&eset);
    FD_SET(fds[0], &rset);
    FD_SET(fds[0], &eset);
    FD_SET(fds[1], &eset);
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    check(select(fds[1] + 1, &rset, NULL, &eset, &tv) == 0, "timeout");
    for (fd = 0, n = 0; fd < FD_SETSIZE; fd++)
        n += FD_ISSET(fd, &rset) + FD_ISSET(fd, &eset);
    check(n == 0, "all bits 0");
    /* No sets: wait for the time */
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    check(select(0, NULL, NULL, NULL, &tv) == 0, "just a timeout");
    /* No sets and no timeout: wait for a signal */
    interrupter = start_interrupts();
    errno = 0;
    check(select(0, NULL, NULL, NULL, NULL) == -1 && errno == EINTR, "EINTR without sets");
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    saved = rset;
    errno = 0;
    check(select(fds[0] + 1, &rset, NULL, NULL, NULL) == -1 && errno == EINTR, "EINTR");
    stop_interrupts(interrupter);
    check(interrupts > 0 && memcmp(&rset, &saved, sizeof(rset)) == 0, "unchanged after EINTR");
    /* O_NONBLOCK doesn't change it */
    check(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0, "O_NONBLOCK");
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == 0, "still waits");
    /* The count covers every set */
    check(write(fds[1], "z", 1) == 1, "write");
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(fds[0], &rset);
    FD_SET(fds[1], &wset);
    check(select(fds[1] + 1, &rset, &wset, NULL, NULL) == 2, "two ready");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* Regular files are ready to read and write */
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp");
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(fd, &rset);
    FD_SET(fd, &wset);
    check(select(fd + 1, &rset, &wset, NULL, NULL) == 2, "regular file");
    check(close(fd) == 0 && unlink(path) == 0, "clean up");

#if HAVE_SOCKETS
    /* Out-of-band data is an exceptional condition */
    {
        int client, server;

        tcp_pair(&client, &server);
        FD_ZERO(&eset);
        FD_SET(server, &eset);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        check(select(server + 1, NULL, NULL, &eset, &tv) == 0, "no exceptional condition");
        check(send(client, "!", 1, MSG_OOB) == 1, "out-of-band data");
        FD_SET(server, &eset);
        check(select(server + 1, NULL, NULL, &eset, NULL) == 1 && FD_ISSET(server, &eset),
              "exceptional condition");
        check(close(client) == 0 && close(server) == 0, "close");
    }
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
