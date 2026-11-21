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
/* glibc defines SO_DOMAIN and SO_PROTOCOL only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_NETWORKING)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>

/* socket makes sockets of each type, and rejects unknown families */

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

int
main(void)
{
    struct rlimit rl, low;
    int           fd, type, value, a, b;
    socklen_t     len = sizeof(type);

    fd = make_socket(AF_INET, SOCK_STREAM);
    check(getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &len) == 0 && type == SOCK_STREAM, "SO_TYPE");
    /* Protocol 0 picks the default, TCP for a stream */
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &value, &len) == 0 && value == IPPROTO_TCP,
          "default protocol");
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &value, &len) == 0 && value == AF_INET,
          "SO_DOMAIN");
    /* Without flags, neither FD_CLOEXEC nor O_NONBLOCK */
    check((fcntl(fd, F_GETFD) & FD_CLOEXEC) == 0, "no FD_CLOEXEC");
    check((fcntl(fd, F_GETFL) & O_NONBLOCK) == 0, "no O_NONBLOCK");
    check(close(fd) == 0, "close");
    fd = make_socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK);
    check(fcntl(fd, F_GETFD) & FD_CLOEXEC, "SOCK_CLOEXEC");
    check(fcntl(fd, F_GETFL) & O_NONBLOCK, "SOCK_NONBLOCK");
    len = sizeof(type);
    check(getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &len) == 0 && type == SOCK_DGRAM,
          "flags aren't part of the type");
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &value, &len) == 0 && value == IPPROTO_UDP,
          "default datagram protocol");
    check(close(fd) == 0, "close");
    /* An explicit protocol */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    check(fd >= 0, "IPPROTO_TCP");
    check(close(fd) == 0, "close");
    fd = make_socket(AF_UNIX, SOCK_STREAM);
    check(close(fd) == 0, "close");
    fd = make_socket(AF_UNIX, SOCK_SEQPACKET);
    check(close(fd) == 0, "close");

    /* The lowest free descriptor (2.6 File Descriptor Allocation) */
    a = make_socket(AF_UNIX, SOCK_DGRAM);
    b = make_socket(AF_UNIX, SOCK_DGRAM);
    check(close(a) == 0, "close a");
    fd = make_socket(AF_UNIX, SOCK_DGRAM);
    check(fd == a, "lowest descriptor");
    check(close(fd) == 0 && close(b) == 0, "close");

    check(socket(12345, SOCK_STREAM, 0) == -1 && errno == EAFNOSUPPORT, "EAFNOSUPPORT");
    /* UDP isn't a stream protocol */
    errno = 0;
    check(socket(AF_INET, SOCK_STREAM, IPPROTO_UDP) == -1
              && (errno == EPROTONOSUPPORT || errno == EPROTOTYPE),
          "EPROTONOSUPPORT");
    /* Types and protocols no system has; glibc passes on Linux's EINVAL */
    errno = 0;
#ifndef __GLIBC__
    check(socket(AF_UNIX, 0x7ff0, 0) == -1
              && (errno == ESOCKTNOSUPPORT || errno == EPROTONOSUPPORT || errno == EPROTOTYPE),
          "unsupported type");
    errno = 0;
    check(socket(AF_INET, SOCK_DGRAM, -1) == -1 && errno == EPROTONOSUPPORT, "unknown protocol");
#endif
    errno = 0;
    check(socket(AF_UNIX, SOCK_STREAM, IPPROTO_TCP) == -1 && errno == EPROTONOSUPPORT,
          "protocol not in the family");

    /* EMFILE once every descriptor is in use */
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    low = rl;
    low.rlim_cur = (rlim_t)lowest_free();
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    errno = 0;
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "restore the limit");
    check(fd == -1 && errno == EMFILE, "EMFILE");
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
