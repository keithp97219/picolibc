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
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* getnameinfo turns socket addresses into host and service names */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifdef __PICOLIBC_NETWORK_DNS
#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-getnameinfo.conf"
#endif

/*
 * Keep picolibc's resolver off the network: its configuration names a
 * port nothing listens on, so names missing from /etc/hosts fail at once
 */
const char *__picolibc_resolv_conf = TEST_FILE_NAME;

static void
no_dns(void)
{
    FILE *f = fopen(TEST_FILE_NAME, "w");

    check(f != NULL && fputs("nameserver 127.0.0.1:1\n", f) >= 0 && fclose(f) == 0,
          "resolver configuration");
}
#endif

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

/* Whether the hosts database lists the IPv4 address */
static int
in_hosts(in_addr_t addr)
{
    struct hostent *h;
    int             found = 0;

    sethostent(0);
    while (!found && (h = gethostent()) != NULL)
        if (h->h_addrtype == AF_INET && memcmp(h->h_addr_list[0], &addr, 4) == 0)
            found = 1;
    endhostent();
    return found;
}

/* The name of the service on port for proto, or the number */
static void
service_name(int port, const char *proto, char *buf, size_t len)
{
    struct servent *s = getservbyport(htons((in_port_t)port), proto);

    if (s)
        snprintf(buf, len, "%s", s->s_name);
    else
        snprintf(buf, len, "%d", port);
}

/* getnameinfo of an IPv6 address */
static int
name6(const char *text, uint32_t scope, char *host, socklen_t hostlen, int flags)
{
    struct sockaddr_in6 sin6;

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_scope_id = scope;
    check(inet_pton(AF_INET6, text, &sin6.sin6_addr) == 1, "inet_pton");
    return getnameinfo((struct sockaddr *)&sin6, sizeof(sin6), host, hostlen, NULL, 0, flags);
}

int
main(void)
{
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
    char                host[1025], serv[32], name4[1025], want[64];
    int                 ret;
    size_t              len;

#ifdef __PICOLIBC_NETWORK_DNS
    no_dns();
#endif

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = htonl(0x0a010203);
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), serv, sizeof(serv),
                      NI_NUMERICHOST | NI_NUMERICSERV)
              == 0,
          "numeric");
    check(strcmp(host, "10.1.2.3") == 0 && strcmp(serv, "80") == 0, "numeric values");
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, sizeof(serv), 0) == 0,
          "service name");
    check(strcmp(serv, "http") == 0, "http");

    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0) == 0,
          "host name");
    check(strcmp(host, "127.0.0.1") != 0, "named");

    /*
     * An address from TEST-NET-1 has no name, though a DNS server may
     * still supply one, or be unreachable; NI_NAMEREQD never falls back
     * to the number
     */
    sin.sin_addr.s_addr = htonl(0xc0000201);
    ret = getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0,
                      NI_NAMEREQD);
    check(ret == EAI_NONAME || ret == EAI_AGAIN || (ret == 0 && strcmp(host, "192.0.2.1") != 0),
          "NI_NAMEREQD");
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, 4, NULL, 0, NI_NUMERICHOST)
              == EAI_OVERFLOW,
          "EAI_OVERFLOW");

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = in6addr_loopback;
    check(getnameinfo((struct sockaddr *)&sin6, sizeof(sin6), host, sizeof(host), NULL, 0,
                      NI_NUMERICHOST)
                  == 0
              && strcmp(host, "::1") == 0,
          "IPv6");
    /* sa_family_t may have just eight bits; no system assigns 250 */
    sin6.sin6_family = 250;
    check(getnameinfo((struct sockaddr *)&sin6, sizeof(sin6), host, sizeof(host), NULL, 0, 0)
              == EAI_FAMILY,
          "EAI_FAMILY");

#ifdef __PICOLIBC__
    /*
     * The hosts file doesn't list TEST-NET-1, and picolibc's DNS, if
     * built, asks only this test's server, which can't be reached
     */
    if (!in_hosts(htonl(0xc0000201))) {
#ifdef __PICOLIBC_NETWORK_DNS
        check(ret == EAI_AGAIN, "NI_NAMEREQD with no server");
#else
        check(ret == EAI_NONAME, "NI_NAMEREQD without a name");
#endif
    }
#else
    (void)in_hosts;
#endif

    /* Bad flags and lengths, and nothing asked for */
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0, 0x40000000)
              == EAI_BADFLAGS,
          "EAI_BADFLAGS");
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin) - 1, host, sizeof(host), NULL, 0, 0)
              == EAI_FAMILY,
          "short IPv4 length");
    check(getnameinfo((struct sockaddr *)&sin6, sizeof(sin6) - 1, host, sizeof(host), NULL, 0, 0)
              == EAI_FAMILY,
          "short IPv6 length");
#ifndef __GLIBC__
    /* glibc succeeds without doing anything */
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, NULL, 0, 0) == EAI_NONAME,
          "neither host nor service");
