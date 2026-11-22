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
#include <freebsd/freebsd-errno.h>

/* FreeBSD says EINVAL for a socket that can't listen; POSIX says EOPNOTSUPP */
static long
_accept_result(int fd, long ret)
{
    int       type;
    socklen_t len = sizeof(type);

    if (ret == _FREEBSD_ERR(FREEBSD_EINVAL)
        && _syscall_raw(FREEBSD_SYS_getsockopt, fd, SOL_SOCKET, SO_TYPE, &type, &len) == 0
        && type != SOCK_STREAM && type != SOCK_SEQPACKET)
        ret = _FREEBSD_ERR(FREEBSD_EOPNOTSUPP);
    return _sock_result(ret);
}

int
accept4(int fd, struct sockaddr * __restrict addr, socklen_t * __restrict len, int flags)
{
    struct sockaddr_storage ss;
    socklen_t               klen = sizeof(ss);
    long                    ret;
    void                   *cancel;

    /* SOCK_NONBLOCK, SOCK_CLOEXEC and SOCK_CLOFORK are FreeBSD's */
    cancel = _cancel_enter();
    if (!addr || !len)
        ret = _syscall_raw(FREEBSD_SYS_accept4, fd, addr, len, flags);
    else
        ret = _syscall_raw(FREEBSD_SYS_accept4, fd, &ss, &klen, flags);
    _cancel_leave(cancel);
    ret = _accept_result(fd, ret);
    if (ret >= 0 && addr && len)
        _sock_addr_out(addr, len, &ss, klen);
    return (int)ret;
}
