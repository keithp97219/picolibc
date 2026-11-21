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

int
tcflow(int fd, int action)
{
    struct __freebsd_termios ktermios;
    unsigned char            c;

    switch (action) {
    case TCOOFF:
        return (int)_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCSTOP);
    case TCOON:
        return (int)_syscall(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCSTART);
    case TCIOFF:
    case TCION:
        /* Send the terminal's STOP or START character */
        if (_tcgetattr_freebsd(fd, &ktermios) < 0)
            return -1;
        c = ktermios.c_cc[action == TCIOFF ? FREEBSD_VSTOP : FREEBSD_VSTART];
        if (c != _POSIX_VDISABLE && _syscall(FREEBSD_SYS_write, fd, &c, 1) < 0)
            return -1;
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}
