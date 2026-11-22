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
#include "local-dev.h"
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-stat-struct.h>

int
_statbuf(struct stat *statbuf, const struct __freebsd_stat *kstat)
{
    statbuf->st_dev = _dev_from_freebsd(kstat->st_dev);
    statbuf->st_ino = kstat->st_ino;
    /* FreeBSD's file type and permission bits are the traditional ones, as here */
    statbuf->st_mode = kstat->st_mode;
    statbuf->st_nlink = kstat->st_nlink;
    statbuf->st_uid = kstat->st_uid;
    statbuf->st_gid = kstat->st_gid;
    /* FreeBSD reports NODEV for anything but a device; picolibc reports 0 */
    statbuf->st_rdev = 0;
    if (S_ISCHR(statbuf->st_mode) || S_ISBLK(statbuf->st_mode))
        statbuf->st_rdev = _dev_from_freebsd(kstat->st_rdev);
    statbuf->st_size = kstat->st_size;
    statbuf->st_blksize = kstat->st_blksize;
    statbuf->st_blocks = kstat->st_blocks;
    statbuf->st_atim.tv_sec = kstat->st_atim.tv_sec;
    statbuf->st_atim.tv_nsec = kstat->st_atim.tv_nsec;
    statbuf->st_mtim.tv_sec = kstat->st_mtim.tv_sec;
    statbuf->st_mtim.tv_nsec = kstat->st_mtim.tv_nsec;
    statbuf->st_ctim.tv_sec = kstat->st_ctim.tv_sec;
    statbuf->st_ctim.tv_nsec = kstat->st_ctim.tv_nsec;
    return 0;
}
