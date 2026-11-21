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

#ifndef _FREEBSD_STAT_STRUCT_H_
#define _FREEBSD_STAT_STRUCT_H_

struct __freebsd_stat {
    __uint64_t                st_dev;
    __uint64_t                st_ino;
    __uint64_t                st_nlink;
    __uint16_t                st_mode;
    __uint8_t                 __adjust_26[2];
    __uint32_t                st_uid;
    __uint32_t                st_gid;
    __uint8_t                 __adjust_36[4];
    __uint64_t                st_rdev;
    struct __freebsd_timespec st_atim;
    struct __freebsd_timespec st_mtim;
    struct __freebsd_timespec st_ctim;
    struct __freebsd_timespec st_birthtim;
    __int64_t                 st_size;
    __int64_t                 st_blocks;
    __int32_t                 st_blksize;
    __uint32_t                st_flags;
    __uint64_t                st_gen;
    __uint8_t                 __adjust_144[80];
};

_Static_assert(sizeof(struct __freebsd_stat) == 224, "struct stat size");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_dev) == 0, "struct stat.st_dev");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_ino) == 8, "struct stat.st_ino");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_nlink) == 16, "struct stat.st_nlink");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_mode) == 24, "struct stat.st_mode");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_uid) == 28, "struct stat.st_uid");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_gid) == 32, "struct stat.st_gid");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_rdev) == 40, "struct stat.st_rdev");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_atim) == 48, "struct stat.st_atim");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_mtim) == 64, "struct stat.st_mtim");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_ctim) == 80, "struct stat.st_ctim");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_birthtim) == 96,
               "struct stat.st_birthtim");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_size) == 112, "struct stat.st_size");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_blocks) == 120,
               "struct stat.st_blocks");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_blksize) == 128,
               "struct stat.st_blksize");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_flags) == 132, "struct stat.st_flags");
_Static_assert(__builtin_offsetof(struct __freebsd_stat, st_gen) == 136, "struct stat.st_gen");

#endif /* _FREEBSD_STAT_STRUCT_H_ */
