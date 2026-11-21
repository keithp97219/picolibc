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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

/* connect, listen, accept, getsockname and getpeername make a TCP connection */

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
static char dir[] = "/tmp/test-connect-XXXXXX";

static struct sockaddr_un
local_name(const char *name)
{
    struct sockaddr_un sun;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "%s/%s", dir, name);
    return sun;
}

/* connect a new AF_UNIX socket of the type to the name; keeps errno */
static int
connect_local(int type, const char *name)
{
    struct sockaddr_un sun = local_name(name);
    int                fd = make_socket(AF_UNIX, type), ret, err;

    ret = connect(fd, (struct sockaddr *)&sun, sizeof(sun));
    err = errno;
    check(close(fd) == 0, "close");
    errno = err;
    return ret;
}

static int
pending_error(int fd)
{
    int       err = -1;
    socklen_t len = sizeof(err);

    check(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0, "SO_ERROR");
    return err;
}

/*
 * Wait for the connection attempt to finish. select reports it writable
 * either way; poll reports POLLOUT for a connection, while a failure is
 * an error or hangup, which excludes POLLOUT
 */
static void
wait_writable(int fd, int connected)
{
    struct pollfd pfd = { .fd = fd, .events = POLLOUT };
    fd_set        wset;

    check(poll(&pfd, 1, -1) == 1, "poll");
    if (connected)
        check(pfd.revents == POLLOUT, "poll writable");
    else
        check(pfd.revents & (POLLERR | POLLHUP), "poll error");
    FD_ZERO(&wset);
    FD_SET(fd, &wset);
    check(select(fd + 1, NULL, &wset, NULL, NULL) == 1 && FD_ISSET(fd, &wset), "select writable");
}

