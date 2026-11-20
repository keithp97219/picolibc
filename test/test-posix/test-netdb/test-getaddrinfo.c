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

/* getaddrinfo finds numeric, local and named addresses and services */

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

/* Whether every entry has this socket type and protocol (0 matches anything) */
static int
all(struct addrinfo *ai, int socktype, int protocol)
{
    for (; ai; ai = ai->ai_next)
        if ((socktype && ai->ai_socktype != socktype) || (protocol && ai->ai_protocol != protocol))
            return 0;
    return 1;
}

static int
count(struct addrinfo *ai, int family, int socktype)
{
    int n = 0;

    for (; ai; ai = ai->ai_next)
        if ((!family || ai->ai_family == family) && (!socktype || ai->ai_socktype == socktype))
            n++;
    return n;
}

int
main(void)
{
    struct addrinfo      hints, *res, *ai;
    struct sockaddr_in  *sin;
    struct sockaddr_in6 *sin6;
    static const char    zero[sizeof(sin->sin_zero)];
    int                  ret, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    check(getaddrinfo("127.0.0.1", "80", &hints, &res) == 0, "numeric");
    check(res && !res->ai_next && res->ai_family == AF_INET && res->ai_socktype == SOCK_STREAM,
          "one result");
    sin = (struct sockaddr_in *)res->ai_addr;
    check(res->ai_addrlen == sizeof(*sin) && sin->sin_port == htons(80)
              && sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK),
          "address");
    freeaddrinfo(res);

    /* Both stream and datagram, with the port from the service name */
    hints.ai_socktype = 0;
    check(getaddrinfo("127.0.0.1", "domain", &hints, &res) == 0, "service name");
    check(count(res, AF_INET, SOCK_STREAM) == 1 && count(res, AF_INET, SOCK_DGRAM) == 1, "types");
    check(((struct sockaddr_in *)res->ai_addr)->sin_port == htons(53), "port 53");
    freeaddrinfo(res);

    /* Without a host, the loopback or wildcard address */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    check(getaddrinfo(NULL, "1234", &hints, &res) == 0, "no host");
    check(count(res, AF_INET, 0) == 1 && count(res, AF_INET6, 0) == 1, "both families");
    freeaddrinfo(res);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    check(getaddrinfo(NULL, "1234", &hints, &res) == 0, "passive");
    check(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr == htonl(INADDR_ANY), "wildcard");
    freeaddrinfo(res);

    /* localhost, from the hosts file or the built-in entry */
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    check(getaddrinfo("localhost", NULL, &hints, &res) == 0, "localhost");
    check(res->ai_canonname != NULL, "canonical name");
    check(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr == htonl(INADDR_LOOPBACK),
          "loopback");
    freeaddrinfo(res);

    /* IPv6 and mapped addresses */
    hints.ai_flags = 0;
    hints.ai_family = AF_INET6;
    check(getaddrinfo("::1", NULL, &hints, &res) == 0 && res->ai_family == AF_INET6, "IPv6");
    freeaddrinfo(res);
    hints.ai_flags = AI_V4MAPPED;
    check(getaddrinfo("10.0.0.1", NULL, &hints, &res) == 0, "mapped");
    check(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)res->ai_addr)->sin6_addr),
          "mapped address");
    freeaddrinfo(res);

    /* No hints: any family, type and protocol */
    check(getaddrinfo("127.0.0.1", "80", NULL, &res) == 0, "no hints");
    check(count(res, AF_INET, SOCK_STREAM) == 1 && count(res, AF_INET, SOCK_DGRAM) == 1
              && count(res, 0, 0) == count(res, AF_INET, 0),
          "no hints results");
    freeaddrinfo(res);

    /* The socket type and protocol limit the results */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    check(getaddrinfo("127.0.0.1", "53", &hints, &res) == 0 && all(res, SOCK_DGRAM, IPPROTO_UDP),
          "datagram only");
    freeaddrinfo(res);
    hints.ai_socktype = 0;
    hints.ai_protocol = IPPROTO_TCP;
    check(getaddrinfo("127.0.0.1", "53", &hints, &res) == 0 && all(res, SOCK_STREAM, IPPROTO_TCP),
          "TCP only");
    freeaddrinfo(res);
    hints.ai_protocol = IPPROTO_ICMP;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0 && all(res, 0, IPPROTO_ICMP),
          "ICMP only");
    freeaddrinfo(res);
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_RAW;
    check(getaddrinfo("127.0.0.1", "80", &hints, &res) == EAI_SERVICE, "no ports for raw sockets");

    /* AI_PASSIVE is ignored with a host */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    check(getaddrinfo("127.0.0.1", "80", &hints, &res) == 0, "passive with a host");
    sin = (struct sockaddr_in *)res->ai_addr;
    check(sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK), "passive host address");
    freeaddrinfo(res);

    /* IPv6 loopback and wildcard addresses, with unset fields zero */
    hints.ai_family = AF_INET6;
    hints.ai_flags = 0;
    check(getaddrinfo(NULL, "80", &hints, &res) == 0 && res->ai_family == AF_INET6, "IPv6 local");
    sin6 = (struct sockaddr_in6 *)res->ai_addr;
    check(res->ai_addrlen == sizeof(*sin6) && sin6->sin6_family == AF_INET6
              && sin6->sin6_port == htons(80) && IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)
              && sin6->sin6_flowinfo == 0 && sin6->sin6_scope_id == 0,
          "IPv6 loopback");
    freeaddrinfo(res);
    hints.ai_flags = AI_PASSIVE;
    check(getaddrinfo(NULL, "80", &hints, &res) == 0, "IPv6 passive");
    sin6 = (struct sockaddr_in6 *)res->ai_addr;
    check(IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr), "IPv6 wildcard");
    freeaddrinfo(res);
    hints.ai_family = AF_INET;
    hints.ai_flags = 0;
    check(getaddrinfo(NULL, "80", &hints, &res) == 0, "IPv4 local");
    sin = (struct sockaddr_in *)res->ai_addr;
    check(sin->sin_family == AF_INET && sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK)
              && memcmp(sin->sin_zero, zero, sizeof(zero)) == 0,
          "IPv4 loopback");
    freeaddrinfo(res);

    /* A numeric host is its own canonical name */
    hints.ai_flags = AI_CANONNAME;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0 && res->ai_canonname
              && strcmp(res->ai_canonname, "127.0.0.1") == 0,
          "numeric canonical name");
    freeaddrinfo(res);

    /* AI_V4MAPPED only applies to AF_INET6, and AI_ALL only with AI_V4MAPPED */
    hints.ai_flags = AI_V4MAPPED;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0 && res->ai_family == AF_INET
              && count(res, AF_INET6, 0) == 0,
          "AI_V4MAPPED with AF_INET");
    freeaddrinfo(res);
    hints.ai_family = AF_UNSPEC;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0 && count(res, AF_INET6, 0) == 0,
          "AI_V4MAPPED with AF_UNSPEC");
    freeaddrinfo(res);
    hints.ai_family = AF_INET6;
    hints.ai_flags = 0;
    ret = getaddrinfo("127.0.0.1", NULL, &hints, &res);
    check(ret != 0, "IPv4 address for AF_INET6");
