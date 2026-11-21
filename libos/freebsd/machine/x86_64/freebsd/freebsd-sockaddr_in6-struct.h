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

#ifndef _FREEBSD_SOCKADDR_IN6_STRUCT_H_
#define _FREEBSD_SOCKADDR_IN6_STRUCT_H_

struct __freebsd_sockaddr_in6 {
    __uint8_t  sin6_len;
    __uint8_t  sin6_family;
    __uint16_t sin6_port;
    __uint32_t sin6_flowinfo;
    __uint8_t  sin6_addr[16];
    __uint32_t sin6_scope_id;
};

_Static_assert(sizeof(struct __freebsd_sockaddr_in6) == 28, "struct sockaddr_in6 size");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_len) == 0,
               "struct sockaddr_in6.sin6_len");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_family) == 1,
               "struct sockaddr_in6.sin6_family");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_port) == 2,
               "struct sockaddr_in6.sin6_port");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_flowinfo) == 4,
               "struct sockaddr_in6.sin6_flowinfo");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_addr) == 8,
               "struct sockaddr_in6.sin6_addr");
_Static_assert(__builtin_offsetof(struct __freebsd_sockaddr_in6, sin6_scope_id) == 24,
               "struct sockaddr_in6.sin6_scope_id");

#endif /* _FREEBSD_SOCKADDR_IN6_STRUCT_H_ */
