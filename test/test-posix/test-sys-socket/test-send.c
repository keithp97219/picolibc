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
#if defined(__PICOLIBC_POSIX_NETWORKING) && defined(__PICOLIBC_POSIX_DEVICE_IO)     \
    && defined(__PICOLIBC_POSIX_FD_MGMT) && defined(__PICOLIBC_POSIX_MULTI_PROCESS) \
    && defined(__PICOLIBC_POSIX_PIPE) && defined(__PICOLIBC_POSIX_SIGNALS)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>

/* send and recv move stream data, with MSG_PEEK and MSG_DONTWAIT */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Skips the test when the system has no sockets of this family */
static int
make_socket(int family, int type)
{
    int fd = socket(family, type, 0);

    if (fd < 0 && (errno == EAFNOSUPPORT || errno == EACCES || errno == EPERM)) {
        printf("no sockets of family %d, skipping\n", family);
        exit(77);
    }
    check(fd >= 0, "socket");
    return fd;
}

/* A loopback IPv4 socket address with port 0 */
static struct sockaddr_in
loopback(void)
{
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return sin;
}

/* A connected pair of TCP sockets over the loopback interface */
static void
tcp_pair(int *client, int *server)
{
    struct sockaddr_in sin = loopback();
    socklen_t          len = sizeof(sin);
    int                listener = make_socket(AF_INET, SOCK_STREAM);

    check(bind(listener, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    check(listen(listener, 1) == 0, "listen");
    check(getsockname(listener, (struct sockaddr *)&sin, &len) == 0, "getsockname");
    *client = make_socket(AF_INET, SOCK_STREAM);
    check(connect(*client, (struct sockaddr *)&sin, sizeof(sin)) == 0, "connect");
    *server = accept(listener, NULL, NULL);
    check(*server >= 0, "accept");
    check(close(listener) == 0, "close listener");
}

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

/* Fill a non-blocking socket's buffers */
static void
fill(int fd)
{
    static char block[4096];
    int         flags = fcntl(fd, F_GETFL);

    check(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0, "non-blocking");
    while (send(fd, block, sizeof(block), 0) > 0)
        ;
    check(errno == EAGAIN || errno == EWOULDBLOCK, "EAGAIN");
    while (send(fd, block, 1, 0) > 0)
        ;
    check(errno == EAGAIN || errno == EWOULDBLOCK, "EAGAIN for a byte");
    check(fcntl(fd, F_SETFL, flags) == 0, "blocking");
}

/* Whether SIGPIPE is pending; discards it */
static int
sigpipe_pending(void)
{
    sigset_t set;
    int      pending;

    check(sigpending(&set) == 0, "sigpending");
    pending = sigismember(&set, SIGPIPE);
    if (pending) {
        int sig;

        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        check(sigwait(&set, &sig) == 0 && sig == SIGPIPE, "sigwait");
    }
    return pending;
}

int
main(void)
{
    static char        big[70000];
    struct sockaddr_in sin = loopback();
    struct linger      linger = { 1, 0 };
    struct pollfd      pfd;
    sigset_t           block, old;
    pid_t              interrupter;
    int                client, server, fd, fds[2];
    char               buf[8];

    tcp_pair(&client, &server);
    check(send(client, "abcdef", 6, 0) == 6, "send");
    check(recv(server, buf, 3, MSG_PEEK | MSG_WAITALL) == 3 && memcmp(buf, "abc", 3) == 0, "peek");
    check(recv(server, buf, 6, MSG_WAITALL) == 6 && memcmp(buf, "abcdef", 6) == 0, "recv");
    check(recv(server, buf, 1, MSG_DONTWAIT) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "MSG_DONTWAIT");
    check(close(server) == 0, "close server");
    /* Writing to a closed connection fails without a SIGPIPE */
    while (send(client, "x", 1, MSG_NOSIGNAL) == 1)
        ;
    check(errno == EPIPE || errno == ECONNRESET, "EPIPE");
    check(close(client) == 0, "close client");

    /* SIGPIPE goes to the calling thread, unless MSG_NOSIGNAL */
    sigemptyset(&block);
    sigaddset(&block, SIGPIPE);
    check(sigprocmask(SIG_BLOCK, &block, &old) == 0, "block SIGPIPE");
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");
    check(shutdown(fds[0], SHUT_WR) == 0, "SHUT_WR");
    errno = 0;
    check(send(fds[0], "x", 1, MSG_NOSIGNAL) == -1 && errno == EPIPE, "shut down for writing");
    check(!sigpipe_pending(), "no SIGPIPE with MSG_NOSIGNAL");
    errno = 0;
    check(send(fds[0], "x", 1, 0) == -1 && errno == EPIPE, "EPIPE");
    check(sigpipe_pending(), "SIGPIPE");
    check(close(fds[0]) == 0, "close");
    /* No longer connected */
    errno = 0;
    check(send(fds[1], "x", 1, 0) == -1 && errno == EPIPE, "peer closed");
    check(sigpipe_pending(), "SIGPIPE for a closed peer");
    check(close(fds[1]) == 0, "close");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "restore the mask");

    /* A reset connection */
    tcp_pair(&client, &server);
    check(setsockopt(server, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == 0, "SO_LINGER");
    check(close(server) == 0, "reset");
    pfd.fd = client;
    pfd.events = POLLIN;
    check(poll(&pfd, 1, -1) == 1, "reset arrived");
    /*
     * POSIX gives ECONNRESET for a connection closed by the peer, and
     * EPIPE for one no longer connected; a reset is both (FreeBSD says
     * EPIPE, Linux ECONNRESET)
     */
    errno = 0;
    check(send(client, "x", 1, MSG_NOSIGNAL) == -1 && (errno == ECONNRESET || errno == EPIPE),
          "ECONNRESET");
    check(close(client) == 0, "close");

    /* Datagrams need a peer, and must fit */
    fd = make_socket(AF_INET, SOCK_DGRAM);
    check(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    errno = 0;
    check(send(fd, "x", 1, 0) == -1 && errno == EDESTADDRREQ, "EDESTADDRREQ");
    {
        socklen_t len = sizeof(sin);
        check(getsockname(fd, (struct sockaddr *)&sin, &len) == 0, "getsockname");
    }
    check(connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0, "connect to itself");
    errno = 0;
    check(send(fd, big, sizeof(big), 0) == -1 && errno == EMSGSIZE, "EMSGSIZE");
    check(send(fd, big, 100, 0) == 100 && recv(fd, buf, sizeof(buf), 0) == sizeof(buf),
          "a datagram that fits");
    check(close(fd) == 0, "close");
    /* Local datagram sockets have no out-of-band data */
    check(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) == 0, "socketpair");
    errno = 0;
    check(send(fds[0], "x", 1, MSG_OOB) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    /* Records stay whole */
    check(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) == 0, "socketpair");
    check(send(fds[0], "ab", 2, MSG_EOR) == 2 && send(fds[0], "cde", 3, MSG_EOR) == 3,
          "send records");
    check(recv(fds[1], buf, sizeof(buf), 0) == 2 && recv(fds[1], buf, sizeof(buf), 0) == 3,
          "record boundaries");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    /* An AF_UNIX stream without a peer */
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    errno = 0;
    check(send(fd, "x", 1, MSG_NOSIGNAL) == -1 && errno == ENOTCONN, "ENOTCONN");
    check(close(fd) == 0, "close");

    /* Full buffers: non-blocking sends fail, blocking ones wait */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");
    fill(fds[0]);
    errno = 0;
    check(send(fds[0], "x", 1, MSG_DONTWAIT) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "full");
    interrupter = start_interrupts();
    errno = 0;
    check(send(fds[0], "x", 1, 0) == -1 && errno == EINTR, "EINTR");
    stop_interrupts(interrupter);
    check(interrupts > 0, "interrupted");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(send(fds[1], "x", 1, 0) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(send(-1, "x", 1, 0) == -1 && errno == EBADF, "EBADF");
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