#ifdef __PICOLIBC__
    /* glibc returns its own EAI_ADDRFAMILY */
    check(ret == EAI_NONAME, "IPv4 address for AF_INET6 error");
#endif
    hints.ai_flags = AI_ALL;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) != 0, "AI_ALL without AI_V4MAPPED");
    hints.ai_flags = AI_V4MAPPED | AI_ALL;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0, "AI_V4MAPPED and AI_ALL");
    sin6 = (struct sockaddr_in6 *)res->ai_addr;
    check(res->ai_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)
              && sin6->sin6_addr.s6_addr[12] == 127 && sin6->sin6_addr.s6_addr[15] == 1,
          "mapped loopback");
    freeaddrinfo(res);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = 0;
    check(getaddrinfo("::1", NULL, &hints, &res) == 0 && count(res, AF_INET, 0) == 0
              && count(res, AF_INET6, 0) > 0,
          "IPv6 address for AF_UNSPEC");
    freeaddrinfo(res);
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    check(getaddrinfo("::1", "80", &hints, &res) == 0
              && ((struct sockaddr_in6 *)res->ai_addr)->sin6_port == htons(80),
          "numeric host and service");
    freeaddrinfo(res);

    /* The results work with socket and bind */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    check(getaddrinfo("127.0.0.1", "0", &hints, &res) == 0, "for bind");
    for (ai = res; ai; ai = ai->ai_next) {
        if (ai->ai_socktype != SOCK_STREAM && ai->ai_socktype != SOCK_DGRAM)
            continue;
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        check(fd >= 0, "socket from the results");
        check(bind(fd, ai->ai_addr, ai->ai_addrlen) == 0, "bind from the results");
        close(fd);
    }

    /* freeaddrinfo frees sublists, leaving errno alone */
    check(res->ai_next != NULL, "two results");
    ai = res->ai_next;
    res->ai_next = NULL;
    errno = EDOM;
    freeaddrinfo(ai);
    freeaddrinfo(res);
    check(errno == EDOM, "freeaddrinfo errno");

    /* A name in the reserved .invalid domain never resolves */
    memset(&hints, 0, sizeof(hints));
    ret = getaddrinfo("no-such-host.invalid", NULL, &hints, &res);
#ifdef __PICOLIBC__
    check(ret == EAI_NONAME, "missing host");
#else
    /* A resolver may also fail to reach its server */
    check(ret == EAI_NONAME || ret == EAI_AGAIN || ret == EAI_FAIL, "missing host");
#endif

    /* Failures */
    memset(&hints, 0, sizeof(hints));
    check(getaddrinfo(NULL, NULL, &hints, &res) == EAI_NONAME, "nothing to find");
    hints.ai_flags = AI_NUMERICHOST;
    check(getaddrinfo("localhost", NULL, &hints, &res) == EAI_NONAME, "AI_NUMERICHOST");
    hints.ai_flags = AI_NUMERICSERV;
    check(getaddrinfo("127.0.0.1", "domain", &hints, &res) == EAI_NONAME, "AI_NUMERICSERV");
    hints.ai_flags = 0;
    hints.ai_family = 12345;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == EAI_FAMILY, "EAI_FAMILY");
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 12345;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == EAI_SOCKTYPE, "EAI_SOCKTYPE");
    hints.ai_socktype = SOCK_STREAM;
    check(getaddrinfo("127.0.0.1", "no-such-service-here", &hints, &res) == EAI_SERVICE,
          "EAI_SERVICE");
    hints.ai_socktype = 0;
    hints.ai_flags = 0x40000000;
    check(getaddrinfo("127.0.0.1", NULL, &hints, &res) == EAI_BADFLAGS, "EAI_BADFLAGS");
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
