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

#define _POSIX_C_SOURCE 202405L
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>

/* The types, structure, constants and prototypes <sys/statvfs.h> defines */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define member_is(type, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

_Static_assert(sizeof(fsblkcnt_t) > 0 && sizeof(fsfilcnt_t) > 0, "fsblkcnt_t, fsfilcnt_t");
_Static_assert((fsblkcnt_t)-1 > 0 && (fsfilcnt_t)-1 > 0, "fsblkcnt_t and fsfilcnt_t are unsigned");

_Static_assert(member_is(struct statvfs, f_bsize, unsigned long)
                   && member_is(struct statvfs, f_frsize, unsigned long)
                   && member_is(struct statvfs, f_blocks, fsblkcnt_t)
                   && member_is(struct statvfs, f_bfree, fsblkcnt_t)
                   && member_is(struct statvfs, f_bavail, fsblkcnt_t)
                   && member_is(struct statvfs, f_files, fsfilcnt_t)
                   && member_is(struct statvfs, f_ffree, fsfilcnt_t)
                   && member_is(struct statvfs, f_favail, fsfilcnt_t)
                   && member_is(struct statvfs, f_fsid, unsigned long)
                   && member_is(struct statvfs, f_flag, unsigned long)
                   && member_is(struct statvfs, f_namemax, unsigned long),
               "struct statvfs members");

/* Distinct bits of f_flag */
_Static_assert(ST_RDONLY != 0 && ST_NOSUID != 0 && (ST_RDONLY & ST_NOSUID) == 0,
               "ST_RDONLY and ST_NOSUID");

#include "../test-flags.h"

/* The functions need system support */
#if TEST_POSIX_FILE_SYSTEM_FD
#define CHECK_PROTOTYPES
static int (* const p_fstatvfs)(int, struct statvfs *) = fstatvfs;
static int (* const p_statvfs)(const char * restrict, struct statvfs * restrict) = statvfs;
#endif

int
main(void)
{
#ifdef CHECK_PROTOTYPES
    (void)p_fstatvfs;
    (void)p_statvfs;
#endif
    printf("success\n");
    return 0;
}