int
main(void)
{
    struct sockaddr_in  a, b, listen_addr;
    struct sockaddr_in6 sin6;
    struct sockaddr_un  sun;
    socklen_t           len;
    int                 client, server, listener, s1, s2, s3, fds[2], ret;
    char                buf[8];

    tcp_pair(&client, &server);
    len = sizeof(a);
    check(getsockname(client, (struct sockaddr *)&a, &len) == 0 && len == sizeof(a), "client name");
    /* connect bound the client to an unused port */
    check(ntohs(a.sin_port) != 0, "bound by connect");
    len = sizeof(b);
    check(getpeername(server, (struct sockaddr *)&b, &len) == 0, "server peer");
    check(a.sin_family == AF_INET && b.sin_family == AF_INET, "families");
    check(a.sin_port == b.sin_port && a.sin_addr.s_addr == b.sin_addr.s_addr, "same address");
    check(write(client, "hi", 2) == 2 && read(server, buf, 2) == 2 && memcmp(buf, "hi", 2) == 0,
          "data");
    /* Already connected */
    len = sizeof(b);
    check(getpeername(client, (struct sockaddr *)&b, &len) == 0, "client peer");
    errno = 0;
    check(connect(client, (struct sockaddr *)&b, sizeof(b)) == -1 && errno == EISCONN, "EISCONN");
    check(close(client) == 0 && read(server, buf, 1) == 0, "end of file after close");
    check(close(server) == 0, "close");
    /* Nobody listening on the port connect just used */
    client = make_socket(AF_INET, SOCK_STREAM);
    check(connect(client, (struct sockaddr *)&a, sizeof(a)) == -1 && errno == ECONNREFUSED,
          "ECONNREFUSED");
    check(close(client) == 0, "close");

    /* A non-blocking connect completes asynchronously and is then writable */
    listener = make_socket(AF_INET, SOCK_STREAM);
    listen_addr = loopback();
    len = sizeof(listen_addr);
    check(bind(listener, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == 0, "bind");
    check(listen(listener, 1) == 0, "listen");
    check(getsockname(listener, (struct sockaddr *)&listen_addr, &len) == 0, "getsockname");
    client = make_socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK);
    ret = connect(client, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    check(ret == 0 || errno == EINPROGRESS, "EINPROGRESS");
    wait_writable(client, 1);
    check(pending_error(client) == 0, "connected");
    server = accept(listener, NULL, NULL);
    check(server >= 0, "accept");
    check(close(server) == 0 && close(client) == 0, "close");
    /* A failed one leaves the error pending, and is writable too */
    check(close(listener) == 0, "close listener");
    client = make_socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK);
    ret = connect(client, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    if (ret == -1 && errno == EINPROGRESS) {
        wait_writable(client, 0);
        check(pending_error(client) == ECONNREFUSED, "pending ECONNREFUSED");
    } else {
        check(ret == -1 && errno == ECONNREFUSED, "immediate ECONNREFUSED");
    }
    check(close(client) == 0, "close");

    /* Datagram sockets: connect sets and resets the peer */
    s1 = make_socket(AF_INET, SOCK_DGRAM);
    s2 = make_socket(AF_INET, SOCK_DGRAM);
    s3 = make_socket(AF_INET, SOCK_DGRAM);
    a = loopback();
    b = loopback();
    check(bind(s1, (struct sockaddr *)&a, sizeof(a)) == 0
              && bind(s2, (struct sockaddr *)&b, sizeof(b)) == 0,
          "bind datagram sockets");
    len = sizeof(a);
    check(getsockname(s1, (struct sockaddr *)&a, &len) == 0, "name s1");
    len = sizeof(b);
    check(getsockname(s2, (struct sockaddr *)&b, &len) == 0, "name s2");
    check(connect(s1, (struct sockaddr *)&b, sizeof(b)) == 0, "connect datagram socket");
    check(send(s1, "one", 3, 0) == 3, "send to the peer");
    check(recv(s2, buf, sizeof(buf), 0) == 3 && memcmp(buf, "one", 3) == 0, "peer got it");
    /* Only the peer's datagrams arrive */
    check(sendto(s3, "bad", 3, 0, (struct sockaddr *)&a, sizeof(a)) == 3, "other sender");
    check(sendto(s2, "two", 3, 0, (struct sockaddr *)&a, sizeof(a)) == 3, "peer sends");
    check(recv(s1, buf, sizeof(buf), 0) == 3 && memcmp(buf, "two", 3) == 0, "only from the peer");
    check(recv(s1, buf, sizeof(buf), MSG_DONTWAIT) == -1
              && (errno == EAGAIN || errno == EWOULDBLOCK),
          "other sender filtered");
    memset(&b, 0, sizeof(b));
    b.sin_family = AF_UNSPEC;
    check(connect(s1, (struct sockaddr *)&b, sizeof(b)) == 0, "AF_UNSPEC");
    len = sizeof(b);
    errno = 0;
    check(getpeername(s1, (struct sockaddr *)&b, &len) == -1 && errno == ENOTCONN, "peer reset");
    errno = 0;
    check(send(s1, "x", 1, 0) == -1 && errno == EDESTADDRREQ, "no peer");
    check(close(s1) == 0 && close(s2) == 0 && close(s3) == 0, "close");

    /* An address of another family */
    client = make_socket(AF_INET, SOCK_STREAM);
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(1);
    errno = 0;
    check(connect(client, (struct sockaddr *)&sin6, sizeof(sin6)) == -1 && errno == EAFNOSUPPORT,
          "EAFNOSUPPORT");
    check(close(client) == 0, "close");

    /* AF_UNIX names */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    s1 = make_socket(AF_UNIX, SOCK_DGRAM);
    sun = local_name("dgram");
    check(bind(s1, (struct sockaddr *)&sun, sizeof(sun)) == 0, "bind datagram name");
    errno = 0;
    check(connect_local(SOCK_STREAM, "dgram") == -1 && errno == EPROTOTYPE, "EPROTOTYPE");
    check(connect_local(SOCK_DGRAM, "dgram") == 0, "datagram peer");
    s2 = make_socket(AF_UNIX, SOCK_STREAM);
    sun = local_name("stream");
    check(bind(s2, (struct sockaddr *)&sun, sizeof(sun)) == 0, "bind stream name");
    errno = 0;
    check(connect_local(SOCK_STREAM, "stream") == -1 && errno == ECONNREFUSED, "not listening");
    errno = 0;
    check(connect_local(SOCK_STREAM, "missing") == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(connect_local(SOCK_STREAM, "missing/sock") == -1 && errno == ENOENT, "ENOENT prefix");
    errno = 0;
    check(connect_local(SOCK_STREAM, "dgram/sock") == -1 && errno == ENOTDIR, "ENOTDIR");
    errno = 0;
    check(connect_local(SOCK_STREAM, "stream/") == -1 && errno == ENOTDIR, "trailing slash");
    check(symlink("loop", local_name("loop").sun_path) == 0, "symlink");
    errno = 0;
    check(connect_local(SOCK_STREAM, "loop/sock") == -1 && errno == ELOOP, "ELOOP");
    check(close(s1) == 0 && close(s2) == 0, "close");
    check(unlink(local_name("dgram").sun_path) == 0 && unlink(local_name("stream").sun_path) == 0
              && unlink(local_name("loop").sun_path) == 0 && rmdir(dir) == 0,
          "clean up");

    check(pipe(fds) == 0, "pipe");
    a = loopback();
    errno = 0;
    check(connect(fds[0], (struct sockaddr *)&a, sizeof(a)) == -1 && errno == ENOTSOCK, "ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(connect(-1, (struct sockaddr *)&a, sizeof(a)) == -1 && errno == EBADF, "EBADF");
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
