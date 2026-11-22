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
#if TEST_POSIX_NETWORKING && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

/* getpeername and getsockname report the two ends of a socket */

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

/* A fresh directory for the AF_UNIX names */
static char dir[] = "/tmp/test-getpeername-XXXXXX";

static struct sockaddr_un
local_name(const char *name)
{
    struct sockaddr_un sun;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "%s/%s", dir, name);
    return sun;
}

int
main(void)
{
    struct sockaddr_in sin, peer, name;
    struct sockaddr_un sun, got;
    unsigned char      small[sizeof(struct sockaddr_in)];
    socklen_t          len;
    int                client, server, fd, a, b, fds[2];

    tcp_pair(&client, &server);
    len = sizeof(name);
    check(getsockname(client, (struct sockaddr *)&name, &len) == 0, "getsockname");
    check(len == sizeof(name) && name.sin_family == AF_INET
              && name.sin_addr.s_addr == htonl(INADDR_LOOPBACK) && name.sin_port != 0,
          "local name");
    len = sizeof(peer);
    check(getpeername(server, (struct sockaddr *)&peer, &len) == 0, "getpeername");
    check(len == sizeof(peer) && peer.sin_port == name.sin_port
              && peer.sin_addr.s_addr == name.sin_addr.s_addr,
          "peer name");
    len = sizeof(sin);
    check(getsockname(server, (struct sockaddr *)&sin, &len) == 0, "server name");
    len = sizeof(peer);
    check(getpeername(client, (struct sockaddr *)&peer, &len) == 0 && peer.sin_port == sin.sin_port,
          "client's peer");

    /* Short buffers get the start of the address and the full length */
    memset(small, 0xa5, sizeof(small));
    len = 4;
    check(getsockname(client, (struct sockaddr *)small, &len) == 0, "short getsockname");
    check(len == sizeof(struct sockaddr_in) && small[4] == 0xa5 && memcmp(small, &name, 4) == 0,
          "truncated local name");
    memset(small, 0xa5, sizeof(small));
    len = 4;
    check(getpeername(server, (struct sockaddr *)small, &len) == 0, "short getpeername");
    check(len == sizeof(struct sockaddr_in) && small[4] == 0xa5 && memcmp(small, &name, 4) == 0,
          "truncated peer name");
    check(close(client) == 0 && close(server) == 0, "close");

    /* Not connected */
    fd = make_socket(AF_INET, SOCK_STREAM);
    len = sizeof(peer);
    errno = 0;
    check(getpeername(fd, (struct sockaddr *)&peer, &len) == -1 && errno == ENOTCONN, "ENOTCONN");
    sin = loopback();
    check(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0 && listen(fd, 1) == 0, "listen");
    len = sizeof(peer);
    errno = 0;
    check(getpeername(fd, (struct sockaddr *)&peer, &len) == -1 && errno == ENOTCONN,
          "listening socket");
    check(close(fd) == 0, "close");
    fd = make_socket(AF_INET, SOCK_DGRAM);
    len = sizeof(peer);
    errno = 0;
    check(getpeername(fd, (struct sockaddr *)&peer, &len) == -1 && errno == ENOTCONN,
          "datagram socket without a peer");
    check(close(fd) == 0, "close");

    /* AF_UNIX names */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    a = make_socket(AF_UNIX, SOCK_DGRAM);
    b = make_socket(AF_UNIX, SOCK_DGRAM);
    sun = local_name("a");
    check(bind(a, (struct sockaddr *)&sun, sizeof(sun)) == 0, "bind");
    check(connect(b, (struct sockaddr *)&sun, sizeof(sun)) == 0, "connect");
    memset(&got, 0, sizeof(got));
    len = sizeof(got);
    check(getsockname(a, (struct sockaddr *)&got, &len) == 0 && got.sun_family == AF_UNIX
              && strcmp(got.sun_path, sun.sun_path) == 0,
          "AF_UNIX name");
    memset(&got, 0, sizeof(got));
    len = sizeof(got);
    check(getpeername(b, (struct sockaddr *)&got, &len) == 0 && got.sun_family == AF_UNIX
              && strcmp(got.sun_path, sun.sun_path) == 0,
          "AF_UNIX peer");
    check(close(a) == 0 && close(b) == 0 && unlink(sun.sun_path) == 0 && rmdir(dir) == 0,
          "clean up");

    /* Not sockets */
    check(pipe(fds) == 0, "pipe");
    len = sizeof(peer);
    errno = 0;
    check(getsockname(fds[0], (struct sockaddr *)&peer, &len) == -1 && errno == ENOTSOCK,
          "getsockname ENOTSOCK");
    errno = 0;
    check(getpeername(fds[0], (struct sockaddr *)&peer, &len) == -1 && errno == ENOTSOCK,
          "getpeername ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(getsockname(-1, (struct sockaddr *)&peer, &len) == -1 && errno == EBADF,
          "getsockname EBADF");
    errno = 0;
    check(getpeername(-1, (struct sockaddr *)&peer, &len) == -1 && errno == EBADF,
          "getpeername EBADF");
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
