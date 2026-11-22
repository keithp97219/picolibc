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

#ifndef _SYS_STATFS_H_
#define _SYS_STATFS_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

_BEGIN_STD_C

#ifndef _FSBLKCNT_T_DECLARED /* as <sys/statvfs.h> */
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
#define _FSBLKCNT_T_DECLARED
#endif

typedef struct {
    int __val[2];
} fsid_t;

/*
 * Linux's statfs (not POSIX, which has statvfs): f_type names the file
 * system type, which statvfs doesn't report (coreutils' stat -f)
 */
struct statfs {
    long       f_type;    /* file system type (the kernel's magic number) */
    long       f_bsize;   /* optimal transfer block size */
    fsblkcnt_t f_blocks;  /* total blocks, in f_frsize units */
    fsblkcnt_t f_bfree;   /* free blocks */
    fsblkcnt_t f_bavail;  /* free blocks for unprivileged users */
    fsfilcnt_t f_files;   /* total inodes */
    fsfilcnt_t f_ffree;   /* free inodes */
    fsid_t     f_fsid;    /* file system id */
    long       f_namelen; /* longest file name */
    long       f_frsize;  /* fragment size */
    long       f_flags;   /* mount flags (the kernel's ST_ bits) */
    long       f_spare[4];
};

int statfs(const char *__path, struct statfs *__buf) __picolibc_export;
int fstatfs(int __fd, struct statfs *__buf) __picolibc_export;

/* The transitional large file names: struct statfs holds 64-bit counts */
#if __LARGEFILE64_VISIBLE
#define statfs64  statfs
#define fstatfs64 fstatfs
#endif

_END_STD_C

#endif /* _SYS_STATFS_H_ */
