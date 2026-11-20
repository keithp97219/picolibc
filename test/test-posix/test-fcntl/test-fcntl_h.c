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
/* S_ISVTX is an XSI symbol */
#define _XOPEN_SOURCE 800
#include <fcntl.h>

/* Checked before any other header could define them */
#if !defined(SEEK_SET) || !defined(SEEK_CUR) || !defined(SEEK_END)
#error <fcntl.h> must define SEEK_SET, SEEK_CUR and SEEK_END
#endif
#if !defined(S_IRWXU) || !defined(S_IRUSR) || !defined(S_IWUSR) || !defined(S_IXUSR)    \
    || !defined(S_IRWXG) || !defined(S_IRGRP) || !defined(S_IWGRP) || !defined(S_IXGRP) \
    || !defined(S_IRWXO) || !defined(S_IROTH) || !defined(S_IWOTH) || !defined(S_IXOTH) \
    || !defined(S_ISUID) || !defined(S_ISGID) || !defined(S_ISVTX)
#error <fcntl.h> must define the file mode constants
#endif
#if defined(__PICOLIBC__) && !defined(O_TTY_INIT)
/* glibc doesn't define O_TTY_INIT */
#error <fcntl.h> must define O_TTY_INIT
#endif

#include <stdio.h>
#include <stdlib.h>

/* The types, constants, structures and prototypes <fcntl.h> defines */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define member_is(type, m, t) _Generic(((type *)0)->m, t: 1, default: 0)

_Static_assert(sizeof(mode_t) > 0 && sizeof(off_t) > 0 && sizeof(pid_t) > 0,
               "mode_t, off_t, pid_t");

/* struct flock */
_Static_assert(member_is(struct flock, l_type, short) && member_is(struct flock, l_whence, short)
                   && member_is(struct flock, l_start, off_t)
                   && member_is(struct flock, l_len, off_t)
                   && member_is(struct flock, l_pid, pid_t),
               "struct flock members");

/* The fcntl commands, usable in #if */
#if F_DUPFD < 0 || F_DUPFD_CLOEXEC < 0 || F_GETFD < 0 || F_SETFD < 0 || F_GETFL < 0 || F_SETFL < 0 \
    || F_GETLK < 0 || F_SETLK < 0 || F_SETLKW < 0 || F_GETOWN < 0 || F_SETOWN < 0
#error fcntl commands
#endif
static const int commands[] = {
    F_DUPFD,
    F_DUPFD_CLOEXEC,
    F_GETFD,
    F_SETFD,
    F_GETFL,
    F_SETFL,
    F_GETLK,
    F_SETLK,
    F_SETLKW,
    F_GETOWN,
    F_SETOWN,
#ifndef __GLIBC__
    /* glibc has these only for _GNU_SOURCE */
    F_OFD_GETLK,
    F_OFD_SETLK,
    F_OFD_SETLKW,
    F_GETOWN_EX,
    F_SETOWN_EX,
#endif
};

#ifdef __PICOLIBC__
/* struct f_owner_ex and its type values; glibc has them only for _GNU_SOURCE */
_Static_assert(member_is(struct f_owner_ex, type, int) && member_is(struct f_owner_ex, pid, pid_t),
               "struct f_owner_ex members");
_Static_assert(F_OWNER_PID != F_OWNER_PGRP, "F_OWNER_PID and F_OWNER_PGRP");
#endif

/* The descriptor flag, usable in #if */
#if FD_CLOEXEC == 0
#error FD_CLOEXEC
#endif

/* The lock types, usable in #if */
#if F_RDLCK == F_UNLCK || F_RDLCK == F_WRLCK || F_UNLCK == F_WRLCK
#error lock types
#endif

/* The l_whence values */
_Static_assert(SEEK_SET != SEEK_CUR && SEEK_SET != SEEK_END && SEEK_CUR != SEEK_END,
               "SEEK_SET, SEEK_CUR, SEEK_END");

/* The file creation flags, usable in #if */
#if O_CLOEXEC == 0 || O_CREAT == 0 || O_DIRECTORY == 0 || O_EXCL == 0 || O_NOCTTY == 0 \
    || O_NOFOLLOW == 0 || O_TRUNC == 0
#error file creation flags
#endif
static const int creation_flags[] = {
    O_CLOEXEC, O_CREAT, O_DIRECTORY, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_TRUNC,
};

#ifndef __GLIBC__
/* O_TTY_INIT may be zero; otherwise it is bitwise-distinct from the other flags */
#if O_TTY_INIT != 0                                                                        \
    && ((O_TTY_INIT                                                                        \
         & (O_CLOEXEC | O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY | O_NOFOLLOW | O_TRUNC)) \
        != 0)
#error O_TTY_INIT
#endif
#endif

