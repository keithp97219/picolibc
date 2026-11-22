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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_NETWORKING && TEST_POSIX_DEVICE_IO && TEST_POSIX_FD_MGMT && TEST_POSIX_PIPE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* recv and recvfrom take messages from sockets */

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

int
main(void)
{
    struct linger linger = { 1, 0 };
    struct pollfd pfd;
    int           client, server, fd, fds[2];
    char          buf[16];

    /* Streams ignore boundaries and discard nothing */
    tcp_pair(&client, &server);
    check(send(client, "ab", 2, 0) == 2 && send(client, "cd", 2, 0) == 2, "send");
    check(recv(server, buf, 4, MSG_PEEK | MSG_WAITALL) == 4 && memcmp(buf, "abcd", 4) == 0,
          "peek across sends");
    check(recv(server, buf, 1, 0) == 1 && buf[0] == 'a', "part");
    check(recv(server, buf, 3, MSG_WAITALL) == 3 && memcmp(buf, "bcd", 3) == 0, "the rest");
    /* A non-blocking descriptor fails when nothing is waiting */
    check(fcntl(server, F_SETFL, O_NONBLOCK) == 0, "O_NONBLOCK");
    errno = 0;
    check(recv(server, buf, 1, 0) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK), "EAGAIN");
    errno = 0;
    check(recvfrom(server, buf, 1, 0, NULL, NULL) == -1
              && (errno == EAGAIN || errno == EWOULDBLOCK),
          "recvfrom EAGAIN");
    check(fcntl(server, F_SETFL, 0) == 0, "blocking");
    /* No out-of-band data */
    errno = 0;
    check(recv(server, buf, 1, MSG_OOB) == -1 && errno == EINVAL, "EINVAL for MSG_OOB");
    /* An orderly shutdown gives 0 once the data is read */
    check(send(client, "e", 1, 0) == 1 && shutdown(client, SHUT_WR) == 0, "shutdown");
    check(recv(server, buf, sizeof(buf), MSG_WAITALL) == 1 && buf[0] == 'e', "data first");
    check(recv(server, buf, sizeof(buf), 0) == 0, "then 0");
    check(recvfrom(server, buf, sizeof(buf), 0, NULL, NULL) == 0, "recvfrom 0");
    check(close(client) == 0 && close(server) == 0, "close");

    /* A reset connection */
    tcp_pair(&client, &server);
    check(setsockopt(server, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == 0, "SO_LINGER");
    check(close(server) == 0, "reset");
    pfd.fd = client;
    pfd.events = POLLIN;
    check(poll(&pfd, 1, -1) == 1, "reset arrived");
    errno = 0;
    check(recv(client, buf, 1, 0) == -1 && errno == ECONNRESET, "ECONNRESET");
    check(close(client) == 0, "close");

    /* Not connected */
    fd = make_socket(AF_INET, SOCK_STREAM);
    errno = 0;
    check(recv(fd, buf, 1, 0) == -1 && errno == ENOTCONN, "ENOTCONN");
    errno = 0;
    check(recvfrom(fd, buf, 1, 0, NULL, NULL) == -1 && errno == ENOTCONN, "recvfrom ENOTCONN");
    check(close(fd) == 0, "close");

    /* Local datagram sockets have no out-of-band data */
    check(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) == 0, "socketpair");
    errno = 0;
    check(recv(fds[0], buf, 1, MSG_OOB) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(recv(fds[0], buf, 1, 0) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    errno = 0;
    check(recvfrom(fds[0], buf, 1, 0, NULL, NULL) == -1 && errno == ENOTSOCK, "recvfrom ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(recv(-1, buf, 1, 0) == -1 && errno == EBADF, "EBADF");
    errno = 0;
    check(recvfrom(-1, buf, 1, 0, NULL, NULL) == -1 && errno == EBADF, "recvfrom EBADF");
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
