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

#ifndef _GLOB_H_
#define _GLOB_H_

#include <sys/cdefs.h>
#define __need_size_t
#include <stddef.h>

_BEGIN_STD_C

typedef struct {
    size_t gl_pathc; /* Count of paths matched by pattern */
    char **gl_pathv; /* Pointer to a list of matched pathnames */
    size_t gl_offs;  /* Slots to reserve at the beginning of gl_pathv */
} glob_t;

#define GLOB_APPEND   0x0001 /* Append to output from previous call */
#define GLOB_DOOFFS   0x0002 /* Use gl_offs */
#define GLOB_ERR      0x0004 /* Return on error */
#define GLOB_MARK     0x0008 /* Append / to matching directories */
#define GLOB_NOCHECK  0x0010 /* Return pattern itself if nothing matches */
#define GLOB_NOSORT   0x0020 /* Don't sort */
#define GLOB_NOESCAPE 0x2000 /* Backslash is an ordinary character */

#define GLOB_NOSPACE  (-1) /* Out of memory */
#define GLOB_ABORTED  (-2) /* Stopped by a read error */
#define GLOB_NOMATCH  (-3) /* No match and GLOB_NOCHECK not set */

#if __BSD_VISIBLE
#define GLOB_ABEND GLOB_ABORTED /* The BSD name */
#endif

int glob(const char * __restrict, int, int (*)(const char *, int), glob_t * __restrict)
    __picolibc_export;
void globfree(glob_t *) __picolibc_export;

/* The transitional large file names: glob uses no off_t */
#if __LARGEFILE64_VISIBLE
typedef glob_t glob64_t;
#define glob64     glob
#define globfree64 globfree
#endif

_END_STD_C

#endif /* _GLOB_H_ */
