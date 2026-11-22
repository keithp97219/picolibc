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
#include "local-cancel.h"
#include <freebsd/freebsd-poll.h>
#include <freebsd/freebsd-pollfd-struct.h>

/*
 * FreeBSD's canonical-mode terminal read ends at a NUL byte as if it
 * were a line delimiter: ttyinq_findchar (sys/kern/tty_inq.c) looks
 * for the delimiters with strchr, which also matches the string's
 * terminating NUL. POSIX makes NUL ordinary data, a read returning
 * the line through its NL, EOL or EOF.
 *
 * The kernel hands out only whole lines, so when a read stopped at a
 * NUL, the rest of that line is already waiting. When a read returns
 * less than asked for and ends in NUL, and fd is a terminal in
 * canonical mode whose EOL and EOF characters aren't NUL, more is read
 * while the last chunk ends in NUL and poll says there is more, so it
 * never blocks. A zero-byte read is the line's EOF, which the kernel
 * has discarded, and an error ends the line with the bytes already
 * read. Other reads pay only the test of the last byte; a short read
 * of another kind of file ending in NUL pays one failing ioctl.
 *
 * A NUL quoted with LNEXT doesn't end a read; one just before an EOF
 * that ended the line can't be told from the kernel's stop, and the
 * next line, if it's already complete, would be read too.
 */
static ssize_t
_read_canonical_nul(int fd, char *buf, size_t got, size_t count)
{
    struct __freebsd_termios kt;
    struct __freebsd_pollfd  pfd;
    long                     ret;

    if (_syscall_raw(FREEBSD_SYS_ioctl, fd, FREEBSD_TIOCGETA, &kt) < 0)
        return (ssize_t)got;
    if (!(kt.c_lflag & FREEBSD_ICANON) || kt.c_cc[FREEBSD_VEOL] == 0 || kt.c_cc[FREEBSD_VEOF] == 0)
        return (ssize_t)got;

    while (got < count && buf[got - 1] == '\0') {
        pfd.fd = fd;
        pfd.events = FREEBSD_POLLIN;
        pfd.revents = 0;
        ret = _syscall_raw(FREEBSD_SYS_poll, &pfd, 1, 0);
        if (ret <= 0 || !(pfd.revents & FREEBSD_POLLIN))
            break;
        ret = _syscall_raw(FREEBSD_SYS_read, fd, buf + got, count - got);
        if (ret <= 0)
            break;
        got += (size_t)ret;
    }
    return (ssize_t)got;
}

ssize_t
read(int fd, void *buf, size_t count)
{
    void *cancel = _cancel_enter();
    long  ret = _syscall(FREEBSD_SYS_read, fd, buf, count);

    if (ret > 0 && (size_t)ret < count && ((char *)buf)[ret - 1] == '\0')
        ret = _read_canonical_nul(fd, buf, (size_t)ret, count);
    _cancel_leave(cancel);
    return ret;
}
