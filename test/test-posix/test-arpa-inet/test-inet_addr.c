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

/* inet_addr converts the traditional IPv4 forms */

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
    check(inet_addr("1.2.3.4") == htonl(0x01020304), "dotted quad");
    check(inet_addr("1.2.772") == htonl(0x01020304), "three parts");
    check(inet_addr("1.131844") == htonl(0x01020304), "two parts");
    check(inet_addr("16909060") == htonl(0x01020304), "one part");
    check(inet_addr("0x7f.0.0.01") == htonl(0x7f000001), "hex and octal");
    check(inet_addr("255.255.255.255") == htonl(0xffffffff), "broadcast");
    check(inet_addr("1.2.3.256") == (in_addr_t)-1, "octet too big");
    check(inet_addr("08.1.1.1") == (in_addr_t)-1, "bad octal");
    check(inet_addr("1.2.3.4x") == (in_addr_t)-1, "trailing junk");
    check(inet_addr("") == (in_addr_t)-1, "empty");

    /* Each form, with the parts in any base */
    check(inet_addr("0377.0xff.255.0") == htonl(0xffffff00), "mixed bases");
    check(inet_addr("0X7F.1") == htonl(0x7f000001), "upper case 0X");
    check(inet_addr("10.0xffffff") == htonl(0x0affffff), "24-bit part");
    check(inet_addr("10.1.0xffff") == htonl(0x0a01ffff), "16-bit part");
    check(inet_addr("0") == 0, "zero");
    check(inet_addr("10.0x1000000") == (in_addr_t)-1, "24-bit part too big");
    check(inet_addr("10.1.65536") == (in_addr_t)-1, "16-bit part too big");
    check(inet_addr("4294967296") == (in_addr_t)-1, "32-bit part too big");
    check(inet_addr("1.2.3.4.5") == (in_addr_t)-1, "five parts");
    check(inet_addr("1..2") == (in_addr_t)-1, "empty part");
    check(inet_addr("1.2.3.") == (in_addr_t)-1, "trailing dot");
    check(inet_addr("0x1g") == (in_addr_t)-1, "bad hex");

    /* The result is in network byte order */
    {
        in_addr_t      a = inet_addr("1.2.3.4");
        unsigned char *p = (unsigned char *)&a;

        check(p[0] == 1 && p[1] == 2 && p[2] == 3 && p[3] == 4, "network order");
    }
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
