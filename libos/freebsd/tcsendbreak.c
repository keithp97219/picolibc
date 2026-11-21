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

#include "local-termios.h"
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-timespec-struct.h>

int
tcsendbreak(int fd, int duration)
{
    /* Like FreeBSD's C library, send a 0.4 second break whatever the duration */
    struct __freebsd_timespec ts = { 0, 400000000L };
    struct __freebsd_termios  ktermios;
    long                      ret;

    (void)duration;
    ret = _syscall_raw(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCSBRK);
    if (ret == _FREEBSD_ERR(FREEBSD_ENOTTY)) {
        /*
         * A terminal that can't send a break, like a pseudo-terminal,
         * isn't asynchronous serial; POSIX lets that do nothing
         */
        return _tcgetattr_freebsd(fd, &ktermios) < 0 ? -1 : 0;
    }
    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    (void)_syscall_raw(FREEBSD_SYS_nanosleep, &ts, NULL);
    return (int)_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCCBRK);
}
