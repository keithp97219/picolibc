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

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>

_BEGIN_STD_C

/*
 * FreeBSD's file system statistics and mount table (not POSIX, which
 * has statvfs): statfs with the file system type name and mount
 * points, getfsstat and getmntinfo. picolibc's layout; libos/freebsd
 * translates the kernel's.
 */

#define MFSNAMELEN 16   /* file system type name, with the null */
#define MNAMELEN   1024 /* mount point and mounted-from names */

typedef struct fsid {
    int32_t val[2];
} fsid_t;

struct statfs {
    uint32_t f_version; /* structure version */
    uint32_t f_type;    /* file system type number */
    uint64_t f_flags;   /* MNT_ flags */
    uint64_t f_bsize;   /* fragment size */
    uint64_t f_iosize;  /* optimal transfer block size */
    uint64_t f_blocks;  /* total data blocks */
    uint64_t f_bfree;   /* free blocks */
    int64_t  f_bavail;  /* free blocks for unprivileged users */
    uint64_t f_files;   /* total file nodes */
    int64_t  f_ffree;   /* free nodes for unprivileged users */
    uint32_t f_namemax; /* longest file name */
    uid_t    f_owner;   /* user that mounted the file system */
    fsid_t   f_fsid;    /* file system id */
    char     f_fstypename[MFSNAMELEN];
    char     f_mntfromname[MNAMELEN];
    char     f_mntonname[MNAMELEN];
};

/* f_flags */
#define MNT_RDONLY      0x0000000000000001ULL
#define MNT_SYNCHRONOUS 0x0000000000000002ULL
#define MNT_NOEXEC      0x0000000000000004ULL
#define MNT_NOSUID      0x0000000000000008ULL
#define MNT_ASYNC       0x0000000000000040ULL
#define MNT_LOCAL       0x0000000000001000ULL
#define MNT_QUOTA       0x0000000000002000ULL
#define MNT_ROOTFS      0x0000000000004000ULL
#define MNT_IGNORE      0x0000000000800000ULL
#define MNT_NOATIME     0x0000000010000000ULL
#define MNT_AUTOMOUNTED 0x0000000200000000ULL

/* getfsstat and getmntinfo modes */
#define MNT_WAIT   1
#define MNT_NOWAIT 2

int statfs(const char *__path, struct statfs *__buf) __picolibc_export;
int fstatfs(int __fd, struct statfs *__buf) __picolibc_export;
int getfsstat(struct statfs *__buf, long __bufsize, int __mode) __picolibc_export;
int getmntinfo(struct statfs **__mntbufp, int __mode) __picolibc_export;

_END_STD_C

#endif /* _SYS_MOUNT_H_ */
