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
#include "local-cancel.h"
#include <stdarg.h>
#include <freebsd/freebsd-errno.h>

int
openat(int dirfd, const char *path, int flags, ...)
{
    va_list ap;
    int     mode;
    void   *cancel;
    long    ret;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    cancel = _cancel_enter();
    ret = _syscall_raw(FREEBSD_SYS_openat, _at_fd_to_freebsd(dirfd), path,
                       _open_flags_to_freebsd(flags), mode);
    _cancel_leave(cancel);
    if (ret < 0) {
        /* FreeBSD reports EMLINK for O_NOFOLLOW on a symbolic link, POSIX ELOOP */
        errno = (ret == _FREEBSD_ERR(FREEBSD_EMLINK) && (flags & O_NOFOLLOW)) ? ELOOP
                                                                              : _syscall_errno(ret);
        return -1;
    }
    return (int)ret;
}