/* The file status flags, usable in #if */
#if O_APPEND == 0 || O_DSYNC == 0 || O_NONBLOCK == 0 || O_RSYNC == 0 || O_SYNC == 0
#error file status flags
#endif
static const int status_flags[] = { O_APPEND, O_NONBLOCK, O_SYNC };

/* The file access modes and their mask, usable in #if */
#if O_ACCMODE == 0 || O_RDONLY == O_WRONLY || O_RDONLY == O_RDWR || O_WRONLY == O_RDWR
#error file access modes
#endif
static const int access_modes[] = {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
#ifndef __GLIBC__
    /* glibc doesn't define O_EXEC or O_SEARCH; they may be equal */
    O_EXEC,
#endif
};

/* The file mode constants from <sys/stat.h> */
_Static_assert(S_IRWXU == 0700 && S_IRUSR == 0400 && S_IWUSR == 0200 && S_IXUSR == 0100
                   && S_IRWXG == 070 && S_IRGRP == 040 && S_IWGRP == 020 && S_IXGRP == 010
                   && S_IRWXO == 07 && S_IROTH == 04 && S_IWOTH == 02 && S_IXOTH == 01
                   && S_ISUID == 04000 && S_ISGID == 02000 && S_ISVTX == 01000,
               "file mode constants");

/* The *at constants */
static const int at_flags[] = { AT_SYMLINK_NOFOLLOW, AT_SYMLINK_FOLLOW, AT_REMOVEDIR };
_Static_assert(AT_EACCESS != 0 && AT_SYMLINK_NOFOLLOW != 0 && AT_SYMLINK_FOLLOW != 0
                   && AT_REMOVEDIR != 0,
               "AT_ flags");

/* The posix_fadvise advice */
static const int advice[] = {
    POSIX_FADV_DONTNEED, POSIX_FADV_NOREUSE,    POSIX_FADV_NORMAL,
    POSIX_FADV_RANDOM,   POSIX_FADV_SEQUENTIAL, POSIX_FADV_WILLNEED,
};

#include <unistd.h>

/* The functions need system support */
#if !defined(__PICOLIBC__)                                                                \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && __PICOLIBC_POSIX_FILE_SYSTEM_FD > 0) \
        && (defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)            \
        && (defined(_POSIX_ADVISORY_INFO) && _POSIX_ADVISORY_INFO > 0))
#define CHECK_PROTOTYPES

/* The prototypes */
static int                (               *const p_creat)(const char *, mode_t) = creat;
static int                (               *const p_fcntl)(int, int, ...) = fcntl;
static int                (               *const p_open)(const char *, int, ...) = open;
static int                (               *const p_openat)(int, const char *, int, ...) = openat;
static int                (               *const p_posix_fadvise)(int, off_t, off_t, int) = posix_fadvise;
static int                (               *const p_posix_fallocate)(int, off_t, off_t) = posix_fallocate;

static const void * const prototypes[] = {
    &p_creat, &p_fcntl, &p_open, &p_openat, &p_posix_fadvise, &p_posix_fallocate,
};
#endif

static int
distinct(const int *values, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (values[i] == values[j])
                return 0;
    return 1;
}

/* Each value has bits that none of the others has */
static int
bitwise_distinct(const int *values, size_t n)
{
    size_t i, j;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            if (i != j && (values[i] & values[j]) != 0)
                return 0;
    return 1;
}

#define COUNT(a)            (sizeof(a) / sizeof(a[0]))
#define DISTINCT(a)         distinct(a, COUNT(a))
#define BITWISE_DISTINCT(a) bitwise_distinct(a, COUNT(a))

int
main(void)
{
    size_t i;

#ifdef CHECK_PROTOTYPES
    (void)prototypes;
#endif

    check(DISTINCT(commands), "fcntl commands are unique");
    check(BITWISE_DISTINCT(creation_flags), "file creation flags are bitwise-distinct");
    check(BITWISE_DISTINCT(status_flags), "O_APPEND, O_NONBLOCK and O_SYNC are bitwise-distinct");
    for (i = 0; i < COUNT(creation_flags); i++)
        check((creation_flags[i] & (O_APPEND | O_NONBLOCK | O_SYNC | O_ACCMODE)) == 0,
              "creation flags don't overlap the status flags and access modes");
    check(DISTINCT(access_modes), "file access modes are unique");
    for (i = 0; i < COUNT(access_modes); i++)
        check((access_modes[i] & O_ACCMODE) == access_modes[i],
              "O_ACCMODE covers the access modes");
#ifndef __GLIBC__
    check((O_SEARCH & O_ACCMODE) == O_SEARCH, "O_ACCMODE covers O_SEARCH");
#endif
    check(DISTINCT(at_flags), "AT_ flags are distinct");
    check(AT_FDCWD < 0, "AT_FDCWD isn't a descriptor");
    check(DISTINCT(advice), "advice values are distinct");
    printf("success\n");
    return 0;
}
