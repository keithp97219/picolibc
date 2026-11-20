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

#include "local-linux.h"
#include <linux/linux-errno.h>
#include <stdbool.h>
#include <sys/socket.h>

/* An AF_UNIX socket with neither a name nor a peer */
static bool
_unbound_local(int fd)
{
    struct sockaddr_storage ss;
    int                     domain;
    socklen_t               len = sizeof(domain);

    if (_syscall_raw(LINUX_SYS_getsockopt, fd, SOL_SOCKET, SO_DOMAIN, &domain, &len) != 0
        || domain != AF_UNIX)
        return false;
    len = sizeof(ss);
    if (_syscall_raw(LINUX_SYS_getsockname, fd, &ss, &len) != 0 || len > sizeof(sa_family_t))
        return false;
    len = sizeof(ss);
    return _syscall_raw(LINUX_SYS_getpeername, fd, &ss, &len) == -LINUX_ENOTCONN;
}

int
listen(int fd, int backlog)
{
    long ret;

    /* Linux takes a negative backlog as the largest one */
    if (backlog < 0)
        backlog = 0;
    ret = _syscall_raw(LINUX_SYS_listen, fd, backlog);
    if (ret == 0)
        return 0;
    /* Linux says EINVAL for an AF_UNIX socket without a name */
    if (ret == -LINUX_EINVAL && _unbound_local(fd))
        errno = EDESTADDRREQ;
    else
        errno = _syscall_errno(ret);
    return -1;
}
