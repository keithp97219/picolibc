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
#include <string.h>

#define __GNI_FLAGS                                                                          \
    (NI_NUMERICHOST | NI_NUMERICSERV | NI_NOFQDN | NI_NAMEREQD | NI_DGRAM | NI_NUMERICSCOPE)

int
getnameinfo(const struct sockaddr * __restrict sa, socklen_t salen, char * __restrict host,
            socklen_t hostlen, char * __restrict serv, socklen_t servlen, int flags)
{
    const void *addr, *lookup_addr;
    int         family, lookup_family, port;
    char        buf[INET6_ADDRSTRLEN];

    if (flags & ~__GNI_FLAGS)
        return EAI_BADFLAGS;
    if (!sa)
        return EAI_FAMILY;
    family = sa->sa_family;
    if (family == AF_INET && salen >= sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;

        addr = &sin->sin_addr;
        port = ntohs(sin->sin_port);
    } else if (family == AF_INET6 && salen >= sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;

        addr = &sin6->sin6_addr;
        port = ntohs(sin6->sin6_port);
    } else {
        return EAI_FAMILY;
    }
    if ((!host || !hostlen) && (!serv || !servlen))
        return EAI_NONAME;

    /*
     * Mapped and compatible addresses are named for their IPv4 address,
     * and the unspecified address has no name
     */
    lookup_family = family;
    lookup_addr = addr;
    if (family == AF_INET6) {
        const struct in6_addr *a6 = addr;

        if (IN6_IS_ADDR_V4MAPPED(a6) || IN6_IS_ADDR_V4COMPAT(a6)) {
            lookup_family = AF_INET;
            lookup_addr = &a6->s6_addr[12];
        } else if (IN6_IS_ADDR_UNSPECIFIED(a6)) {
            lookup_family = AF_UNSPEC;
        }
    }

    if (host && hostlen) {
        bool named = false;

        if (!(flags & NI_NUMERICHOST) && lookup_family != AF_UNSPEC) {
            named = __netdb_host_name(lookup_family, lookup_addr, host, hostlen);
            if (named && (flags & NI_NOFQDN)) {
                char *dot = strchr(host, '.');

                if (dot)
                    *dot = '\0';
            }
        }
        if (!named) {
            if (flags & NI_NAMEREQD)
                return EAI_NONAME;
            if (!inet_ntop(family, addr, buf, sizeof(buf)))
                return EAI_SYSTEM;
            if (strlen(buf) >= hostlen)
                return EAI_OVERFLOW;
            strcpy(host, buf);
        }
    }

    if (serv && servlen) {
        bool named = false;

        if (!(flags & NI_NUMERICSERV))
            named = __netdb_service_name(port, (flags & NI_DGRAM) ? "udp" : "tcp", serv, servlen);
        if (!named) {
            int len = snprintf(buf, sizeof(buf), "%d", port);

            if (len < 0 || (size_t)len >= servlen)
                return EAI_OVERFLOW;
            strcpy(serv, buf);
        }
    }
    return 0;
}
