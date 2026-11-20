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

/* inet_pton converts IPv4 and IPv6 text to binary, rejecting bad forms */

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
v6(const char *src, const char *hex)
{
    unsigned char addr[16], want[16];
    int           i;

    if (inet_pton(AF_INET6, src, addr) != 1)
        return 0;
    for (i = 0; i < 16; i++) {
        unsigned b;

        sscanf(hex + 2 * i, "%2x", &b);
        want[i] = (unsigned char)b;
    }
    return memcmp(addr, want, 16) == 0;
}

int
main(void)
{
    struct in_addr in;
    unsigned char  addr[16];

    check(inet_pton(AF_INET, "192.168.1.255", &in) == 1 && in.s_addr == htonl(0xc0a801ff), "IPv4");
    check(inet_pton(AF_INET, "0.0.0.0", &in) == 1 && in.s_addr == 0, "zero");
    check(inet_pton(AF_INET, "256.1.1.1", &in) == 0, "octet too big");
    check(inet_pton(AF_INET, "1.2.3", &in) == 0, "too few parts");
    check(inet_pton(AF_INET, "1.2.3.4.5", &in) == 0, "too many parts");
#ifdef __PICOLIBC__
    /* POSIX allows leading zeros in each decimal part; glibc rejects them */
    check(inet_pton(AF_INET, "001.002.003.010", &in) == 1 && in.s_addr == htonl(0x0102030a),
          "leading zeros");
    check(v6("::ffff:01.002.3.4", "00000000000000000000ffff01020304"),
          "leading zeros in IPv4 tail");
#endif
    check(inet_pton(AF_INET, "0001.2.3.4", &in) == 0, "four digits");
    check(inet_pton(AF_INET, "1.2.3.4 ", &in) == 0, "trailing space");
    check(inet_pton(AF_INET, " 1.2.3.4", &in) == 0, "leading space");
    check(inet_pton(AF_INET, "0x1.2.3.4", &in) == 0, "hexadecimal");
    check(inet_pton(AF_INET, "1.2.3.", &in) == 0, "empty part");
    check(inet_pton(AF_INET, "", &in) == 0, "empty");

    /* A failed conversion leaves the destination alone */
    in.s_addr = htonl(0x01020304);
    check(inet_pton(AF_INET, "1.2.3", &in) == 0 && in.s_addr == htonl(0x01020304), "unchanged");

    check(v6("::", "00000000000000000000000000000000"), "::");
    check(v6("::1", "00000000000000000000000000000001"), "::1");
    check(v6("2001:db8::8:800:200c:417a", "20010db80000000000080800200c417a"), "compressed");
    check(v6("1:2:3:4:5:6:7:8", "00010002000300040005000600070008"), "full");
    check(v6("::ffff:1.2.3.4", "00000000000000000000ffff01020304"), "IPv4-mapped");
    check(v6("fe80::", "fe800000000000000000000000000000"), "trailing ::");
    check(v6("ABCD::EF", "abcd00000000000000000000000000ef"), "upper case");
    check(inet_pton(AF_INET6, "1::2::3", addr) == 0, "two ::");
    check(inet_pton(AF_INET6, "12345::", addr) == 0, "group too long");
    check(inet_pton(AF_INET6, "1:2:3:4:5:6:7:8:9", addr) == 0, "too many groups");
    check(inet_pton(AF_INET6, "1:2:3:4:5:6:7", addr) == 0, "too few groups");
    check(inet_pton(AF_INET6, ":1::", addr) == 0, "lone leading colon");
    check(inet_pton(AF_INET6, "1:", addr) == 0, "trailing colon");
    check(v6("1:2:3:4:5:6:1.2.3.4", "00010002000300040005000601020304"), "IPv4 tail");
    check(v6("::1.2.3.4", "00000000000000000000000001020304"), "compressed with an IPv4 tail");
    check(v6("1:2:3:4:5:6:7::", "00010002000300040005000600070000"), ":: for one group");
    check(v6("0001:02:003:0004::", "00010002000300040000000000000000"), "leading zeros in groups");
    check(inet_pton(AF_INET6, "1:2:3:4:5:6:7:1.2.3.4", addr) == 0, "IPv4 tail too late");
    check(inet_pton(AF_INET6, "1:2:3:4:5:6:7:8::", addr) == 0, ":: with eight groups");
    check(inet_pton(AF_INET6, "::ffff:1.2.3", addr) == 0, "short IPv4 tail");
    check(inet_pton(AF_INET6, "1.2.3.4", addr) == 0, "IPv4 only");
    check(inet_pton(AF_INET6, "1:2:3:4:5:6:7:g", addr) == 0, "not hexadecimal");
    check(inet_pton(AF_INET6, "", addr) == 0, "empty");
    errno = 0;
    check(inet_pton(12345, "1.2.3.4", addr) == -1 && errno == EAFNOSUPPORT, "EAFNOSUPPORT");
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
