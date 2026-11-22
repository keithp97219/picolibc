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

#ifndef _LOCAL_DIRENT_H_
#define _LOCAL_DIRENT_H_

#include "local-freebsd.h"
#include <dirent.h>
#include <freebsd/freebsd-dirent.h>
#include <freebsd/freebsd-dirent-struct.h>

static inline unsigned char
_dt_from_freebsd(__uint8_t kdt)
{
    switch (kdt) {
    case FREEBSD_DT_BLK:
        return DT_BLK;
    case FREEBSD_DT_CHR:
        return DT_CHR;
    case FREEBSD_DT_DIR:
        return DT_DIR;
    case FREEBSD_DT_FIFO:
        return DT_FIFO;
    case FREEBSD_DT_LNK:
        return DT_LNK;
    case FREEBSD_DT_REG:
        return DT_REG;
    case FREEBSD_DT_SOCK:
        return DT_SOCK;
    default:
        return DT_UNKNOWN;
    }
}

/* The largest record the kernel writes */
#define _DIRENT_RECLEN_MAX sizeof(struct __freebsd_dirent)

/* Read directory entries */
static inline long
_getdents(int fd, void *buf, size_t nbyte)
{
    return _syscall(FREEBSD_SYS_getdirentries, fd, buf, nbyte, NULL);
}

/* As _getdents, returning a _syscall_raw error */
static inline long
_getdents_raw(int fd, void *buf, size_t nbyte)
{
    return _syscall_raw(FREEBSD_SYS_getdirentries, (long)fd, buf, nbyte, NULL);
}

static inline __uint64_t
_dirent_ino(const struct __freebsd_dirent *k)
{
    return k->d_fileno;
}

static inline size_t
_dirent_namlen(const struct __freebsd_dirent *k)
{
    return k->d_namlen;
}

static inline unsigned char
_dirent_type(const struct __freebsd_dirent *k)
{
    return _dt_from_freebsd(k->d_type);
}

/* Entries for removed files (whiteouts and empty slots) have no inode */
static inline int
_dirent_is_hole(const struct __freebsd_dirent *k)
{
    return k->d_fileno == 0 || k->d_type == FREEBSD_DT_WHT;
}

#endif /* _LOCAL_DIRENT_H_ */
