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

/*
 * gethostbyaddr (removed from POSIX in 2008, still everywhere):
 * getnameinfo's name for an IPv4 or IPv6 address
 */
struct hostent *
gethostbyaddr(const void *addr, socklen_t len, int type)
{
    static struct hostent   h;
    static char             hname[NI_MAXHOST];
    static unsigned char    haddr[16];
    static char            *addr_list[2];
    static char            *aliases[1];
    struct sockaddr_storage ss;
    socklen_t               sslen;
    int                     err;

    memset(&ss, 0, sizeof(ss));
    if (type == AF_INET && len == sizeof(struct in_addr)) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;

        sin->sin_family = AF_INET;
        memcpy(&sin->sin_addr, addr, len);
        sslen = sizeof(*sin);
    } else if (type == AF_INET6 && len == sizeof(struct in6_addr)) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;

        sin6->sin6_family = AF_INET6;
        memcpy(&sin6->sin6_addr, addr, len);
        sslen = sizeof(*sin6);
    } else {
        h_errno = NO_RECOVERY;
        return NULL;
    }
    err = getnameinfo((struct sockaddr *)&ss, sslen, hname, sizeof(hname), NULL, 0, NI_NAMEREQD);
    if (err != 0) {
        h_errno = err == EAI_AGAIN ? TRY_AGAIN : HOST_NOT_FOUND;
        return NULL;
    }
    memcpy(haddr, addr, len);
    addr_list[0] = (char *)haddr;
    addr_list[1] = NULL;
    aliases[0] = NULL;
    h.h_name = hname;
    h.h_aliases = aliases;
    h.h_addrtype = type;
    h.h_length = (int)len;
    h.h_addr_list = addr_list;
    return &h;
}
