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
/* <ftw.h> is an XSI header */
#define _XOPEN_SOURCE 800
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>

/* The structure, constants and prototype <ftw.h> defines, and what it takes from <sys/stat.h> */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define member_is(type, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

_Static_assert(member_is(struct FTW, base, int) && member_is(struct FTW, level, int),
               "struct FTW members");

/* The type values, distinct (glibc defines them as enumerators, so not with #if) */
_Static_assert(FTW_F != FTW_D && FTW_F != FTW_DNR && FTW_F != FTW_DP && FTW_F != FTW_NS
                   && FTW_F != FTW_SL && FTW_F != FTW_SLN && FTW_D != FTW_DNR && FTW_D != FTW_DP
                   && FTW_D != FTW_NS && FTW_D != FTW_SL && FTW_D != FTW_SLN && FTW_DNR != FTW_DP
                   && FTW_DNR != FTW_NS && FTW_DNR != FTW_SL && FTW_DNR != FTW_SLN
                   && FTW_DP != FTW_NS && FTW_DP != FTW_SL && FTW_DP != FTW_SLN && FTW_NS != FTW_SL
                   && FTW_NS != FTW_SLN && FTW_SL != FTW_SLN,
               "FTW_ type values are distinct");

/* The flags, distinct bits */
_Static_assert(FTW_PHYS && FTW_MOUNT && FTW_DEPTH && FTW_CHDIR && !(FTW_PHYS & FTW_MOUNT)
                   && !(FTW_PHYS & FTW_DEPTH) && !(FTW_PHYS & FTW_CHDIR) && !(FTW_MOUNT & FTW_DEPTH)
                   && !(FTW_MOUNT & FTW_CHDIR) && !(FTW_DEPTH & FTW_CHDIR),
               "FTW_ flags are distinct bits");

/* FTW_XDEV is new in POSIX.1-2024; glibc 2.43 lacks it */
#ifndef __GLIBC__
_Static_assert(FTW_XDEV && !(FTW_XDEV & (FTW_PHYS | FTW_MOUNT | FTW_DEPTH | FTW_CHDIR)),
               "FTW_XDEV is a distinct bit");
#endif

/* struct stat, the st_mode names and the file type macros */
static struct stat st;
_Static_assert(S_IFMT && S_IFDIR && S_IFREG && S_IFLNK && S_IRWXU && S_IRUSR && S_IXOTH,
               "st_mode names");

/* The function needs system support */
#include "../posix-groups.h"
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM)
#define CHECK_PROTOTYPES
static int (* const p_nftw)(const char *,
                            int (*)(const char *, const struct stat *, int, struct FTW *), int, int)
    = nftw;
#endif

int
main(void)
{
    st.st_mode = S_IFDIR | S_IRWXU;
    check(S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode), "file type macros");
#ifdef CHECK_PROTOTYPES
    (void)p_nftw;
#endif
    printf("success\n");
    return 0;
}
