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

#include "local-netdb.h"
#include <stdlib.h>
#include <string.h>

#define __GAI_MAX_ADDRS 16
#define __GAI_FLAGS                                                                    \
    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_V4MAPPED | AI_ALL | AI_ADDRCONFIG \
     | AI_NUMERICSERV)

static const struct {
    int         socktype;
    int         protocol;
    const char *proto;
} __gai_types[] = {
    { SOCK_STREAM, IPPROTO_TCP, "tcp" },
    { SOCK_DGRAM,  IPPROTO_UDP, "udp" },
    { SOCK_RAW,    0,           NULL  },
};

#define __GAI_NTYPES (sizeof(__gai_types) / sizeof(__gai_types[0]))

/* Parses a decimal port number, returning -1 for anything else */
static int
__gai_numeric_port(const char *service)
{
    char *end;
    long  port;

    if (!*service)
        return -1;
    port = strtol(service, &end, 10);
    if (*end || port < 0 || port > 65535)
        return -1;
    return (int)port;
}

static struct addrinfo *
__gai_entry(const struct __netdb_addr *a, int type, int port, const char *canon)
{
    size_t           canonlen = canon ? strlen(canon) + 1 : 0;
    struct addrinfo *ai = calloc(1, sizeof(*ai) + sizeof(struct sockaddr_in6) + canonlen);

    if (!ai)
        return NULL;
    ai->ai_family = a->family;
    ai->ai_socktype = __gai_types[type].socktype;
    ai->ai_protocol = __gai_types[type].protocol;
    ai->ai_addr = (struct sockaddr *)(ai + 1);
    if (a->family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;

        sin->sin_family = AF_INET;
        sin->sin_port = htons((uint16_t)port);
        memcpy(&sin->sin_addr, a->addr, 4);
        ai->ai_addrlen = sizeof(*sin);
    } else {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;

        sin6->sin6_family = AF_INET6;
        sin6->sin6_port = htons((uint16_t)port);
        memcpy(&sin6->sin6_addr, a->addr, 16);
        ai->ai_addrlen = sizeof(*sin6);
    }
    if (canon) {
        ai->ai_canonname = (char *)ai->ai_addr + sizeof(struct sockaddr_in6);
        memcpy(ai->ai_canonname, canon, canonlen);
    }
    return ai;
}

/* Turns an IPv4 address into an IPv4-mapped IPv6 one */
static void
__gai_map(struct __netdb_addr *a)
{
    memmove(a->addr + 12, a->addr, 4);
    memset(a->addr, 0, 10);
    a->addr[10] = 0xff;
    a->addr[11] = 0xff;
    a->family = AF_INET6;
}

int
getaddrinfo(const char * __restrict node, const char * __restrict service,
            const struct addrinfo * __restrict hints, struct addrinfo ** __restrict res)
{
    struct __netdb_addr addrs[__GAI_MAX_ADDRS];
    char                canon_buf[256];
    const char         *canon = NULL;
    struct addrinfo    *head = NULL, **tail = &head;
    int                 flags = 0, family = AF_UNSPEC, socktype = 0, protocol = 0;
    int                 ports[__GAI_NTYPES];
    int                 naddrs = 0, i, t, any_type = 0;

    if (hints) {
        flags = hints->ai_flags;
        family = hints->ai_family;
        socktype = hints->ai_socktype;
        protocol = hints->ai_protocol;
    }
    if (flags & ~__GAI_FLAGS)
        return EAI_BADFLAGS;
    if ((flags & AI_CANONNAME) && !node)
        return EAI_BADFLAGS;
    if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6)
        return EAI_FAMILY;
    if (socktype != 0 && socktype != SOCK_STREAM && socktype != SOCK_DGRAM && socktype != SOCK_RAW)
        return EAI_SOCKTYPE;
    if (!node && !service)
        return EAI_NONAME;

    /* The port for each socket type, or -1 where the service has none */
    for (t = 0; t < (int)__GAI_NTYPES; t++) {
        ports[t] = -1;
        if (socktype && socktype != __gai_types[t].socktype)
            continue;
        if (protocol && __gai_types[t].protocol && protocol != __gai_types[t].protocol)
            continue;
        if (!service) {
            ports[t] = 0;
        } else if (__gai_types[t].proto) {
            ports[t] = __gai_numeric_port(service);
            if (ports[t] < 0 && !(flags & AI_NUMERICSERV))
                ports[t] = __netdb_service_port(service, __gai_types[t].proto);
        }
        if (ports[t] >= 0)
            any_type = 1;
    }
    if (!any_type) {
        if (service && (flags & AI_NUMERICSERV) && __gai_numeric_port(service) < 0)
            return EAI_NONAME;
        return EAI_SERVICE;
    }

    if (!node) {
        /* The wildcard address for servers, otherwise the loopback address */
        bool passive = (flags & AI_PASSIVE) != 0;

        if (family != AF_INET) {
            addrs[naddrs].family = AF_INET6;
            memcpy(addrs[naddrs].addr, passive ? &in6addr_any : &in6addr_loopback, 16);
            naddrs++;
        }
        if (family != AF_INET6) {
            in_addr_t a = htonl(passive ? INADDR_ANY : INADDR_LOOPBACK);

            addrs[naddrs].family = AF_INET;
            memcpy(addrs[naddrs].addr, &a, 4);
            naddrs++;
        }
    } else if (inet_pton(AF_INET, node, addrs[0].addr) == 1) {
        addrs[0].family = AF_INET;
        naddrs = 1;
        if (family == AF_INET6) {
            if (!(flags & AI_V4MAPPED))
                return EAI_NONAME;
            __gai_map(&addrs[0]);
        }
    } else if (inet_pton(AF_INET6, node, addrs[0].addr) == 1) {
        if (family == AF_INET)
            return EAI_NONAME;
        addrs[0].family = AF_INET6;
        naddrs = 1;
    } else if (flags & AI_NUMERICHOST) {
        return EAI_NONAME;
    } else {
        struct __netdb_addr found[__GAI_MAX_ADDRS];
        int                 nfound, have6 = 0;

        canon_buf[0] = '\0';
        nfound = __netdb_host_addrs(node, found, __GAI_MAX_ADDRS, canon_buf, sizeof(canon_buf));
        for (i = 0; i < nfound; i++)
            if (found[i].family == AF_INET6)
                have6 = 1;
        for (i = 0; i < nfound; i++) {
            if (family == AF_UNSPEC || found[i].family == family) {
                addrs[naddrs++] = found[i];
            } else if (family == AF_INET6 && found[i].family == AF_INET && (flags & AI_V4MAPPED)
                       && (!have6 || (flags & AI_ALL))) {
                addrs[naddrs] = found[i];
                __gai_map(&addrs[naddrs++]);
            }
        }
        if (!naddrs)
            return EAI_NONAME;
        if (flags & AI_CANONNAME)
            canon = canon_buf[0] ? canon_buf : node;
    }
    if ((flags & AI_CANONNAME) && !canon)
        canon = node;

    for (i = 0; i < naddrs; i++) {
        for (t = 0; t < (int)__GAI_NTYPES; t++) {
            if (ports[t] < 0)
                continue;
            *tail = __gai_entry(&addrs[i], t, ports[t], canon);
            if (!*tail) {
                freeaddrinfo(head);
                return EAI_MEMORY;
            }
            /* Only the first entry has the canonical name */
            canon = NULL;
            tail = &(*tail)->ai_next;
        }
    }
    *res = head;
    return 0;
}
