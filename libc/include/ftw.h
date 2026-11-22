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

#ifndef _FTW_H_
#define _FTW_H_

#include <sys/cdefs.h>
#include <sys/stat.h>

_BEGIN_STD_C

/* <ftw.h> is an XSI header */
#if __XSI_VISIBLE || __MISC_VISIBLE

struct FTW {
    int base;  /* Offset of the file name in the path */
    int level; /* Depth below the starting directory */
};

/* Values of the type argument to the nftw callback */
#define FTW_F   0 /* File */
#define FTW_D   1 /* Directory */
#define FTW_DNR 2 /* Directory that can't be read */
#define FTW_DP  3 /* Directory, with its contents already visited */
#define FTW_NS  4 /* Neither a symbolic link nor stat-able */
#define FTW_SL  5 /* Symbolic link */
#define FTW_SLN 6 /* Symbolic link to a file that doesn't exist */

/* Flags for nftw */
#define FTW_PHYS  0x01 /* Don't follow symbolic links */
#define FTW_MOUNT 0x02 /* Stay on the starting file system */
#define FTW_DEPTH 0x04 /* Visit directories after their contents */
#define FTW_CHDIR 0x08 /* Change to each directory before reading it */
#define FTW_XDEV  0x10 /* Don't descend into other file systems */

int nftw(const char *, int (*)(const char *, const struct stat *, int, struct FTW *), int, int)
    __picolibc_export;

/* The transitional large file names: struct stat is already large */
#if __LARGEFILE64_VISIBLE
#define nftw64 nftw
#endif

#endif /* __XSI_VISIBLE || __MISC_VISIBLE */

_END_STD_C

#endif /* _FTW_H_ */
