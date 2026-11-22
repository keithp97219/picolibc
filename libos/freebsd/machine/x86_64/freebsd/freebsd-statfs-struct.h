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

#ifndef _FREEBSD_STATFS_STRUCT_H_
#define _FREEBSD_STATFS_STRUCT_H_

struct __freebsd_statfs {
    __uint8_t  __adjust_0[4];
    __uint32_t f_type;
    __uint64_t f_flags;
    __uint64_t f_bsize;
    __uint64_t f_iosize;
    __uint64_t f_blocks;
    __uint64_t f_bfree;
    __int64_t  f_bavail;
    __uint64_t f_files;
    __int64_t  f_ffree;
    __uint8_t  __adjust_72[112];
    __uint32_t f_namemax;
    __uint32_t f_owner;
    __int32_t  f_fsid[2];
    __uint8_t  __adjust_200[80];
    char       f_fstypename[16];
    char       f_mntfromname[1024];
    char       f_mntonname[1024];
};

_Static_assert(sizeof(struct __freebsd_statfs) == 2344, "struct statfs size");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_flags) == 8, "struct statfs.f_flags");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_bsize) == 16, "struct statfs.f_bsize");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_iosize) == 24,
               "struct statfs.f_iosize");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_blocks) == 32,
               "struct statfs.f_blocks");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_bfree) == 40, "struct statfs.f_bfree");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_bavail) == 48,
               "struct statfs.f_bavail");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_files) == 56, "struct statfs.f_files");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_ffree) == 64, "struct statfs.f_ffree");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_namemax) == 184,
               "struct statfs.f_namemax");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_fsid) == 192, "struct statfs.f_fsid");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_type) == 4, "struct statfs.f_type");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_owner) == 188,
               "struct statfs.f_owner");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_fstypename) == 280,
               "struct statfs.f_fstypename");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_mntfromname) == 296,
               "struct statfs.f_mntfromname");
_Static_assert(__builtin_offsetof(struct __freebsd_statfs, f_mntonname) == 1320,
               "struct statfs.f_mntonname");

#endif /* _FREEBSD_STATFS_STRUCT_H_ */
