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
#include <string.h>

/* Four decimal numbers 0 to 255 of one to three digits, separated by dots */
static int
__inet_pton4(const char *src, unsigned char *dst)
{
    unsigned char addr[4];
    int           octets = 0;

    while (octets < 4) {
        unsigned value = 0;
        int      digits = 0;

        while (*src >= '0' && *src <= '9') {
            if (++digits > 3)
                return 0;
            value = value * 10 + (unsigned)(*src++ - '0');
            if (value > 255)
                return 0;
        }
        if (!digits)
            return 0;
        addr[octets++] = (unsigned char)value;
        if (octets < 4 && *src++ != '.')
            return 0;
    }
    if (*src)
        return 0;
    memcpy(dst, addr, 4);
    return 1;
}

static int
__hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* Eight groups of up to four hex digits, with :: and a dotted IPv4 tail */
static int
__inet_pton6(const char *src, unsigned char *dst)
{
    unsigned char addr[16];
    int           n = 0, gap = -1;

    if (src[0] == ':') {
        if (src[1] != ':')
            return 0;
        src++;
    }
    while (*src) {
        unsigned    value = 0;
        int         digits = 0, h;
        const char *dot, *colon;

        if (*src == ':') {
            if (gap >= 0)
                return 0;
            gap = n;
            src++;
            if (!*src)
                break;
            continue;
        }
        /* A dotted IPv4 address fills the last 32 bits */
        dot = strchr(src, '.');
        colon = strchr(src, ':');
        if (n <= 12 && dot && (!colon || dot < colon)) {
            if (!__inet_pton4(src, addr + n))
                return 0;
            n += 4;
            break;
        }
        while ((h = __hexval(*src)) >= 0) {
            if (++digits > 4)
                return 0;
            value = (value << 4) | (unsigned)h;
            src++;
        }
        if (!digits || n == 16)
            return 0;
        addr[n++] = (unsigned char)(value >> 8);
        addr[n++] = (unsigned char)value;
        if (*src == ':') {
            src++;
            if (!*src)
                return 0;
        } else if (*src) {
            return 0;
        }
    }
    if (gap >= 0) {
        if (n == 16)
            return 0;
        memmove(addr + 16 - (n - gap), addr + gap, (size_t)(n - gap));
        memset(addr + gap, 0, (size_t)(16 - n));
    } else if (n != 16) {
        return 0;
    }
    memcpy(dst, addr, 16);
    return 1;
}

int
inet_pton(int af, const char * __restrict src, void * __restrict dst)
{
    switch (af) {
    case AF_INET:
        return __inet_pton4(src, dst);
    case AF_INET6:
        return __inet_pton6(src, dst);
    default:
        errno = EAFNOSUPPORT;
        return -1;
    }
}
