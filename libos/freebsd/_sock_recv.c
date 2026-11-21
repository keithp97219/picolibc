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

#include "local-socket.h"

int
_sock_recv_start(int fd, int flags, size_t len, int *lowat)
{
    int       type, want;
    socklen_t olen = sizeof(type);

    if ((flags & MSG_OOB) && _sock_domain(fd) == AF_UNIX) {
        errno = EOPNOTSUPP;
        return -1;
    }
    if ((flags & (MSG_PEEK | MSG_WAITALL | MSG_DONTWAIT)) != (MSG_PEEK | MSG_WAITALL) || len <= 1)
        return 0;
    if (_syscall_raw(FREEBSD_SYS_getsockopt, fd, SOL_SOCKET, SO_TYPE, &type, &olen) < 0
        || type != SOCK_STREAM)
        return 0;
    olen = sizeof(*lowat);
    if (_syscall_raw(FREEBSD_SYS_getsockopt, fd, SOL_SOCKET, SO_RCVLOWAT, lowat, &olen) < 0)
        return 0;
    /* The kernel limits the mark to the buffer size */
    want = len > INT_MAX ? INT_MAX : (int)len;
    if (want <= *lowat
        || _syscall_raw(FREEBSD_SYS_setsockopt, fd, SOL_SOCKET, SO_RCVLOWAT, &want, sizeof(want))
            < 0)
        return 0;
    return 1;
}

void
_sock_recv_end(int fd, int lowat)
{
    (void)_syscall_raw(FREEBSD_SYS_setsockopt, fd, SOL_SOCKET, SO_RCVLOWAT, &lowat, sizeof(lowat));
}
