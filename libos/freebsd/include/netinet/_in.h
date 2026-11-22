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

/* The FreeBSD Internet protocol ABI, for picolibc's <netinet/in.h> */

#ifndef _NETINET__IN_H_
#define _NETINET__IN_H_

#include <sys/_socket.h>
#include <sys/_types.h>

#ifndef _IN_PORT_T_DECLARED
#define _IN_PORT_T_DECLARED
typedef __uint16_t in_port_t;
#endif

#ifndef _IN_ADDR_T_DECLARED
#define _IN_ADDR_T_DECLARED
typedef __uint32_t in_addr_t;
#endif

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    unsigned char  sin_len;
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct in6_addr {
    union {
        __uint8_t  __s6_addr[16];
        __uint16_t __s6_addr16[8];
        __uint32_t __s6_addr32[4];
    } __in6_union;
};
#define s6_addr __in6_union.__s6_addr

/*
 * As on other BSD systems, says that sockaddr_in6 has sin6_len; not a
 * name POSIX gives <netinet/in.h>
 */
#if __BSD_VISIBLE
#define SIN6_LEN
#endif

struct sockaddr_in6 {
    unsigned char   sin6_len;
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    __uint32_t      sin6_flowinfo;
    struct in6_addr sin6_addr;
    __uint32_t      sin6_scope_id;
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int    ipv6mr_interface;
};

#define IPPROTO_IP          0
#define IPPROTO_ICMP        1
#define IPPROTO_TCP         6
#define IPPROTO_UDP         17
#define IPPROTO_IPV6        41
#define IPPROTO_ICMPV6      58
#define IPPROTO_RAW         255

#define IPV6_UNICAST_HOPS   4
#define IPV6_MULTICAST_IF   9
#define IPV6_MULTICAST_HOPS 10
#define IPV6_MULTICAST_LOOP 11
#define IPV6_JOIN_GROUP     12
#define IPV6_LEAVE_GROUP    13
#define IPV6_V6ONLY         27

#endif /* _NETINET__IN_H_ */
