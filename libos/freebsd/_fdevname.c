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
#include <freebsd/freebsd-fiodgname_arg-struct.h>
#include <limits.h>
#include <string.h>

/*
 * Store "/dev/" and the name of the device fd refers to in buf.
 * Returns 0 or an errno value.
 */
int
_fdevname(int fd, char *buf, size_t size)
{
    struct __freebsd_fiodgname_arg arg;
    long                           ret;

    if (size <= 5)
        return ERANGE;
    memcpy(buf, "/dev/", 5);
    arg.len = size - 5 > INT_MAX ? INT_MAX : (int)(size - 5);
    arg.buf = buf + 5;
    ret = _syscall_raw(FREEBSD_SYS_ioctl, fd, FREEBSD_FIODGNAME, &arg);
    if (ret < 0) {
        /* The kernel says EINVAL for a name that doesn't fit */
        if (ret == _FREEBSD_ERR(FREEBSD_EINVAL))
            return ERANGE;
        return _syscall_errno(ret);
    }
    return 0;
}
