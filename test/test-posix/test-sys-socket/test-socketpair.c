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
#if defined(__PICOLIBC_POSIX_NETWORKING) && defined(__PICOLIBC_POSIX_FD_MGMT) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

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

/* socketpair makes a connected pair of local sockets */

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

/* Both ends have the same type, family and protocol */
static void
same_sockets(int sv[2], int type)
{
    int       value[2], i, opt;
    socklen_t len;
    int const opts[] = { SO_TYPE, SO_DOMAIN, SO_PROTOCOL };

    for (opt = 0; opt < 3; opt++) {
        for (i = 0; i < 2; i++) {
            len = sizeof(value[i]);
            check(getsockopt(sv[i], SOL_SOCKET, opts[opt], &value[i], &len) == 0, "getsockopt");
        }
        check(value[0] == value[1], "identical sockets");
        if (opts[opt] == SO_TYPE)
            check(value[0] == type, "socket type");
        if (opts[opt] == SO_DOMAIN)
            check(value[0] == AF_UNIX, "socket domain");
    }
}

/* glibc passes on Linux, which stores the descriptors before it fails */
static int
vector_unchanged(int sv[2])
{
#ifdef __GLIBC__
    (void)sv;
    return 1;
#else
    return sv[0] == -2 && sv[1] == -2;
#endif
}

int
main(void)
{
    struct rlimit rl, low;
    int           sv[2], a, b;
    char          buf[8];

    check(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");
    same_sockets(sv, SOCK_STREAM);
    check(write(sv[0], "ping", 4) == 4 && read(sv[1], buf, 4) == 4 && memcmp(buf, "ping", 4) == 0,
          "one way");
    check(write(sv[1], "pong", 4) == 4 && read(sv[0], buf, 4) == 4 && memcmp(buf, "pong", 4) == 0,
          "other way");
    check((fcntl(sv[0], F_GETFD) & FD_CLOEXEC) == 0 && (fcntl(sv[1], F_GETFL) & O_NONBLOCK) == 0,
          "no flags");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    /* Datagrams keep their boundaries */
    check(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == 0, "datagram pair");
    same_sockets(sv, SOCK_DGRAM);
    check(send(sv[0], "ab", 2, 0) == 2 && send(sv[0], "cde", 3, 0) == 3, "send datagrams");
    check(recv(sv[1], buf, sizeof(buf), 0) == 2 && recv(sv[1], buf, sizeof(buf), 0) == 3,
          "datagram boundaries");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");
    check(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) == 0, "sequenced-packet pair");
    same_sockets(sv, SOCK_SEQPACKET);
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    /* The flags apply to both descriptors */
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, sv) == 0, "flags");
    check((fcntl(sv[0], F_GETFD) & FD_CLOEXEC) && (fcntl(sv[1], F_GETFD) & FD_CLOEXEC),
          "SOCK_CLOEXEC");
    check((fcntl(sv[0], F_GETFL) & O_NONBLOCK) && (fcntl(sv[1], F_GETFL) & O_NONBLOCK),
          "SOCK_NONBLOCK");
    check(read(sv[0], buf, 1) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK), "non-blocking");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    /* The lowest free descriptors */
    a = lowest_free();
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair");
    check((sv[0] == a || sv[1] == a) && sv[0] != sv[1], "lowest descriptors");
    check(close(sv[0]) == 0 && close(sv[1]) == 0, "close");

    /* Errors leave the vector alone */
    sv[0] = sv[1] = -2;
    errno = 0;
    check(socketpair(AF_INET, SOCK_STREAM, 0, sv) == -1 && errno == EOPNOTSUPP, "EOPNOTSUPP");
    check(vector_unchanged(sv), "vector unchanged");
    errno = 0;
    check(socketpair(12345, SOCK_STREAM, 0, sv) == -1 && errno == EAFNOSUPPORT, "EAFNOSUPPORT");
    check(vector_unchanged(sv), "vector unchanged");
    errno = 0;
    check(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_TCP, sv) == -1 && errno == EPROTONOSUPPORT,
          "EPROTONOSUPPORT");
    check(vector_unchanged(sv), "vector unchanged");

    /* EMFILE with one descriptor free, which stays free */
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    a = lowest_free();
    b = open("/dev/null", O_RDONLY);
    check(b == a, "open");
    check(close(b) == 0, "close");
    low = rl;
    low.rlim_cur = (rlim_t)a + 1;
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    errno = 0;
    sv[0] = sv[1] = -2;
    b = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "restore the limit");
    check(b == -1 && errno == EMFILE, "EMFILE");
    check(vector_unchanged(sv), "vector unchanged after EMFILE");
    check(lowest_free() == a, "no descriptor allocated");
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
