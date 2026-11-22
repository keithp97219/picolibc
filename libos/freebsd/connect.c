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
#include "local-cancel.h"
#include <stddef.h>

int
connect(int fd, const struct sockaddr *addr, socklen_t len)
{
    void *cancel = _cancel_enter();
    long  ret = _syscall_raw(FREEBSD_SYS_connect, fd, addr, len);

    _cancel_leave(cancel);

    /*
     * AF_UNSPEC resets a datagram socket's peer. FreeBSD does that, but
     * then reports that it can't connect to the address
     */
    if (ret < 0 && addr && len >= offsetof(struct sockaddr, sa_family) + sizeof(sa_family_t)
        && addr->sa_family == AF_UNSPEC) {
        struct sockaddr_storage ss;
        socklen_t               slen = sizeof(ss);
        int                     type;
        socklen_t               tlen = sizeof(type);

        if (_syscall_raw(FREEBSD_SYS_getsockopt, fd, SOL_SOCKET, SO_TYPE, &type, &tlen) == 0
            && (type == SOCK_DGRAM || type == SOCK_RAW)
            && _syscall_raw(FREEBSD_SYS_getpeername, fd, &ss, &slen) < 0)
            return 0;
    }
    return (int)_sock_result(ret);
}
