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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)         \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS_FD)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* utimensat sets file times, honouring UTIME_OMIT and UTIME_NOW */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

int
main(void)
{
    struct stat     st, before;
    struct timespec times[2], ctime;
    char            abs_dir[PATH_MAX], path[PATH_MAX + 8];
    long            tries = 0;
    time_t          now;
    int             dfd, fd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(getcwd(abs_dir, sizeof(abs_dir) - sizeof(dir) - 1) != NULL, "getcwd");
    strcat(abs_dir, "/");
    strcat(abs_dir, dir);
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    fd = openat(dfd, "file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create file");

    times[0].tv_sec = 1000000000;
    times[0].tv_nsec = 0;
    times[1].tv_sec = 2000000000;
    times[1].tv_nsec = 0;
    check(utimensat(dfd, "file", times, 0) == 0, "utimensat");
    check(fstatat(dfd, "file", &st, 0) == 0, "fstatat");
    check(st.st_atim.tv_sec == 1000000000 && st.st_mtim.tv_sec == 2000000000, "times set");

    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = 1500000000;
    check(utimensat(dfd, "file", times, 0) == 0, "UTIME_OMIT");
    check(fstatat(dfd, "file", &st, 0) == 0, "fstatat");
    check(st.st_atim.tv_sec == 1000000000 && st.st_mtim.tv_sec == 1500000000, "atime kept");

    check(utimensat(dfd, "file", NULL, 0) == 0, "now");
    check(fstatat(dfd, "file", &st, 0) == 0 && st.st_mtim.tv_sec > 1500000000, "mtime now");
    check(utimensat(dfd, "file", NULL, AT_REMOVEDIR) == -1 && errno == EINVAL, "bad flag");

    /* Nanoseconds are kept; tv_sec is ignored with UTIME_NOW and UTIME_OMIT */
    times[0].tv_sec = 1000000000;
    times[0].tv_nsec = 123456789;
    times[1].tv_sec = 1100000000;
    times[1].tv_nsec = 987654321;
    check(utimensat(dfd, "file", times, 0) == 0 && fstatat(dfd, "file", &st, 0) == 0,
          "nanoseconds");
    check(st.st_atim.tv_sec == 1000000000 && st.st_mtim.tv_sec == 1100000000, "seconds set");
    /* The file system may round down to its resolution */
    check(st.st_atim.tv_nsec <= 123456789 && st.st_mtim.tv_nsec <= 987654321, "never later");
    check(st.st_atim.tv_nsec >= 123000000 && st.st_mtim.tv_nsec >= 987000000,
          "at least millisecond resolution");
    times[0].tv_sec = -12345;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = -12345;
    times[1].tv_nsec = UTIME_NOW;
    now = time(NULL);
    check(utimensat(dfd, "file", times, 0) == 0 && fstatat(dfd, "file", &st, 0) == 0,
          "tv_sec ignored");
    check(st.st_atim.tv_sec == 1000000000, "UTIME_OMIT ignores tv_sec");
    check(st.st_mtim.tv_sec >= now - 60 && st.st_mtim.tv_sec <= now + 60, "UTIME_NOW is now");
    /* The change time is marked for update */
    ctime = st.st_ctim;
    times[0].tv_nsec = 0;
    times[1].tv_sec = 1200000000;
    times[1].tv_nsec = 0;
    do {
        times[0].tv_sec++;
        check(utimensat(dfd, "file", times, 0) == 0 && fstatat(dfd, "file", &st, 0) == 0,
              "set again");
    } while (st.st_ctim.tv_sec == ctime.tv_sec && st.st_ctim.tv_nsec == ctime.tv_nsec
             && ++tries < 1000000);
    check(st.st_ctim.tv_sec > ctime.tv_sec
              || (st.st_ctim.tv_sec == ctime.tv_sec && st.st_ctim.tv_nsec > ctime.tv_nsec),
          "st_ctim advanced");

    /* Invalid nanoseconds fail and change nothing */
    check(fstatat(dfd, "file", &before, 0) == 0, "fstatat before");
    times[0].tv_sec = 1;
    times[0].tv_nsec = 1000000000;
    times[1].tv_sec = 1;
    times[1].tv_nsec = 0;
    errno = 0;
    check(utimensat(dfd, "file", times, 0) == -1 && errno == EINVAL, "EINVAL 1000000000");
    times[0].tv_nsec = 0;
    times[1].tv_nsec = -1;
    errno = 0;
    check(utimensat(dfd, "file", times, 0) == -1 && errno == EINVAL, "EINVAL negative");
    check(fstatat(dfd, "file", &st, 0) == 0 && st.st_atim.tv_sec == before.st_atim.tv_sec
              && st.st_mtim.tv_sec == before.st_mtim.tv_sec,
          "a failed call changes nothing");

    /* AT_SYMLINK_NOFOLLOW changes the link, not the target */
    check(symlinkat("file", dfd, "link") == 0, "symlinkat");
    times[0].tv_sec = 1300000000;
    times[0].tv_nsec = 0;
    times[1].tv_sec = 1400000000;
    times[1].tv_nsec = 0;
    check(utimensat(dfd, "link", times, AT_SYMLINK_NOFOLLOW) == 0, "AT_SYMLINK_NOFOLLOW");
    check(fstatat(dfd, "link", &st, AT_SYMLINK_NOFOLLOW) == 0 && st.st_mtim.tv_sec == 1400000000
              && st.st_atim.tv_sec == 1300000000,
          "link times");
    check(fstatat(dfd, "file", &st, 0) == 0 && st.st_mtim.tv_sec == before.st_mtim.tv_sec,
          "target unchanged");
    /* Without the flag the target changes */
    check(utimensat(dfd, "link", times, 0) == 0 && fstatat(dfd, "file", &st, 0) == 0
              && st.st_mtim.tv_sec == 1400000000,
          "the link is followed");
    check(unlinkat(dfd, "link", 0) == 0, "unlink link");

    /* AT_FDCWD and absolute paths */
    check(fchdir(dfd) == 0, "fchdir");
    times[1].tv_sec = 1500000000;
    check(utimensat(AT_FDCWD, "file", times, 0) == 0, "AT_FDCWD");
    check(chdir("/") == 0, "chdir /");
    check(fstatat(dfd, "file", &st, 0) == 0 && st.st_mtim.tv_sec == 1500000000, "AT_FDCWD times");
    snprintf(path, sizeof(path), "%s/file", abs_dir);
    times[1].tv_sec = 1600000000;
    check(utimensat(-1, path, times, 0) == 0 && fstatat(dfd, "file", &st, 0) == 0
              && st.st_mtim.tv_sec == 1600000000,
          "an absolute path ignores the descriptor");

    /* Permissions: only the owner may set explicit times; both UTIME_OMIT needs nothing */
    if (geteuid() != 0 && stat("/", &st) == 0 && st.st_uid != geteuid()) {
        errno = 0;
        check(utimensat(AT_FDCWD, "/", times, 0) == -1 && errno == EPERM, "EPERM");
        times[0].tv_nsec = UTIME_NOW;
        times[1].tv_nsec = UTIME_NOW;
        if (access("/", W_OK) != 0) {
            errno = 0;
            check(utimensat(AT_FDCWD, "/", times, 0) == -1 && errno == EACCES,
                  "EACCES for UTIME_NOW");
            errno = 0;
            check(utimensat(AT_FDCWD, "/", NULL, 0) == -1 && errno == EACCES, "EACCES for NULL");
        }
        times[0].tv_nsec = UTIME_OMIT;
        times[1].tv_nsec = UTIME_OMIT;
        check(utimensat(AT_FDCWD, "/", times, 0) == 0, "UTIME_OMIT needs no permission");
    }

    /* Errors */
    times[0].tv_nsec = 0;
    times[1].tv_nsec = 0;
    errno = 0;
    check(utimensat(dfd, "", times, 0) == -1 && errno == ENOENT, "ENOENT empty path");
    errno = 0;
    check(utimensat(dfd, "missing", times, 0) == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(utimensat(dfd, "file/", times, 0) == -1 && errno == ENOTDIR, "ENOTDIR trailing slash");
    errno = 0;
    check(utimensat(-1, "file", times, 0) == -1 && errno == EBADF, "EBADF");
    fd = openat(dfd, "file", O_RDONLY);
    check(fd >= 0, "open file");
    errno = 0;
    check(utimensat(fd, "file", times, 0) == -1 && errno == ENOTDIR, "ENOTDIR descriptor");
    check(close(fd) == 0, "close");

    check(unlinkat(dfd, "file", 0) == 0, "cleanup");
    check(close(dfd) == 0 && rmdir(abs_dir) == 0, "rmdir");
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
