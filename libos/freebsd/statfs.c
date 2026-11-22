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
#include <freebsd/freebsd-mount-flags.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>

#define MAP_MNT(f)                       \
    do {                                 \
        if (kbuf->f_flags & FREEBSD_##f) \
            buf->f_flags |= f;           \
    } while (0)

/* The kernel's statfs as picolibc's */
void
_statfsbuf(struct statfs *buf, const struct __freebsd_statfs *kbuf)
{
    memset(buf, 0, sizeof(*buf));
    buf->f_type = kbuf->f_type;
    MAP_MNT(MNT_RDONLY);
    MAP_MNT(MNT_SYNCHRONOUS);
    MAP_MNT(MNT_NOEXEC);
    MAP_MNT(MNT_NOSUID);
    MAP_MNT(MNT_ASYNC);
    MAP_MNT(MNT_LOCAL);
    MAP_MNT(MNT_QUOTA);
    MAP_MNT(MNT_ROOTFS);
    MAP_MNT(MNT_IGNORE);
    MAP_MNT(MNT_NOATIME);
    MAP_MNT(MNT_AUTOMOUNTED);
    buf->f_bsize = kbuf->f_bsize;
    buf->f_iosize = kbuf->f_iosize;
    buf->f_blocks = kbuf->f_blocks;
    buf->f_bfree = kbuf->f_bfree;
    buf->f_bavail = kbuf->f_bavail;
    buf->f_files = kbuf->f_files;
    buf->f_ffree = kbuf->f_ffree;
    buf->f_namemax = kbuf->f_namemax;
    buf->f_owner = (uid_t)kbuf->f_owner;
    buf->f_fsid.val[0] = kbuf->f_fsid[0];
    buf->f_fsid.val[1] = kbuf->f_fsid[1];
    _Static_assert(MFSNAMELEN == FREEBSD_MFSNAMELEN, "MFSNAMELEN");
    _Static_assert(MNAMELEN == FREEBSD_MNAMELEN, "MNAMELEN");
    memcpy(buf->f_fstypename, kbuf->f_fstypename, MFSNAMELEN);
    memcpy(buf->f_mntfromname, kbuf->f_mntfromname, MNAMELEN);
    memcpy(buf->f_mntonname, kbuf->f_mntonname, MNAMELEN);
    buf->f_fstypename[MFSNAMELEN - 1] = '\0';
    buf->f_mntfromname[MNAMELEN - 1] = '\0';
    buf->f_mntonname[MNAMELEN - 1] = '\0';
}

int
statfs(const char *path, struct statfs *buf)
{
    struct __freebsd_statfs kbuf;

    if (_syscall(FREEBSD_SYS_statfs, path, &kbuf) < 0)
        return -1;
    _statfsbuf(buf, &kbuf);
    return 0;
}

int
fstatfs(int fd, struct statfs *buf)
{
    struct __freebsd_statfs kbuf;

    if (_syscall(FREEBSD_SYS_fstatfs, fd, &kbuf) < 0)
        return -1;
    _statfsbuf(buf, &kbuf);
    return 0;
}

static int
_mnt_mode(int mode)
{
    return mode == MNT_WAIT ? FREEBSD_MNT_WAIT : FREEBSD_MNT_NOWAIT;
}

int
getfsstat(struct statfs *buf, long bufsize, int mode)
{
    struct __freebsd_statfs *kbufs;
    long                     n, want, i;

    if (mode != MNT_WAIT && mode != MNT_NOWAIT) {
        errno = EINVAL;
        return -1;
    }
    if (!buf || bufsize < (long)sizeof(*buf))
        return (int)_syscall(FREEBSD_SYS_getfsstat, NULL, 0, _mnt_mode(mode));
    want = bufsize / (long)sizeof(*buf);
    kbufs = malloc((size_t)want * sizeof(*kbufs));
    if (!kbufs) {
        errno = ENOMEM;
        return -1;
    }
    n = _syscall(FREEBSD_SYS_getfsstat, kbufs, want * (long)sizeof(*kbufs), _mnt_mode(mode));
    for (i = 0; i < n; i++)
        _statfsbuf(&buf[i], &kbufs[i]);
    free(kbufs);
    return (int)n;
}

/* The mount table in storage reused by the next call */
int
getmntinfo(struct statfs **mntbufp, int mode)
{
    static struct statfs *table;
    static long           table_len;
    int                   n;

    for (;;) {
        n = getfsstat(NULL, 0, mode);
        if (n < 0)
            return 0;
        if (n + 1 > table_len) {
            struct statfs *grown = realloc(table, (size_t)(n + 1) * sizeof(*table));

            if (!grown)
                return 0;
            table = grown;
            table_len = n + 1;
        }
        n = getfsstat(table, table_len * (long)sizeof(*table), mode);
        if (n < 0)
            return 0;
        /* More mounts than were counted: try again */
        if (n < table_len)
            break;
    }
    *mntbufp = table;
    return n;
}
