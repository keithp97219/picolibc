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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_ATTRIBUTES && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_FD \
    && TEST_POSIX_SYMBOLIC_LINKS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* chmod sets a file's permission bits and reports why it can't */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char            dir[] = "chmod-XXXXXX";
    char            file[64], missing[64], notdir[64];
    struct stat     st;
    struct timespec times[2], ctime;
    long            tries = 0;
    int             fd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(file, sizeof(file), "%s/file", dir);
    snprintf(missing, sizeof(missing), "%s/missing", dir);
    snprintf(notdir, sizeof(notdir), "%s/file/x", dir);
    fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0 && close(fd) == 0, "create");

    check(chmod(file, 0640) == 0, "chmod");
    check(stat(file, &st) == 0 && (st.st_mode & 07777) == 0640, "mode 0640");
    check(chmod(file, 0) == 0, "chmod 0");
    check(stat(file, &st) == 0 && (st.st_mode & 07777) == 0, "mode 0");
    /* The file type bits are untouched */
    check(S_ISREG(st.st_mode), "still a regular file");
    check(chmod(dir, 0755) == 0 && stat(dir, &st) == 0 && S_ISDIR(st.st_mode)
              && (st.st_mode & 0777) == 0755,
          "directory");

    /* S_ISUID, S_ISGID and S_ISVTX can be set; S_ISGID needs the file's group */
    check(stat(file, &st) == 0, "stat for the group");
    check(chmod(file, S_ISUID | 0700) == 0 && stat(file, &st) == 0
              && (st.st_mode & 07777) == (S_ISUID | 0700),
          "S_ISUID");
    if (st.st_gid == getegid()) {
        check(chmod(file, S_ISGID | 0750) == 0 && stat(file, &st) == 0
                  && (st.st_mode & 07777) == (S_ISGID | 0750),
              "S_ISGID");
    }
    check(chmod(dir, S_ISVTX | 0700) == 0 && stat(dir, &st) == 0
              && (st.st_mode & 07777) == (S_ISVTX | 0700),
          "S_ISVTX on a directory");
    check(chmod(dir, 0700) == 0, "directory mode back");

    /* File type bits in mode are ignored */
    check(chmod(file, S_IFDIR | 0604) == 0, "file type bits");
    check(stat(file, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 07777) == 0604,
          "file type bits ignored");

    /* The change time is marked for update */
    times[0].tv_sec = 1000000000;
    times[0].tv_nsec = 0;
    times[1] = times[0];
    check(utimensat(AT_FDCWD, file, times, 0) == 0 && stat(file, &st) == 0, "set times");
    ctime = st.st_ctim;
    do {
        check(chmod(file, 0600) == 0 && stat(file, &st) == 0, "chmod again");
        check(chmod(file, 0640) == 0 && stat(file, &st) == 0, "chmod again");
    } while (st.st_ctim.tv_sec == ctime.tv_sec && st.st_ctim.tv_nsec == ctime.tv_nsec
             && ++tries < 1000000);
    check(st.st_ctim.tv_sec > ctime.tv_sec
              || (st.st_ctim.tv_sec == ctime.tv_sec && st.st_ctim.tv_nsec > ctime.tv_nsec),
          "st_ctim advanced");
    check(st.st_mtim.tv_sec == 1000000000, "st_mtim unchanged");

    /* Only the owner can change the mode; root can change anything */
    if (geteuid() != 0) {
        check(stat("/", &st) == 0, "stat /");
        if (st.st_uid != geteuid()) {
            errno = 0;
            check(chmod("/", st.st_mode & 07777) == -1 && errno == EPERM, "EPERM");
        }
        /* Search permission on the path prefix */
        check(chmod(dir, 0600) == 0, "remove search permission");
        errno = 0;
        check(chmod(file, 0600) == -1 && errno == EACCES, "EACCES");
        check(chmod(dir, 0700) == 0, "restore search permission");
    }
    snprintf(notdir, sizeof(notdir), "%s/file/", dir);
    errno = 0;
    check(chmod(notdir, 0600) == -1 && errno == ENOTDIR, "ENOTDIR trailing slash");
    snprintf(notdir, sizeof(notdir), "%s/loop", dir);
    check(symlink("loop", notdir) == 0, "symlink loop");
    errno = 0;
    check(chmod(notdir, 0600) == -1 && errno == ELOOP, "ELOOP");
    check(unlink(notdir) == 0, "unlink loop");
    snprintf(notdir, sizeof(notdir), "%s/file/x", dir);

    errno = 0;
    check(chmod(missing, 0600) == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(chmod("", 0600) == -1 && errno == ENOENT, "empty path");
    errno = 0;
    check(chmod(notdir, 0600) == -1 && errno == ENOTDIR, "ENOTDIR");

    check(unlink(file) == 0 && rmdir(dir) == 0, "clean up");
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
