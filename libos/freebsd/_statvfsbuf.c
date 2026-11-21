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

int
_statvfsbuf(struct statvfs *buf, const struct __freebsd_statfs *kbuf)
{
    /* FreeBSD's f_bsize is the fragment size its block counts use */
    buf->f_bsize = (unsigned long)kbuf->f_iosize;
    buf->f_frsize = (unsigned long)kbuf->f_bsize;
    buf->f_blocks = (fsblkcnt_t)kbuf->f_blocks;
    buf->f_bfree = (fsblkcnt_t)kbuf->f_bfree;
    buf->f_bavail = kbuf->f_bavail < 0 ? 0 : (fsblkcnt_t)kbuf->f_bavail;
    buf->f_files = (fsfilcnt_t)kbuf->f_files;
    buf->f_ffree = kbuf->f_ffree < 0 ? 0 : (fsfilcnt_t)kbuf->f_ffree;
    buf->f_favail = buf->f_ffree;
    buf->f_fsid = (unsigned long)(((unsigned long long)(unsigned int)kbuf->f_fsid[0])
                                  | ((unsigned long long)(unsigned int)kbuf->f_fsid[1] << 32));
    buf->f_flag = 0;
    if (kbuf->f_flags & FREEBSD_MNT_RDONLY)
        buf->f_flag |= ST_RDONLY;
    if (kbuf->f_flags & FREEBSD_MNT_NOSUID)
        buf->f_flag |= ST_NOSUID;
    buf->f_namemax = (unsigned long)kbuf->f_namemax;
    return 0;
}
