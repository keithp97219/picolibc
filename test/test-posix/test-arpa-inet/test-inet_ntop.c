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

/* inet_ntop formats addresses, compressing IPv6 zeros as RFC 5952 describes */

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

static int
v6(const char *src)
{
    unsigned char addr[16];
    char          buf[INET6_ADDRSTRLEN];

    return inet_pton(AF_INET6, src, addr) == 1 && inet_ntop(AF_INET6, addr, buf, sizeof(buf)) == buf
        && strcmp(buf, src) == 0;
}

int
main(void)
{
    struct in_addr in;
    char           buf[INET6_ADDRSTRLEN];

    in.s_addr = htonl(0x7f000001);
    check(inet_ntop(AF_INET, &in, buf, sizeof(buf)) == buf && strcmp(buf, "127.0.0.1") == 0,
          "IPv4");
    check(inet_ntop(AF_INET, &in, buf, 9) == NULL && errno == ENOSPC, "ENOSPC");
    check(v6("::"), "::");
    check(v6("::1"), "::1");
    check(v6("2001:db8::1"), "compressed");
    check(v6("2001:db8:0:1:1:1:1:1"), "single zero group not compressed");
    check(v6("2001:0:0:1::1"), "longest run compressed");
    check(v6("1::4:0:0:7:8"), "first of equal runs compressed");
    check(v6("::ffff:1.2.3.4"), "IPv4-mapped");
    check(v6("fe80::1"), "link local");
    check(inet_ntop(12345, &in, buf, sizeof(buf)) == NULL && errno == EAFNOSUPPORT, "EAFNOSUPPORT");

    /* The buffer sizes from <netinet/in.h> are enough; one byte less is not */
    in.s_addr = htonl(0xffffffff);
    check(inet_ntop(AF_INET, &in, buf, INET_ADDRSTRLEN) == buf
              && strcmp(buf, "255.255.255.255") == 0,
          "INET_ADDRSTRLEN");
    errno = 0;
    check(inet_ntop(AF_INET, &in, buf, 15) == NULL && errno == ENOSPC, "IPv4 one byte short");
    {
        unsigned char addr[16];

        memset(addr, 0xff, sizeof(addr));
        check(inet_ntop(AF_INET6, addr, buf, INET6_ADDRSTRLEN) == buf
                  && strcmp(buf, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") == 0,
              "longest IPv6");
        errno = 0;
        check(inet_ntop(AF_INET6, addr, buf, 39) == NULL && errno == ENOSPC, "IPv6 one byte short");
        check(inet_pton(AF_INET6, "::1", addr) == 1, "::1");
        errno = 0;
        check(inet_ntop(AF_INET6, addr, buf, 3) == NULL && errno == ENOSPC, "IPv6 ENOSPC");
        check(inet_ntop(AF_INET6, addr, buf, 4) == buf && strcmp(buf, "::1") == 0,
              "IPv6 just fits");
    }

    /* IPv4 text converts back to the same address */
    check(inet_pton(AF_INET, "10.20.30.40", &in) == 1
              && inet_ntop(AF_INET, &in, buf, sizeof(buf)) == buf
              && strcmp(buf, "10.20.30.40") == 0,
          "IPv4 round trip");
    check(v6("1:2:3:4:5:6:7:8"), "no zeros");
    check(v6("1::"), "trailing zeros");
    (void)make_socket;
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
