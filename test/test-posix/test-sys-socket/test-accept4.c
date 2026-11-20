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
/* glibc declares accept4, SO_DOMAIN and SO_PROTOCOL only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>

/* accept4 sets flags on the accepted socket */

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

/* The lowest descriptor a new file would get */
static int
lowest_free(void)
{
    int fd = open("/dev/null", O_RDONLY);

    check(fd >= 0, "open");
    check(close(fd) == 0, "close");
    return fd;
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

/* A listening loopback socket, and its address */
static int
listener_at(struct sockaddr_in *sin, int flags)
{
    socklen_t len = sizeof(*sin);
    int       fd = make_socket(AF_INET, SOCK_STREAM | flags);

    *sin = loopback();
    check(bind(fd, (struct sockaddr *)sin, sizeof(*sin)) == 0, "bind");
    check(listen(fd, 4) == 0, "listen");
    check(getsockname(fd, (struct sockaddr *)sin, &len) == 0, "getsockname");
    return fd;
}

/* A client connected to the listener at sin */
static int
connect_to(const struct sockaddr_in *sin)
{
    int fd = make_socket(AF_INET, SOCK_STREAM);

    check(connect(fd, (const struct sockaddr *)sin, sizeof(*sin)) == 0, "connect");
    return fd;
}

static int
int_option(int fd, int name)
{
    int       value;
    socklen_t len = sizeof(value);

    check(getsockopt(fd, SOL_SOCKET, name, &value, &len) == 0, "getsockopt");
    return value;
}

int
main(void)
{
    struct sockaddr_in sin, peer, name;
    struct rlimit      rl, low;
    unsigned char      small[sizeof(struct sockaddr_in)];
    socklen_t          len = sizeof(sin);
    int                listener, client, server, other, fds[2];
    pid_t              timer;

    listener = listener_at(&sin, 0);
    check(fcntl(listener, F_SETFL, O_NONBLOCK) == 0, "non-blocking listener");
    check(accept4(listener, NULL, NULL, 0) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "nothing waiting");
    check(accept(listener, NULL, NULL) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "accept with nothing waiting");
    client = connect_to(&sin);
    len = sizeof(peer);
    server = accept4(listener, (struct sockaddr *)&peer, &len, SOCK_CLOEXEC | SOCK_NONBLOCK);
    check(server >= 0, "accept4");
    check(len == sizeof(peer) && peer.sin_family == AF_INET, "peer address");
    /* The peer address is the client's own name */
    len = sizeof(name);
    check(getsockname(client, (struct sockaddr *)&name, &len) == 0, "client name");
    check(peer.sin_port == name.sin_port && peer.sin_addr.s_addr == name.sin_addr.s_addr,
          "the client's address");
    check(fcntl(server, F_GETFD) & FD_CLOEXEC, "SOCK_CLOEXEC");
    check(fcntl(server, F_GETFL) & O_NONBLOCK, "SOCK_NONBLOCK");
    check(close(server) == 0 && close(client) == 0, "close");

    /* accept4 without flags: neither flag, whatever the listener has */
    check(fcntl(listener, F_SETFD, FD_CLOEXEC) == 0, "FD_CLOEXEC on the listener");
    client = connect_to(&sin);
    server = accept4(listener, NULL, NULL, 0);
    check(server >= 0, "accept4 without flags");
    check((fcntl(server, F_GETFD) & FD_CLOEXEC) == 0, "no FD_CLOEXEC from accept4");
    check((fcntl(server, F_GETFL) & O_NONBLOCK) == 0, "no O_NONBLOCK from accept4");
    check(close(server) == 0 && close(client) == 0, "close");

    /* accept clears FD_CLOEXEC; the socket matches the listener's type */
    client = connect_to(&sin);
    server = accept(listener, NULL, NULL);
    check(server >= 0, "accept");
    check((fcntl(server, F_GETFD) & FD_CLOEXEC) == 0, "no FD_CLOEXEC from accept");
    check(int_option(server, SO_TYPE) == SOCK_STREAM, "same type");
    check(int_option(server, SO_DOMAIN) == AF_INET
              && int_option(server, SO_PROTOCOL) == IPPROTO_TCP,
          "same family and protocol");
    /* The accepted socket can't accept; the listener still can */
    errno = 0;
    check(accept(server, NULL, NULL) == -1 && errno == EINVAL, "accepted socket can't accept");
    check(close(server) == 0 && close(client) == 0, "close");
    client = connect_to(&sin);
    other = accept(listener, NULL, NULL);
    check(other >= 0, "the listener accepts more");
    check(close(other) == 0 && close(client) == 0, "close");

    /* A short buffer gets the start of the address and the full length */
    client = connect_to(&sin);
    memset(small, 0xa5, sizeof(small));
    len = 4;
    server = accept(listener, (struct sockaddr *)small, &len);
    check(server >= 0, "accept with a short buffer");
    check(len == sizeof(struct sockaddr_in), "actual length");
    check(small[4] == 0xa5 && small[sizeof(small) - 1] == 0xa5, "truncated");
    check(close(server) == 0 && close(client) == 0, "close");

    /* Errors leave the length alone */
    len = 12345;
    check(accept(listener, (struct sockaddr *)&peer, &len) == -1
              && (errno == EAGAIN || errno == EWOULDBLOCK) && len == 12345,
          "length unchanged");
    errno = 0;
    check(accept4(listener, NULL, NULL, 0x7fff0000) == -1 && errno == EINVAL, "bad flags");

    /* EMFILE once every descriptor is in use */
    client = connect_to(&sin);
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    low = rl;
    low.rlim_cur = (rlim_t)lowest_free();
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    errno = 0;
    server = accept(listener, NULL, NULL);
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "restore the limit");
    check(server == -1 && errno == EMFILE, "EMFILE");
    check(close(client) == 0 && close(listener) == 0, "close");

    /* A blocking accept waits until a signal interrupts it */
    listener = listener_at(&sin, 0);
    timer = start_interrupts();
    errno = 0;
    server = accept(listener, NULL, NULL);
    stop_interrupts(timer);
    check(server == -1 && errno == EINTR && interrupts > 0, "EINTR");
    check(close(listener) == 0, "close");

    /* Not listening, not a connection-mode socket, not a socket */
    listener = make_socket(AF_INET, SOCK_STREAM);
    sin = loopback();
    check(bind(listener, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    errno = 0;
    check(accept(listener, NULL, NULL) == -1 && errno == EINVAL, "not listening");
    check(close(listener) == 0, "close");
    listener = make_socket(AF_INET, SOCK_DGRAM);
    errno = 0;
    check(accept(listener, NULL, NULL) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(close(listener) == 0, "close");
    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(accept(fds[0], NULL, NULL) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(accept(-1, NULL, NULL) == -1 && errno == EBADF, "EBADF");
    errno = 0;
    check(accept4(-1, NULL, NULL, 0) == -1 && errno == EBADF, "accept4 EBADF");
    (void)tcp_pair;
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
