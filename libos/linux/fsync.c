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
#include "local-cancel.h"
#include <unistd.h>

int
fsync(int fd)
{
    void *cancel = _cancel_enter();
    char  name[_OPATH_NAME_MAX];
    long  ret = _syscall_raw(LINUX_SYS_fsync, fd);

    /*
     * An O_SEARCH (O_PATH) descriptor: sync the file it names through
     * a descriptor that allows it
     */
    if (ret == -LINUX_EBADF && _opath_name(fd, name)) {
        /* Read-only is 0 */
        long rfd = _syscall_raw(LINUX_SYS_openat, (long)(int)LINUX_AT_FDCWD, name, LINUX_O_CLOEXEC);

        if (rfd >= 0) {
            ret = _syscall_raw(LINUX_SYS_fsync, rfd);
            (void)_syscall_raw(LINUX_SYS_close, rfd);
        } else {
            ret = rfd;
        }
    }
    _cancel_leave(cancel);
    if (ret < 0) {
        errno = _syscall_errno(ret);
        return -1;
    }
    return 0;
}
