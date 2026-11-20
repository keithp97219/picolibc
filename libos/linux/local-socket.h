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

#ifndef _LOCAL_SOCKET_H_
#define _LOCAL_SOCKET_H_

#include "local-linux.h"
#include <sys/socket.h>
#include <net/if.h>

/* Kernel values the socket ABI headers don't publish */
#define LINUX_SIOCATMARK   0x8905
#define LINUX_SIOCGIFNAME  0x8910
#define LINUX_SIOCGIFINDEX 0x8933

/* One past the largest socket type the kernel knows */
#define LINUX_SOCK_MAX 11

/*
 * Linux says EINVAL for a socket type it doesn't know, or a type with
 * unknown flags, and for an out-of-range protocol number. POSIX has
 * EPROTOTYPE and EPROTONOSUPPORT for those.
 */
static inline int
_socket_einval(int type)
{
    int base = type & ~(SOCK_NONBLOCK | SOCK_CLOEXEC);

    return base <= 0 || base >= LINUX_SOCK_MAX ? EPROTOTYPE : EPROTONOSUPPORT;
}

/* The 64-bit timeout SO_RCVTIMEO and SO_SNDTIMEO use on 32-bit targets */
struct __kernel_sock_timeval {
    __int64_t tv_sec;
    __int64_t tv_usec;
};

/* The parts of struct ifreq used here, at the kernel's size */
struct __kernel_ifreq {
    char ifr_name[IF_NAMESIZE];
    union {
        int  ifr_ifindex;
        char __pad[sizeof(void *) == 8 ? 24 : 16];
    };
};

#endif /* _LOCAL_SOCKET_H_ */
