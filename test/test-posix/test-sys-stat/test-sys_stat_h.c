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
/* mknod is an XSI interface */
#define _XOPEN_SOURCE 800
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* The types, structure, constants, macros and prototypes <sys/stat.h> defines */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define member_is(type, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

_Static_assert(sizeof(blkcnt_t) > 0 && sizeof(blksize_t) > 0 && sizeof(dev_t) > 0
                   && sizeof(ino_t) > 0 && sizeof(mode_t) > 0 && sizeof(nlink_t) > 0
                   && sizeof(uid_t) > 0 && sizeof(gid_t) > 0 && sizeof(off_t) > 0
                   && sizeof(time_t) > 0 && sizeof(struct timespec) > 0,
               "the types");

/* struct stat */
_Static_assert(member_is(struct stat, st_dev, dev_t) && member_is(struct stat, st_ino, ino_t)
                   && member_is(struct stat, st_mode, mode_t)
                   && member_is(struct stat, st_nlink, nlink_t)
                   && member_is(struct stat, st_uid, uid_t) && member_is(struct stat, st_gid, gid_t)
                   && member_is(struct stat, st_rdev, dev_t)
                   && member_is(struct stat, st_size, off_t)
                   && member_is(struct stat, st_atim, struct timespec)
                   && member_is(struct stat, st_mtim, struct timespec)
                   && member_is(struct stat, st_ctim, struct timespec)
                   && member_is(struct stat, st_blksize, blksize_t)
                   && member_is(struct stat, st_blocks, blkcnt_t),
               "struct stat members");

/* The file types, usable in #if */
#if S_IFMT == 0 || S_IFBLK == 0 || S_IFCHR == 0 || S_IFIFO == 0 || S_IFREG == 0 || S_IFDIR == 0 \
    || S_IFLNK == 0 || S_IFSOCK == 0
#error file types
#endif
static const mode_t types[] = { S_IFBLK, S_IFCHR, S_IFIFO, S_IFREG, S_IFDIR, S_IFLNK, S_IFSOCK };

/* The file mode bits, with their values, usable in #if */
#if S_IRWXU != 0700 || S_IRUSR != 0400 || S_IWUSR != 0200 || S_IXUSR != 0100 || S_IRWXG != 070    \
    || S_IRGRP != 040 || S_IWGRP != 020 || S_IXGRP != 010 || S_IRWXO != 07 || S_IROTH != 04       \
    || S_IWOTH != 02 || S_IXOTH != 01 || S_ISUID != 04000 || S_ISGID != 02000 || S_ISVTX != 01000
#error file mode bits
#endif

/* The UTIME_ values are distinct and outside [0, 999999999] */
_Static_assert(UTIME_NOW != UTIME_OMIT, "UTIME_NOW and UTIME_OMIT are distinct");
_Static_assert((UTIME_NOW < 0 || UTIME_NOW > 999999999)
                   && (UTIME_OMIT < 0 || UTIME_OMIT > 999999999),
               "UTIME_NOW and UTIME_OMIT are outside the nanosecond range");

#include "../posix-groups.h"

/* The functions need system support */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && defined(__PICOLIBC_POSIX_FILE_ATTRIBUTES_FD) \
    && defined(__PICOLIBC_POSIX_FIFO_FD)
#define CHECK_PROTOTYPES

/* The prototypes */
static int (* const p_chmod)(const char *, mode_t) = chmod;
static int (* const p_fchmod)(int, mode_t) = fchmod;
static int (* const p_fchmodat)(int, const char *, mode_t, int) = fchmodat;
static int (* const p_fstat)(int, struct stat *) = fstat;
static int (* const p_fstatat)(int, const char * restrict, struct stat * restrict, int) = fstatat;
static int (* const p_futimens)(int, const struct timespec[2]) = futimens;
static int (* const p_lstat)(const char * restrict, struct stat * restrict) = lstat;
static int (* const p_mkdir)(const char *, mode_t) = mkdir;
static int (* const p_mkdirat)(int, const char *, mode_t) = mkdirat;
static int (* const p_mkfifo)(const char *, mode_t) = mkfifo;
static int (* const p_mkfifoat)(int, const char *, mode_t) = mkfifoat;
static int (* const p_mknod)(const char *, mode_t, dev_t) = mknod;
static int (* const p_mknodat)(int, const char *, mode_t, dev_t) = mknodat;
static int (* const p_stat)(const char * restrict, struct stat * restrict) = stat;
static mode_t (* const p_umask)(mode_t) = umask;
static int    (   *const p_utimensat)(int, const char *, const struct timespec[2], int) = utimensat;

static const void * const prototypes[] = {
    &p_chmod,   &p_fchmod, &p_fchmodat, &p_fstat, &p_fstatat, &p_futimens, &p_lstat, &p_mkdir,
    &p_mkdirat, &p_mkfifo, &p_mkfifoat, &p_mknod, &p_mknodat, &p_stat,     &p_umask, &p_utimensat,
};
#endif

int
main(void)
{
    struct stat st = { 0 };
    size_t      i, j;
    /* A file mode bit expression is a valid open() mode without a cast */
    int         mode = S_IRUSR | S_IWUSR | S_IRGRP;

#ifdef CHECK_PROTOTYPES
    (void)prototypes;
#endif
    (void)mode;

    for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        check((types[i] & ~S_IFMT) == 0, "file types are in S_IFMT");
        for (j = 0; j < sizeof(types) / sizeof(types[0]); j++)
            check(i == j || types[i] != types[j], "file types are distinct");
        check((types[i] & 07777) == 0, "file types don't overlap the mode bits");
    }

    /* Each S_IS macro is true for its own type only */
    for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        mode_t m = types[i] | 0644;

        check(!S_ISBLK(m) == (types[i] != S_IFBLK), "S_ISBLK");
        check(!S_ISCHR(m) == (types[i] != S_IFCHR), "S_ISCHR");
        check(!S_ISFIFO(m) == (types[i] != S_IFIFO), "S_ISFIFO");
        check(!S_ISREG(m) == (types[i] != S_IFREG), "S_ISREG");
        check(!S_ISDIR(m) == (types[i] != S_IFDIR), "S_ISDIR");
        check(!S_ISLNK(m) == (types[i] != S_IFLNK), "S_ISLNK");
        check(!S_ISSOCK(m) == (types[i] != S_IFSOCK), "S_ISSOCK");
    }

    /* Neither implementation makes these distinct file types */
    st.st_mode = S_IFREG | 0600;
    check(S_TYPEISMQ(&st) == 0, "S_TYPEISMQ");
    check(S_TYPEISSEM(&st) == 0, "S_TYPEISSEM");
    check(S_TYPEISSHM(&st) == 0, "S_TYPEISSHM");

    /* The compatibility macros name the seconds of the timestamps */
    st.st_atim.tv_sec = 1;
    st.st_mtim.tv_sec = 2;
    st.st_ctim.tv_sec = 3;
    check(st.st_atime == 1 && st.st_mtime == 2 && st.st_ctime == 3,
          "st_atime, st_mtime and st_ctime");
    check(&st.st_atime == &st.st_atim.tv_sec, "st_atime is st_atim.tv_sec");

    printf("success\n");
    return 0;
}
