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
#if defined(__PICOLIBC_NETWORK_DNS) && _POSIX_THREADS > 0

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

/*
 * picolibc's DNS resolver, against a server in this test: the
 * configuration file comes from __picolibc_resolv_conf, and the server
 * listens on a port the kernel picks
 */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-dns.conf"
#endif

const char *__picolibc_resolv_conf = TEST_FILE_NAME;

#define HOST_MAX 1025

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int                   server, spoofer;
static volatile sig_atomic_t stop;
static volatile int          local_queries;

/* Appends 'name' to a message, uncompressed */
static size_t
put_name(unsigned char *p, const char *name)
{
    size_t n = 0;

    while (*name) {
        const char *dot = strchr(name, '.');
        size_t      l = dot ? (size_t)(dot - name) : strlen(name);

        p[n++] = (unsigned char)l;
        memcpy(p + n, name, l);
        n += l;
        name += l + (dot != NULL);
    }
    p[n++] = 0;
    return n;
}

/* Appends a record of 'type' owned by the question's name, or by 'owner' */
static size_t
put_record(unsigned char *p, const char *owner, int type, const void *data, size_t len)
{
    size_t n;

    if (owner) {
        n = put_name(p, owner);
    } else {
        p[0] = 0xc0;
        p[1] = 12;
        n = 2;
    }
    p[n++] = 0;
    p[n++] = (unsigned char)type;
    p[n++] = 0;
    p[n++] = 1; /* IN */
    memset(p + n, 0, 3);
    p[n + 3] = 60; /* TTL */
    n += 4;
    p[n++] = (unsigned char)(len >> 8);
    p[n++] = (unsigned char)len;
    memcpy(p + n, data, len);
    return n + len;
}

static size_t
put_address(unsigned char *p, const char *owner, int type, const char *text)
{
    unsigned char a[16];

    (void)inet_pton(type == 1 ? AF_INET : AF_INET6, text, a);
    return put_record(p, owner, type, a, type == 1 ? 4 : 16);
}

static size_t
put_target(unsigned char *p, const char *owner, int type, const char *target)
{
    unsigned char t[256];

    return put_record(p, owner, type, t, put_name(t, target));
}

/* Answers one query; 'r' already holds the header and question */
static size_t
answer(const char *name, int type, unsigned char *r, size_t n, int *rcode, int *count, int *wrong)
{
    const char *v4 = NULL, *v6 = NULL, *ptr = NULL;

    if (strcasecmp(name, "host.example.test") == 0) {
        v4 = "192.0.2.10";
        v6 = "2001:db8::10";
    } else if (strcasecmp(name, "alias.example.test") == 0) {
        /* A CNAME, then the records of its target */
        n += put_target(r + n, NULL, 5, "host.example.test");
        if (type == 1)
            n += put_address(r + n, "host.example.test", 1, "192.0.2.10");
        else
            n += put_address(r + n, "host.example.test", 28, "2001:db8::10");
        *count = 2;
        return n;
    } else if (strcasecmp(name, "short.example.test") == 0) {
        v4 = "192.0.2.11";
    } else if (strcasecmp(name, "badid.example.test") == 0) {
        v4 = "192.0.2.12";
        *wrong = 1;
    } else if (strcasecmp(name, "spoof.example.test") == 0) {
        v4 = "192.0.2.13";
        *wrong = 2;
    } else if (strcasecmp(name, "v6only.example.test") == 0) {
        v6 = "2001:db8::12";
    } else if (strcasecmp(name, "servfail.example.test") == 0
               || strcasecmp(name, "98.2.0.192.in-addr.arpa") == 0) {
        *rcode = 2;
    } else if (strcasecmp(name, "refused.example.test") == 0) {
        *rcode = 5;
    } else if (strcasecmp(name, "silent.example.test") == 0) {
        *rcode = -1;
    } else if (strcasecmp(name, "10.2.0.192.in-addr.arpa") == 0
               || strcasecmp(name,
                             "0.1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2."
                             "ip6.arpa")
                   == 0) {
        ptr = "host.example.test";
    } else {
        *rcode = 3;
    }
    if (type == 1 && v4) {
        n += put_address(r + n, NULL, 1, v4);
        *count = 1;
    } else if (type == 28 && v6) {
        n += put_address(r + n, NULL, 28, v6);
        *count = 1;
    } else if (type == 12 && ptr) {
        n += put_target(r + n, NULL, 12, ptr);
        *count = 1;
    }
    return n;
}

