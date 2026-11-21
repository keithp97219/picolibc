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
#include <unistd.h>

int
tcsetpgrp(int fd, pid_t pgrp)
{
    int          cur;
    unsigned int ptn;
    long         ret = _syscall_raw(LINUX_SYS_ioctl, fd, LINUX_TIOCSPGRP, &pgrp);

    if (ret >= 0)
        return 0;
    ret = _syscall_errno(ret);
    switch (ret) {
    case ESRCH:
        /* Linux reports a group that doesn't exist with ESRCH */
        ret = EPERM;
        break;
    case ENOTTY:
        /*
         * Linux turns the EIO for an orphaned background group into
         * ENOTTY. When the descriptor is the caller's controlling
         * terminal (TIOCGPGRP works on it and it isn't a pty manager),
         * that is what happened.
         */
        if (_syscall_raw(LINUX_SYS_ioctl, fd, LINUX_TIOCGPGRP, &cur) >= 0
            && _syscall_raw(LINUX_SYS_ioctl, fd, LINUX_TIOCGPTN, &ptn) < 0)
            ret = EIO;
        break;
    }
    errno = (int)ret;
    return -1;
}
