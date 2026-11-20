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

#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char *
__inet_ntop4(const unsigned char *src, char *dst, socklen_t size)
{
    char buf[INET_ADDRSTRLEN];

    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
    if (strlen(buf) >= size) {
        errno = ENOSPC;
        return NULL;
    }
    return strcpy(dst, buf);
}

/* RFC 5952 text: the longest run of two or more zero groups becomes :: */
static const char *
__inet_ntop6(const unsigned char *src, char *dst, socklen_t size)
{
    char     buf[INET6_ADDRSTRLEN], *p = buf;
    unsigned words[8];
    int      best = -1, best_len = 0, cur = -1, i;

    for (i = 0; i < 8; i++)
        words[i] = (unsigned)src[2 * i] << 8 | src[2 * i + 1];
    for (i = 0; i <= 8; i++) {
        if (i < 8 && words[i] == 0) {
            if (cur < 0)
                cur = i;
        } else if (cur >= 0) {
            if (i - cur > best_len) {
                best = cur;
                best_len = i - cur;
            }
            cur = -1;
        }
    }
    if (best_len < 2)
        best = -1;
    for (i = 0; i < 8; i++) {
        if (i == best) {
            *p++ = ':';
            if (i == 0)
                *p++ = ':';
            i += best_len - 1;
            continue;
        }
        /* IPv4-compatible and IPv4-mapped addresses end in dotted form */
        if (i == 6 && best == 0 && (best_len == 6 || (best_len == 5 && words[5] == 0xffff))) {
            if (!__inet_ntop4(src + 12, p, (socklen_t)(sizeof(buf) - (size_t)(p - buf))))
                return NULL;
            p += strlen(p);
            break;
        }
        p += sprintf(p, "%x", words[i]);
        if (i < 7)
            *p++ = ':';
    }
    *p = '\0';
    if (strlen(buf) >= size) {
        errno = ENOSPC;
        return NULL;
    }
    return strcpy(dst, buf);
}

const char *
inet_ntop(int af, const void * __restrict src, char * __restrict dst, socklen_t size)
{
    switch (af) {
    case AF_INET:
        return __inet_ntop4(src, dst, size);
    case AF_INET6:
        return __inet_ntop6(src, dst, size);
    default:
        errno = EAFNOSUPPORT;
        return NULL;
    }
}