static void *
serve(void *arg)
{
    (void)arg;
    while (!stop) {
        struct pollfd           pfd = { .fd = server, .events = POLLIN };
        struct sockaddr_storage from;
        socklen_t               fromlen = sizeof(from);
        unsigned char           q[512], r[1024];
        char                    name[256];
        ssize_t                 len;
        size_t                  p = 12, o = 0, n;
        int                     type, rcode = 0, count = 0, wrong = 0;

        if (poll(&pfd, 1, 100) <= 0)
            continue;
        len = recvfrom(server, q, sizeof(q), 0, (struct sockaddr *)&from, &fromlen);
        if (len < 17)
            continue;
        while (p < (size_t)len && q[p]) {
            if (o)
                name[o++] = '.';
            memcpy(name + o, q + p + 1, q[p]);
            o += q[p];
            p += q[p] + 1;
        }
        name[o] = '\0';
        type = (q[p + 1] << 8) | q[p + 2];
        n = p + 5;
        if (strstr(name, "invalid") || strstr(name, "localhost"))
            local_queries++;

        memcpy(r, q, n);
        n = answer(name, type, r, n, &rcode, &count, &wrong);
        if (rcode < 0)
            continue;
        r[2] = 0x81;
        r[3] = (unsigned char)(0x80 | rcode);
        r[6] = 0;
        r[7] = (unsigned char)count;
        if (wrong == 1) {
            /* A reply to some other query comes first */
            r[1] ^= 1;
            (void)sendto(server, r, n, 0, (struct sockaddr *)&from, fromlen);
            r[1] ^= 1;
        } else if (wrong == 2) {
            /* So does one from another address */
            (void)sendto(spoofer, r, n, 0, (struct sockaddr *)&from, fromlen);
        }
        (void)sendto(server, r, n, 0, (struct sockaddr *)&from, fromlen);
    }
    return NULL;
}

/* The first address getaddrinfo finds for 'name' in 'family', as text, or an EAI_ code */
static int
lookup(const char *name, int family, int flags, char *text, char *canon)
{
    struct addrinfo hints, *res;
    int             ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = flags;
    ret = getaddrinfo(name, NULL, &hints, &res);
    if (ret)
        return ret;
    if (res->ai_family == AF_INET)
        inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, text, INET6_ADDRSTRLEN);
    else
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, text,
                  INET6_ADDRSTRLEN);
    if (canon)
        strcpy(canon, res->ai_canonname ? res->ai_canonname : "");
    freeaddrinfo(res);
    return 0;
}

static int
reverse(const char *text, int flags, char *host)
{
    struct sockaddr_in6 sin6;
    struct sockaddr_in  sin;

    if (strchr(text, ':')) {
        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, text, &sin6.sin6_addr);
        return getnameinfo((struct sockaddr *)&sin6, sizeof(sin6), host, HOST_MAX, NULL, 0, flags);
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    inet_pton(AF_INET, text, &sin.sin_addr);
    return getnameinfo((struct sockaddr *)&sin, sizeof(sin), host, HOST_MAX, NULL, 0, flags);
}

