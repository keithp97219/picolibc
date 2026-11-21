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
#include <dirent.h>
#include <string.h>
#include <freebsd/freebsd-dirent.h>
#include <freebsd/freebsd-dirent-struct.h>

static unsigned char
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

struct dirent *
readdir(DIR *dir)
{
    struct __freebsd_dirent *k_dirent;
    struct dirent           *dirent;
    size_t                   name_len;

    for (;;) {
        if (dir->__dd_offset == dir->__dd_count) {
            __int64_t base;
            ssize_t   ret = _syscall(FREEBSD_SYS_getdirentries, dir->__dd_fd, dir->__dd_buf,
                                     sizeof(dir->__dd_buf), &base);
            if (ret <= 0)
                return NULL;
            dir->__dd_offset = 0;
            dir->__dd_count = (size_t)ret;
        }
        k_dirent = (struct __freebsd_dirent *)(dir->__dd_buf + dir->__dd_offset);
        dir->__dd_offset += k_dirent->d_reclen;
        /* Entries for removed files (whiteouts and empty slots) have no inode */
        if (k_dirent->d_fileno != 0 && k_dirent->d_type != FREEBSD_DT_WHT)
            break;
    }

    dirent = &dir->__dd_dirent;
    dirent->d_ino = (ino_t)k_dirent->d_fileno;
    dirent->d_type = _dt_from_freebsd(k_dirent->d_type);

    name_len = k_dirent->d_namlen;
    if (name_len >= sizeof(dirent->d_name))
        name_len = sizeof(dirent->d_name) - 1;
    memcpy(dirent->d_name, k_dirent->d_name, name_len);
    dirent->d_name[name_len] = '\0';
    return dirent;
}
