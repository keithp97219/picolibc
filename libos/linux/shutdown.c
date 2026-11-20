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

/*
 * Linux shuts down AF_UNIX sockets that aren't connected; other
 * families report ENOTCONN. Listening sockets stay allowed, as for TCP.
 */
static bool
_unconnected_local(int fd)
{
    struct sockaddr_storage ss;
    int                     value;
    socklen_t               len = sizeof(ss);

    if (_syscall_raw(LINUX_SYS_getpeername, fd, &ss, &len) != -LINUX_ENOTCONN)
        return false;
    len = sizeof(value);
    if (_syscall_raw(LINUX_SYS_getsockopt, fd, SOL_SOCKET, SO_DOMAIN, &value, &len) != 0
        || value != AF_UNIX)
        return false;
    len = sizeof(value);
    return _syscall_raw(LINUX_SYS_getsockopt, fd, SOL_SOCKET, SO_ACCEPTCONN, &value, &len) == 0
        && value == 0;
}

int
shutdown(int fd, int how)
{
    if ((how == SHUT_RD || how == SHUT_WR || how == SHUT_RDWR) && _unconnected_local(fd)) {
        errno = ENOTCONN;
        return -1;
    }
    return (int)_syscall(LINUX_SYS_shutdown, fd, how);
}
