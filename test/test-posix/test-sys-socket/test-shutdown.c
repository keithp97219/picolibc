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
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* shutdown ends one or both directions of a connection */

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
    int  client, server, fd, fds[2];
    char buf[4];

    tcp_pair(&client, &server);
    check(shutdown(client, SHUT_WR) == 0, "SHUT_WR");
    check(read(server, buf, sizeof(buf)) == 0, "end of file");
    errno = 0;
    check(send(client, "x", 1, MSG_NOSIGNAL) == -1 && errno == EPIPE, "no more sends");
    check(write(server, "ok", 2) == 2 && read(client, buf, 2) == 2, "other direction open");
    check(shutdown(server, SHUT_RDWR) == 0, "SHUT_RDWR");
    errno = 0;
    check(send(server, "x", 1, MSG_NOSIGNAL) == -1 && errno == EPIPE, "no sends after SHUT_RDWR");
    check(shutdown(server, 42) == -1 && errno == EINVAL, "bad how");
    check(close(client) == 0 && close(server) == 0, "close");
    check(shutdown(server, SHUT_RD) == -1 && errno == EBADF, "EBADF");

    /* No more receives */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");
    check(shutdown(fds[0], SHUT_RD) == 0, "SHUT_RD");
    check(recv(fds[0], buf, sizeof(buf), 0) == 0, "receives end");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* Not connected */
    fd = make_socket(AF_INET, SOCK_STREAM);
    errno = 0;
    check(shutdown(fd, SHUT_RDWR) == -1 && errno == ENOTCONN, "TCP ENOTCONN");
    check(close(fd) == 0, "close");
    fd = make_socket(AF_INET, SOCK_DGRAM);
    errno = 0;
    check(shutdown(fd, SHUT_WR) == -1 && errno == ENOTCONN, "UDP ENOTCONN");
    check(close(fd) == 0, "close");
    /* glibc passes on Linux, which shuts down unconnected AF_UNIX sockets */
#ifndef __GLIBC__
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    errno = 0;
    check(shutdown(fd, SHUT_RDWR) == -1 && errno == ENOTCONN, "AF_UNIX stream ENOTCONN");
    check(close(fd) == 0, "close");
    fd = make_socket(AF_UNIX, SOCK_DGRAM);
    errno = 0;
    check(shutdown(fd, SHUT_RD) == -1 && errno == ENOTCONN, "AF_UNIX datagram ENOTCONN");
    check(close(fd) == 0, "close");
#endif
    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(shutdown(fds[0], SHUT_RD) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
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
