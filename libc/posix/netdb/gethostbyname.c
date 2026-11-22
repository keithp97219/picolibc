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
 * gethostbyname (removed from POSIX in 2008, still everywhere):
 * getaddrinfo's IPv4 answers in the traditional static hostent
 */

#define HOSTENT_ADDRS 16

int h_errno;

struct hostent *
gethostbyname(const char *name)
{
    static struct hostent h;
    static char           hname[NI_MAXHOST];
    static struct in_addr addrs[HOSTENT_ADDRS];
    static char          *addr_list[HOSTENT_ADDRS + 1];
    static char          *aliases[1];
    struct addrinfo       hints, *res, *ai;
    int                   n = 0, err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    err = getaddrinfo(name, NULL, &hints, &res);
    if (err != 0) {
        h_errno = err == EAI_AGAIN ? TRY_AGAIN : err == EAI_FAIL ? NO_RECOVERY : HOST_NOT_FOUND;
        return NULL;
    }
    for (ai = res; ai && n < HOSTENT_ADDRS; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET)
            continue;
        addrs[n] = ((struct sockaddr_in *)ai->ai_addr)->sin_addr;
        addr_list[n] = (char *)&addrs[n];
        n++;
    }
    addr_list[n] = NULL;
    strncpy(hname, res->ai_canonname ? res->ai_canonname : name, sizeof(hname) - 1);
    hname[sizeof(hname) - 1] = '\0';
    freeaddrinfo(res);
    if (n == 0) {
        h_errno = NO_DATA;
        return NULL;
    }
    aliases[0] = NULL;
    h.h_name = hname;
    h.h_aliases = aliases;
    h.h_addrtype = AF_INET;
    h.h_length = sizeof(struct in_addr);
    h.h_addr_list = addr_list;
    return &h;
}
