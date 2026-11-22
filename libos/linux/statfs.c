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

#include "local-statfs.h"
#include <string.h>
#include <sys/statfs.h>

/* Copy the kernel's statfs out as picolibc's */
void
_statfsbuf(struct statfs *buf, const struct __kernel_statfs_size *kbuf)
{
    memset(buf, 0, sizeof(*buf));
    buf->f_type = (long)kbuf->f_type;
    buf->f_bsize = (long)kbuf->f_bsize;
    buf->f_blocks = (fsblkcnt_t)kbuf->f_blocks;
    buf->f_bfree = (fsblkcnt_t)kbuf->f_bfree;
    buf->f_bavail = (fsblkcnt_t)kbuf->f_bavail;
    buf->f_files = (fsfilcnt_t)kbuf->f_files;
    buf->f_ffree = (fsfilcnt_t)kbuf->f_ffree;
    buf->f_fsid.__val[0] = kbuf->f_fsid.__val[0];
    buf->f_fsid.__val[1] = kbuf->f_fsid.__val[1];
    buf->f_namelen = (long)kbuf->f_namelen;
    buf->f_frsize = (long)kbuf->f_frsize;
    buf->f_flags = (long)kbuf->f_flags;
}

int
statfs(const char *path, struct statfs *buf)
{
    struct __kernel_statfs_size kbuf;
    int                         ret;

#ifdef LINUX_SYS_statfs64
    ret = _syscall(LINUX_SYS_statfs64, path, sizeof(kbuf), &kbuf);
#else
    ret = _syscall(LINUX_SYS_statfs, path, &kbuf);
#endif
    if (ret < 0)
        return ret;
    _statfsbuf(buf, &kbuf);
    return 0;
}
