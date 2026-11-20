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

#define _DEFAULT_SOURCE
#include "local-termios.h"
#include <unistd.h>
#include <stdio.h>

int
ttyname_r(int fd, char *buf, size_t bufsize)
{
    struct __kernel_termios2 ktermios;
    char                     fdpath[sizeof("/proc/self/fd/") + 12];
    long                     ret;

    /* Only a terminal has a terminal name */
    ret = _syscall_raw(LINUX_SYS_ioctl, fd, LINUX_TCGETS2, &ktermios);
    if (ret < 0)
        return _syscall_errno(ret);
    if (bufsize == 0)
        return ERANGE;
    (void)snprintf(fdpath, sizeof(fdpath), "/proc/self/fd/%d", fd);
#ifdef LINUX_SYS_readlink
    ret = _syscall_raw(LINUX_SYS_readlink, fdpath, buf, bufsize);
#else
    ret = _syscall_raw(LINUX_SYS_readlinkat, LINUX_AT_FDCWD, fdpath, buf, bufsize);
#endif
    if (ret < 0)
        return _syscall_errno(ret);
    /* readlink adds no null, and cuts off a name that doesn't fit */
    if ((size_t)ret >= bufsize)
        return ERANGE;
    buf[ret] = '\0';
    return 0;
}
