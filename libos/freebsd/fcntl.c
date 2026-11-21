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

#include "local-freebsd.h"
#include <stdarg.h>

/*
 * The descriptor and status flag commands; the others (record locks,
 * ownership) come later.
 */
int
fcntl(int fd, int cmd, ...)
{
    va_list ap;
    long    arg;
    long    ret;

    va_start(ap, cmd);
    arg = va_arg(ap, long);
    va_end(ap);

    switch (cmd) {
    case F_DUPFD:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_DUPFD, arg);
    case F_DUPFD_CLOEXEC:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_DUPFD_CLOEXEC, arg);
    case F_GETFD:
        ret = _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_GETFD);
        if (ret < 0)
            return -1;
        return (ret & FREEBSD_FD_CLOEXEC) ? FD_CLOEXEC : 0;
    case F_SETFD:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_SETFD,
                        (arg & FD_CLOEXEC) ? FREEBSD_FD_CLOEXEC : 0);
    case F_GETFL:
        ret = _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_GETFL);
        if (ret < 0)
            return -1;
        return _open_flags_from_freebsd((int)ret);
    case F_SETFL:
        return _syscall(FREEBSD_SYS_fcntl, fd, FREEBSD_F_SETFL, _open_flags_to_freebsd((int)arg));
    default:
        errno = EINVAL;
        return -1;
    }
}
