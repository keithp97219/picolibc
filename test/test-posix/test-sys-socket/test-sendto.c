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
#if defined(__PICOLIBC_POSIX_NETWORKING)

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

/* sendto and recvfrom pass datagrams with their addresses */

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
static char dir[] = "/tmp/test-sendto-XXXXXX";

static struct sockaddr_un
local_name(const char *name)
{
    struct sockaddr_un sun;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "%s/%s", dir, name);
    return sun;
}

/* sendto a name in the directory; keeps errno */
static ssize_t
send_local(int fd, const char *name)
{
    struct sockaddr_un sun = local_name(name);

    return sendto(fd, "x", 1, 0, (struct sockaddr *)&sun, sizeof(sun));
}

int
main(void)
{
    static char         big[70000];
    struct sockaddr_in  a = loopback(), b = loopback(), c = loopback(), from;
    struct sockaddr_in6 sin6;
    struct sockaddr_un  sun;
    socklen_t           len = sizeof(a);
    int     sa = make_socket(AF_INET, SOCK_DGRAM), sb = make_socket(AF_INET, SOCK_DGRAM);
    int     sc = make_socket(AF_INET, SOCK_DGRAM), client, server, fd, u, fds[2];
    char    buf[16];
    ssize_t ret;

    check(bind(sa, (struct sockaddr *)&a, sizeof(a)) == 0, "bind a");
    check(getsockname(sa, (struct sockaddr *)&a, &len) == 0, "name a");
    len = sizeof(b);
    check(bind(sb, (struct sockaddr *)&b, sizeof(b)) == 0, "bind b");
    check(getsockname(sb, (struct sockaddr *)&b, &len) == 0, "name b");
    len = sizeof(c);
    check(bind(sc, (struct sockaddr *)&c, sizeof(c)) == 0, "bind c");
    check(getsockname(sc, (struct sockaddr *)&c, &len) == 0, "name c");
    check(sendto(sa, "datagram", 8, 0, (struct sockaddr *)&b, sizeof(b)) == 8, "sendto");
    len = sizeof(from);
    check(recvfrom(sb, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len) == 8, "recvfrom");
    check(memcmp(buf, "datagram", 8) == 0, "contents");
    check(from.sin_port == a.sin_port && from.sin_addr.s_addr == a.sin_addr.s_addr, "sender");
    check(recvfrom(sb, buf, sizeof(buf), MSG_DONTWAIT, NULL, NULL) == -1
              && (errno == EAGAIN || errno == EWOULDBLOCK),
          "empty");
    /* With a peer set, an address either overrides it or is refused */
    check(connect(sa, (struct sockaddr *)&b, sizeof(b)) == 0, "connect");
    ret = sendto(sa, "other", 5, 0, (struct sockaddr *)&c, sizeof(c));
    if (ret == 5)
        check(recv(sc, buf, sizeof(buf), 0) == 5 && memcmp(buf, "other", 5) == 0,
              "override the peer");
    else
        check(ret == -1 && errno == EISCONN, "EISCONN");
    /* Too large */
    errno = 0;
    check(sendto(sc, big, sizeof(big), 0, (struct sockaddr *)&b, sizeof(b)) == -1
              && errno == EMSGSIZE,
          "EMSGSIZE");
    /* An address of another family */
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(9);
    errno = 0;
    check(sendto(sc, "x", 1, 0, (struct sockaddr *)&sin6, sizeof(sin6)) == -1
              && errno == EAFNOSUPPORT,
          "EAFNOSUPPORT");
    /* Broadcasts need SO_BROADCAST; nothing is sent without it */
    memset(&from, 0, sizeof(from));
    from.sin_family = AF_INET;
    from.sin_port = htons(9);
    from.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    errno = 0;
    ret = sendto(sc, "x", 1, 0, (struct sockaddr *)&from, sizeof(from));
    /* Without a route for broadcasts there's nothing to check */
    if (!(ret == -1 && (errno == ENETUNREACH || errno == EHOSTUNREACH)))
        check(ret == -1 && errno == EACCES, "SO_BROADCAST needed");
    check(close(sa) == 0 && close(sb) == 0 && close(sc) == 0, "close");

    /* A connection-mode socket ignores the address, or refuses it */
    tcp_pair(&client, &server);
    ret = sendto(client, "y", 1, 0, (struct sockaddr *)&c, sizeof(c));
    if (ret == 1)
        check(recv(server, buf, 1, 0) == 1 && buf[0] == 'y', "address ignored");
    else
        check(ret == -1 && errno == EISCONN, "connection-mode EISCONN");
    check(close(client) == 0 && close(server) == 0, "close");

    /* AF_UNIX names */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    fd = make_socket(AF_UNIX, SOCK_DGRAM);
    sun = local_name("sock");
    check(bind(fd, (struct sockaddr *)&sun, sizeof(sun)) == 0, "bind");
    u = make_socket(AF_UNIX, SOCK_DGRAM);
    check(send_local(u, "sock") == 1 && recv(fd, buf, sizeof(buf), 0) == 1, "send to a name");
    errno = 0;
    check(send_local(u, "missing") == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(send_local(u, "missing/sock") == -1 && errno == ENOENT, "ENOENT prefix");
    errno = 0;
    check(send_local(u, "sock/x") == -1 && errno == ENOTDIR, "ENOTDIR");
    errno = 0;
    check(send_local(u, "sock/") == -1 && errno == ENOTDIR, "trailing slash");
    check(symlink("loop", local_name("loop").sun_path) == 0, "symlink");
    errno = 0;
    check(send_local(u, "loop/sock") == -1 && errno == ELOOP, "ELOOP");
    check(close(fd) == 0 && close(u) == 0, "close");
    check(unlink(local_name("sock").sun_path) == 0 && unlink(local_name("loop").sun_path) == 0
              && rmdir(dir) == 0,
          "clean up");

    check(pipe(fds) == 0, "pipe");
    errno = 0;
    check(sendto(fds[1], "x", 1, 0, (struct sockaddr *)&b, sizeof(b)) == -1 && errno == ENOTSOCK,
          "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(sendto(-1, "x", 1, 0, (struct sockaddr *)&b, sizeof(b)) == -1 && errno == EBADF, "EBADF");
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