int
main(void)
{
    struct sockaddr_in sin;
    socklen_t          len = sizeof(sin);
    pthread_t          thread;
    char               text[INET6_ADDRSTRLEN], canon[HOST_MAX], host[HOST_MAX];
    FILE              *f;

    server = socket(AF_INET, SOCK_DGRAM, 0);
    spoofer = socket(AF_INET, SOCK_DGRAM, 0);
    check(server >= 0 && spoofer >= 0, "socket");
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(bind(server, (struct sockaddr *)&sin, sizeof(sin)) == 0, "bind");
    check(getsockname(server, (struct sockaddr *)&sin, &len) == 0, "getsockname");

    /* A server that refuses at once, then this test's */
    f = fopen(TEST_FILE_NAME, "w");
    check(f != NULL, "write the configuration");
    fprintf(f,
            "# test configuration\nsearch example.test\nnameserver 127.0.0.1:1\n"
            "nameserver 127.0.0.1:%d\noptions timeout:1 attempts:1\n",
            ntohs(sin.sin_port));
    check(fclose(f) == 0, "close the configuration");
    check(pthread_create(&thread, NULL, serve, NULL) == 0, "pthread_create");

    /* Addresses of each family */
    check(lookup("host.example.test", AF_INET, 0, text, NULL) == 0
              && strcmp(text, "192.0.2.10") == 0,
          "A record");
    check(lookup("host.example.test", AF_INET6, 0, text, NULL) == 0
              && strcmp(text, "2001:db8::10") == 0,
          "AAAA record");
    check(lookup("v6only.example.test", AF_UNSPEC, 0, text, NULL) == 0
              && strcmp(text, "2001:db8::12") == 0,
          "AAAA without A");
    check(lookup("v6only.example.test", AF_INET, 0, text, NULL) == EAI_NONAME, "no A record");

    /* CNAME chains, the search list and absolute names */
    check(lookup("alias.example.test", AF_INET, AI_CANONNAME, text, canon) == 0
              && strcmp(text, "192.0.2.10") == 0 && strcmp(canon, "host.example.test") == 0,
          "CNAME");
    check(lookup("short", AF_INET, 0, text, NULL) == 0 && strcmp(text, "192.0.2.11") == 0,
          "search list");
    check(lookup("short.", AF_INET, 0, text, NULL) == EAI_NONAME, "absolute name");

    /* Replies that don't belong to the query are ignored */
    check(lookup("badid.example.test", AF_INET, 0, text, NULL) == 0
              && strcmp(text, "192.0.2.12") == 0,
          "wrong id ignored");
    check(lookup("spoof.example.test", AF_INET, 0, text, NULL) == 0
              && strcmp(text, "192.0.2.13") == 0,
          "other address ignored");

    /* Errors */
    check(lookup("nxdomain.example.test", AF_INET, 0, text, NULL) == EAI_NONAME, "NXDOMAIN");
    check(lookup("servfail.example.test", AF_INET, 0, text, NULL) == EAI_AGAIN, "SERVFAIL");
    check(lookup("refused.example.test", AF_INET, 0, text, NULL) == EAI_FAIL, "REFUSED");
    check(lookup("silent.example.test", AF_INET, 0, text, NULL) == EAI_AGAIN, "timeout");

    /* RFC 6761 names never reach the server */
    check(lookup("no-such-host.invalid", AF_UNSPEC, 0, text, NULL) == EAI_NONAME, ".invalid");
    check(lookup("test.localhost", AF_INET, 0, text, NULL) == 0 && strcmp(text, "127.0.0.1") == 0,
          ".localhost");
    check(local_queries == 0, "no queries for .invalid or .localhost");

    /* Reverse lookups */
    check(reverse("192.0.2.10", NI_NAMEREQD, host) == 0 && strcmp(host, "host.example.test") == 0,
          "PTR");
    check(reverse("192.0.2.10", NI_NAMEREQD | NI_NOFQDN, host) == 0 && strcmp(host, "host") == 0,
          "PTR with NI_NOFQDN");
    check(reverse("2001:db8::10", NI_NAMEREQD, host) == 0 && strcmp(host, "host.example.test") == 0,
          "IPv6 PTR");
    check(reverse("192.0.2.99", NI_NAMEREQD, host) == EAI_NONAME, "no PTR");
    check(reverse("192.0.2.99", 0, host) == 0 && strcmp(host, "192.0.2.99") == 0,
          "numeric fallback");
    check(reverse("192.0.2.98", NI_NAMEREQD, host) == EAI_AGAIN, "PTR SERVFAIL");
    check(reverse("192.0.2.98", 0, host) == 0 && strcmp(host, "192.0.2.98") == 0,
          "numeric fallback after SERVFAIL");

    stop = 1;
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(close(server) == 0 && close(spoofer) == 0 && unlink(TEST_FILE_NAME) == 0, "clean up");
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
