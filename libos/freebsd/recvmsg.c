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
#include <string.h>

ssize_t
recvmsg(int fd, struct msghdr *msg, int flags)
{
    struct sockaddr_storage ss;
    struct msghdr           h;
    size_t                  total = 0;
    int                     lowat, start;
    long                    ret;

    if (!msg)
        return (ssize_t)_sock_result(_syscall_raw(FREEBSD_SYS_recvmsg, fd, msg, flags));
    if (!_msg_iov_valid(msg->msg_iov, msg->msg_iovlen))
        return -1;
    for (int i = 0; i < msg->msg_iovlen; i++)
        total += msg->msg_iov[i].iov_len;
    start = _sock_recv_start(fd, flags, total, &lowat);
    if (start < 0)
        return -1;
    /* Receive the address whole, to report its length */
    h = *msg;
    if (h.msg_name) {
        h.msg_name = &ss;
        h.msg_namelen = sizeof(ss);
    }
    ret = _syscall_raw(FREEBSD_SYS_recvmsg, fd, &h, flags);
    if (start)
        _sock_recv_end(fd, lowat);
    if (ret >= 0) {
        if (msg->msg_name)
            _sock_addr_out(msg->msg_name, &msg->msg_namelen, &ss, h.msg_namelen);
        else
            msg->msg_namelen = h.msg_namelen;
        msg->msg_controllen = h.msg_controllen;
        msg->msg_flags = h.msg_flags;
    }
    return (ssize_t)_sock_result(ret);
}
