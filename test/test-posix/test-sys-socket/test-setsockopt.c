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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                       \
    || (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
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

/* QEMU's user-mode emulation, which handles some options itself */
static bool
is_qemu(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char  line[256];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "BogoMIPS", 8) == 0) {
            ret = strstr(line, "100.00") != NULL;
            break;
        }
    }
    fclose(f);
    return ret;
}

static int
get_int(int fd, int name)
{
    int       value = -1;
    socklen_t len = sizeof(value);

    check(getsockopt(fd, SOL_SOCKET, name, &value, &len) == 0 && len == sizeof(value),
          "getsockopt");
    return value;
}

/* Turn a boolean option on and off */
static void
flip(int fd, int name)
{
    int on = 1, off = 0;

    check(setsockopt(fd, SOL_SOCKET, name, &on, sizeof(on)) == 0 && get_int(fd, name) != 0,
          "option on");
    check(setsockopt(fd, SOL_SOCKET, name, &off, sizeof(off)) == 0 && get_int(fd, name) == 0,
          "option off");
}

int
main(void)
{
    struct timeval tv = { 0, 100000 }, got;
    struct linger  linger = { 1, 5 }, glinger;
    socklen_t      len;
    int            fd = make_socket(AF_INET, SOCK_DGRAM), on = 1, value, tcp, fds[2];
    char           buf[4];
    bool           qemu = is_qemu();

    check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0, "SO_REUSEADDR");
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, &len) == 0 && value != 0, "get");
    check(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0, "SO_RCVTIMEO");
    len = sizeof(got);
    check(getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &got, &len) == 0, "get SO_RCVTIMEO");
    /*
     * QEMU's user-mode emulation reports nothing for the timeouts, or,
     * for the 64-bit time options 32-bit targets use, an unconverted value
     */
    if (!qemu)
        check(len == sizeof(got) && got.tv_sec == 0 && got.tv_usec >= 90000
                  && got.tv_usec <= 110000,
              "timeout value");
    /* A short buffer gets the start of the value */
    memset(&got, 0xa5, sizeof(got));
    len = sizeof(got.tv_sec);
    check(getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &got, &len) == 0, "short SO_RCVTIMEO");
    if (!qemu)
        check(len == sizeof(got.tv_sec) && got.tv_sec == 0, "truncated timeout");
    len = 2;
    value = -1;
    check(getsockopt(fd, SOL_SOCKET, SO_TYPE, &value, &len) == 0 && len <= 2, "short SO_TYPE");
    {
        struct sockaddr_in sin = loopback();
        check(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    }
    /* The receive times out */
    check(recv(fd, buf, sizeof(buf), 0) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK),
          "timed out");
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    check(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0, "SO_SNDTIMEO");
    len = sizeof(got);
    check(getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &got, &len) == 0, "get SO_SNDTIMEO");
    if (!qemu)
        check(got.tv_sec == 1 && got.tv_usec == 0, "send timeout value");
    /* Microseconds out of range don't fit */
    tv.tv_usec = 1000000;
    errno = 0;
    check(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 && errno == EDOM, "EDOM");
    check(setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == 0, "SO_LINGER");
    len = sizeof(glinger);
    check(getsockopt(fd, SOL_SOCKET, SO_LINGER, &glinger, &len) == 0, "get SO_LINGER");
    check(glinger.l_onoff != 0 && glinger.l_linger == 5, "linger value");
    errno = 0;
    check(setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, 1) == -1 && errno == EINVAL,
          "short SO_LINGER");
    len = sizeof(value);
    check(getsockopt(fd, SOL_SOCKET, SO_ERROR, &value, &len) == 0 && value == 0, "SO_ERROR");
    check(getsockopt(fd, SOL_SOCKET, 12345, &value, &len) == -1 && errno == ENOPROTOOPT,
          "ENOPROTOOPT");
    errno = 0;
    check(setsockopt(fd, SOL_SOCKET, 12345, &on, sizeof(on)) == -1
              && (errno == ENOPROTOOPT || errno == EINVAL),
          "set ENOPROTOOPT");

    /* The read-only options describe the socket */
    check(get_int(fd, SO_TYPE) == SOCK_DGRAM, "SO_TYPE");
    check(get_int(fd, SO_DOMAIN) == AF_INET, "SO_DOMAIN");
    check(get_int(fd, SO_PROTOCOL) == IPPROTO_UDP, "SO_PROTOCOL");
    check(get_int(fd, SO_ACCEPTCONN) == 0, "SO_ACCEPTCONN");
    (void)get_int(fd, SO_DEBUG);
    /* Boolean options */
    flip(fd, SO_BROADCAST);
    flip(fd, SO_DONTROUTE);
    flip(fd, SO_REUSEADDR);
    tcp = make_socket(AF_INET, SOCK_STREAM);
    flip(tcp, SO_KEEPALIVE);
    flip(tcp, SO_OOBINLINE);
    /* Sizes may be adjusted, but not below what was asked */
    value = 65536;
    check(setsockopt(tcp, SOL_SOCKET, SO_RCVBUF, &value, sizeof(value)) == 0
              && get_int(tcp, SO_RCVBUF) >= value,
          "SO_RCVBUF");
    check(setsockopt(tcp, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) == 0
              && get_int(tcp, SO_SNDBUF) >= value,
          "SO_SNDBUF");
    value = 4;
    check(setsockopt(tcp, SOL_SOCKET, SO_RCVLOWAT, &value, sizeof(value)) == 0
              && get_int(tcp, SO_RCVLOWAT) == 4,
          "SO_RCVLOWAT");
    check(get_int(tcp, SO_SNDLOWAT) >= 1, "SO_SNDLOWAT");
    check(get_int(tcp, SO_PROTOCOL) == IPPROTO_TCP, "TCP protocol");
    check(close(tcp) == 0, "close");
    check(close(fd) == 0, "close");

    /* Not sockets */
    check(pipe(fds) == 0, "pipe");
    len = sizeof(value);
    errno = 0;
    check(getsockopt(fds[0], SOL_SOCKET, SO_TYPE, &value, &len) == -1 && errno == ENOTSOCK,
          "getsockopt ENOTSOCK");
    errno = 0;
    check(setsockopt(fds[0], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1 && errno == ENOTSOCK,
          "setsockopt ENOTSOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(getsockopt(-1, SOL_SOCKET, SO_TYPE, &value, &len) == -1 && errno == EBADF,
          "getsockopt EBADF");
    errno = 0;
    check(setsockopt(-1, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1 && errno == EBADF,
          "setsockopt EBADF");
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
