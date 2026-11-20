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

/* setsockopt and getsockopt set and report socket options */

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

#include <sys/time.h>

int
main(void)
{
    struct timeval tv = { 0, 100000 }, got;
    struct linger  linger = { 1, 5 }, glinger;
    socklen_t      len;
    int            fd = make_socket(AF_INET, SOCK_DGRAM), on = 1, value;
    char           buf[4];

    check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0, "SO_REUSEADDR");
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, &len) == 0 && value != 0, "get");

    check(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0, "SO_RCVTIMEO");
    len = sizeof(got);
    check(getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &got, &len) == 0, "get SO_RCVTIMEO");
    /* QEMU's user-mode emulation reports nothing for this option */
    if (len != 0)
        check(got.tv_sec == 0 && got.tv_usec >= 90000 && got.tv_usec <= 110000, "timeout value");
    {
        struct sockaddr_in sin = loopback();

        check(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    }
    /* The receive times out */
    check(recv(fd, buf, sizeof(buf), 0) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "timed out");

    check(setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == 0, "SO_LINGER");
    len = sizeof(glinger);
    check(getsockopt(fd, SOL_SOCKET, SO_LINGER, &glinger, &len) == 0, "get SO_LINGER");
    check(glinger.l_onoff != 0 && glinger.l_linger == 5, "linger value");

    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_ERROR, &value, &len) == 0 && value == 0, "SO_ERROR");
    check(getsockopt(fd, SOL_SOCKET, 12345, &value, &len) == -1 && errno == ENOPROTOOPT,
          "ENOPROTOOPT");
    check(close(fd) == 0, "close");
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
