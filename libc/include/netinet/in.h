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

#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#include <sys/cdefs.h>
#include <sys/_types.h>
/* The types and values come from the system's network stack */
#include <netinet/_in.h>

_BEGIN_STD_C

#ifndef _UINT8_T_DECLARED
typedef __uint8_t uint8_t;
#define _UINT8_T_DECLARED
#endif

#ifndef _UINT32_T_DECLARED
typedef __uint32_t uint32_t;
#define _UINT32_T_DECLARED
#endif

#define INADDR_ANY       ((in_addr_t)0x00000000)
#define INADDR_BROADCAST ((in_addr_t)0xffffffff)
#define INADDR_LOOPBACK  ((in_addr_t)0x7f000001)

#define INET_ADDRSTRLEN  16
#define INET6_ADDRSTRLEN 46

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#define IN6ADDR_ANY_INIT                                       \
    {                                                          \
        { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } \
    }
#define IN6ADDR_LOOPBACK_INIT                                  \
    {                                                          \
        { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } \
    }

#define __IN6_ADDR32(a, i)                                     \
    ((__uint32_t)((const __uint8_t *)(a))[4 * (i)] << 24       \
     | (__uint32_t)((const __uint8_t *)(a))[4 * (i) + 1] << 16 \
     | (__uint32_t)((const __uint8_t *)(a))[4 * (i) + 2] << 8  \
     | (__uint32_t)((const __uint8_t *)(a))[4 * (i) + 3])

#define IN6_IS_ADDR_UNSPECIFIED(a)                                                \
    (__IN6_ADDR32((a)->s6_addr, 0) == 0 && __IN6_ADDR32((a)->s6_addr, 1) == 0     \
     && __IN6_ADDR32((a)->s6_addr, 2) == 0 && __IN6_ADDR32((a)->s6_addr, 3) == 0)
#define IN6_IS_ADDR_LOOPBACK(a)                                                   \
    (__IN6_ADDR32((a)->s6_addr, 0) == 0 && __IN6_ADDR32((a)->s6_addr, 1) == 0     \
     && __IN6_ADDR32((a)->s6_addr, 2) == 0 && __IN6_ADDR32((a)->s6_addr, 3) == 1)
#define IN6_IS_ADDR_MULTICAST(a) ((a)->s6_addr[0] == 0xff)
#define IN6_IS_ADDR_LINKLOCAL(a) ((a)->s6_addr[0] == 0xfe && ((a)->s6_addr[1] & 0xc0) == 0x80)
#define IN6_IS_ADDR_SITELOCAL(a) ((a)->s6_addr[0] == 0xfe && ((a)->s6_addr[1] & 0xc0) == 0xc0)
#define IN6_IS_ADDR_V4MAPPED(a)                                               \
    (__IN6_ADDR32((a)->s6_addr, 0) == 0 && __IN6_ADDR32((a)->s6_addr, 1) == 0 \
     && __IN6_ADDR32((a)->s6_addr, 2) == 0xffff)
#define IN6_IS_ADDR_V4COMPAT(a)                                                  \
    (__IN6_ADDR32((a)->s6_addr, 0) == 0 && __IN6_ADDR32((a)->s6_addr, 1) == 0    \
     && __IN6_ADDR32((a)->s6_addr, 2) == 0 && __IN6_ADDR32((a)->s6_addr, 3) > 1)
#define IN6_IS_ADDR_MC_NODELOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((a)->s6_addr[1] & 0xf) == 0x1)
#define IN6_IS_ADDR_MC_LINKLOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((a)->s6_addr[1] & 0xf) == 0x2)
#define IN6_IS_ADDR_MC_SITELOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((a)->s6_addr[1] & 0xf) == 0x5)
#define IN6_IS_ADDR_MC_ORGLOCAL(a)  (IN6_IS_ADDR_MULTICAST(a) && ((a)->s6_addr[1] & 0xf) == 0x8)
#define IN6_IS_ADDR_MC_GLOBAL(a)    (IN6_IS_ADDR_MULTICAST(a) && ((a)->s6_addr[1] & 0xf) == 0xe)

_END_STD_C

/* POSIX makes the byte order functions visible here too */
#include <arpa/inet.h>

#endif /* _NETINET_IN_H_ */
