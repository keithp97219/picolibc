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

#ifndef _FREEBSD_MSGHDR_STRUCT_H_
#define _FREEBSD_MSGHDR_STRUCT_H_

struct __freebsd_msghdr {
    void      *msg_name;
    __uint32_t msg_namelen;
    __uint8_t  __adjust_12[4];
    void      *msg_iov;
    int        msg_iovlen;
    __uint8_t  __adjust_28[4];
    void      *msg_control;
    __uint32_t msg_controllen;
    int        msg_flags;
};

_Static_assert(sizeof(struct __freebsd_msghdr) == 48, "struct msghdr size");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_name) == 0,
               "struct msghdr.msg_name");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_namelen) == 8,
               "struct msghdr.msg_namelen");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_iov) == 16, "struct msghdr.msg_iov");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_iovlen) == 24,
               "struct msghdr.msg_iovlen");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_control) == 32,
               "struct msghdr.msg_control");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_controllen) == 40,
               "struct msghdr.msg_controllen");
_Static_assert(__builtin_offsetof(struct __freebsd_msghdr, msg_flags) == 44,
               "struct msghdr.msg_flags");

#endif /* _FREEBSD_MSGHDR_STRUCT_H_ */
