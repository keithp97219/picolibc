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

#ifndef _MNTENT_H_
#define _MNTENT_H_

#include <sys/cdefs.h>
#include <stdio.h>

_BEGIN_STD_C

/*
 * The mounted file system table (a BSD/SunOS interface kept by glibc
 * and musl): fstab-format lines of fsname, dir, type, options, freq
 * and passno. Linux's /etc/mtab is /proc/self/mounts.
 */

#define MNTTAB          "/etc/fstab"
#define MOUNTED         "/etc/mtab"

#define MNTTYPE_IGNORE  "ignore"
#define MNTTYPE_NFS     "nfs"
#define MNTTYPE_SWAP    "swap"

#define MNTOPT_DEFAULTS "defaults"
#define MNTOPT_RO       "ro"
#define MNTOPT_RW       "rw"
#define MNTOPT_SUID     "suid"
#define MNTOPT_NOSUID   "nosuid"
#define MNTOPT_NOAUTO   "noauto"

struct mntent {
    char *mnt_fsname; /* device or server */
    char *mnt_dir;    /* mount point */
    char *mnt_type;   /* file system type */
    char *mnt_opts;   /* comma-separated options */
    int   mnt_freq;   /* dump frequency in days */
    int   mnt_passno; /* fsck pass number */
};

FILE          *setmntent(const char *__file, const char *__mode) __picolibc_export;
struct mntent *getmntent(FILE *__stream) __picolibc_export;
struct mntent *getmntent_r(FILE *__stream, struct mntent *__mnt, char *__buf, int __buflen)
    __picolibc_export;
int   addmntent(FILE *__stream, const struct mntent *__mnt) __picolibc_export;
int   endmntent(FILE *__stream) __picolibc_export;
char *hasmntopt(const struct mntent *__mnt, const char *__opt) __picolibc_export;

_END_STD_C

#endif /* _MNTENT_H_ */