#endif

    /* A zero length means the name isn't returned */
    strcpy(host, "unchanged");
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, 0, serv, sizeof(serv), 0) == 0
              && strcmp(host, "unchanged") == 0,
          "zero host length");

    /* The loopback address has a name (checked above) */
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), name4, sizeof(name4), NULL, 0,
                      NI_NAMEREQD)
              == 0,
          "loopback name");

    /* A name that doesn't fit overflows, rather than turning into the number */
    len = strlen(name4);
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, (socklen_t)len, NULL, 0,
                      NI_NAMEREQD)
              == EAI_OVERFLOW,
          "host name overflow");
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, (socklen_t)len + 1, NULL, 0,
                      NI_NAMEREQD)
                  == 0
              && strcmp(host, name4) == 0,
          "host name just fits");
    sin.sin_port = htons(80);
    service_name(80, "tcp", want, sizeof(want));
    len = strlen(want);
    if (len > 2) {
        check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, (socklen_t)len, 0)
                  == EAI_OVERFLOW,
              "service name overflow");
        check(
            getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, (socklen_t)len + 1, 0)
                    == 0
                && strcmp(serv, want) == 0,
            "service name just fits");
    }
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, 2, NI_NUMERICSERV)
              == EAI_OVERFLOW,
          "port overflow");

    /* A port without a service name is returned as its number */
    sin.sin_port = htons(0);
    service_name(0, "tcp", want, sizeof(want));
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, sizeof(serv), 0) == 0
              && strcmp(serv, want) == 0,
          "port without a name");

#ifdef NI_NUMERICSCOPE
    /* NI_NUMERICSCOPE is ignored for IPv4 */
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0,
                      NI_NUMERICHOST | NI_NUMERICSCOPE)
                  == 0
              && strcmp(host, "127.0.0.1") == 0,
          "NI_NUMERICSCOPE with IPv4");
#endif

    /* NI_NOFQDN gives the whole name or the part before a dot */
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, sizeof(host), NULL, 0,
                      NI_NAMEREQD | NI_NOFQDN)
              == 0,
          "NI_NOFQDN");
    len = strlen(host);
    check(strncmp(host, name4, len) == 0 && (name4[len] == '\0' || name4[len] == '.'),
          "NI_NOFQDN name");

    /* NI_DGRAM picks the UDP service, which can differ from the TCP one */
    sin.sin_port = htons(514);
    service_name(514, "udp", want, sizeof(want));
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, sizeof(serv), NI_DGRAM)
                  == 0
              && strcmp(serv, want) == 0,
          "NI_DGRAM");
    service_name(514, "tcp", want, sizeof(want));
    check(getnameinfo((struct sockaddr *)&sin, sizeof(sin), NULL, 0, serv, sizeof(serv), 0) == 0
              && strcmp(serv, want) == 0,
          "stream by default");

    /* Mapped and compatible addresses have the name of their IPv4 address */
    check(name6("::ffff:127.0.0.1", 0, host, sizeof(host), NI_NAMEREQD) == 0
              && strcmp(host, name4) == 0,
          "IPv4-mapped");
    check(name6("::127.0.0.1", 0, host, sizeof(host), NI_NAMEREQD) == 0 && strcmp(host, name4) == 0,
          "IPv4-compatible");
    check(name6("::ffff:127.0.0.1", 0, host, sizeof(host), NI_NUMERICHOST) == 0
              && strcmp(host, "::ffff:127.0.0.1") == 0,
          "numeric IPv4-mapped");

    /* The unspecified address is never looked up */
    check(name6("::", 0, host, sizeof(host), NI_NAMEREQD) == EAI_NONAME, "unspecified named");
    check(name6("::", 0, host, sizeof(host), 0) == 0 && strcmp(host, "::") == 0, "unspecified");

    /* The scope follows the address, as a number with NI_NUMERICSCOPE */
    {
        struct if_nameindex *list = if_nameindex();
        unsigned             index;
        char                 ifname[IF_NAMESIZE], number[16];

        check(list != NULL && list[0].if_index != 0, "an interface");
        index = list[0].if_index;
        snprintf(ifname, sizeof(ifname), "%s", list[0].if_name);
        if_freenameindex(list);
        snprintf(number, sizeof(number), "%u", index);

        check(name6("fe80::1", 0, host, sizeof(host), NI_NUMERICHOST) == 0
                  && strcmp(host, "fe80::1") == 0,
              "no scope");
        check(name6("fe80::1", index, host, sizeof(host), NI_NUMERICHOST) == 0
                  && strncmp(host, "fe80::1%", 8) == 0
                  && (strcmp(host + 8, ifname) == 0 || strcmp(host + 8, number) == 0),
              "link-local scope");
#ifdef __PICOLIBC__
        check(strcmp(host + 8, ifname) == 0, "link-local scope name");
#endif
#ifdef NI_NUMERICSCOPE
        /* glibc doesn't define NI_NUMERICSCOPE */
        check(name6("fe80::1", index, host, sizeof(host), NI_NUMERICHOST | NI_NUMERICSCOPE) == 0
                  && strncmp(host, "fe80::1%", 8) == 0 && strcmp(host + 8, number) == 0,
              "NI_NUMERICSCOPE");
#endif
        check(name6("2001:db8::1", index, host, sizeof(host), NI_NUMERICHOST) == 0
                  && strncmp(host, "2001:db8::1%", 12) == 0,
              "global scope");
    }
    (void)make_socket;
    (void)tcp_pair;
#ifdef __PICOLIBC_NETWORK_DNS
    (void)unlink(TEST_FILE_NAME);
#endif
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
