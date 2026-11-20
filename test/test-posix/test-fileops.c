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

/* dup3, fchdir, ftruncate, truncate, futimens and posix_getdents */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Unique for each variant of the test, which may run in parallel */
#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "FILEOPS.DIR"
#endif
#define DIR_NAME  TEST_FILE_NAME
#define FILE_NAME DIR_NAME "/file"

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
cleanup(void)
{
    (void)unlink(FILE_NAME);
    (void)unlink(DIR_NAME "/a");
    (void)unlink(DIR_NAME "/bb");
    (void)unlink(DIR_NAME "/ccc");
    (void)rmdir(DIR_NAME);
}

/* Past what a 32-bit offset can hold; files are sparse, so this costs no space */
#define BIG_SIZE ((off_t)5 * 1024 * 1024 * 1024)

int
main(void)
{
    struct stat     st;
    struct timespec times[2];
    char            cwd[256], here[256];
    int             fd, fd2, dirfd;

    cleanup();
    check(mkdir(DIR_NAME, 0700) == 0, "mkdir");
    fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "open");

    /* dup3 */
    fd2 = dup3(fd, 100, O_CLOEXEC);
    check(fd2 == 100, "dup3");
    check(fcntl(fd2, F_GETFD) & FD_CLOEXEC, "dup3 O_CLOEXEC");
    check(close(fd2) == 0, "close dup3");
    errno = 0;
    check(dup3(fd, fd, 0) == -1 && errno == EINVAL, "dup3 to the same descriptor");
    errno = 0;
    check(dup3(fd, 100, O_APPEND) == -1 && errno == EINVAL, "dup3 bad flags");

    /* ftruncate and truncate, past 32 bits */
    check(ftruncate(fd, BIG_SIZE) == 0, "ftruncate past 4 GiB");
    check(fstat(fd, &st) == 0 && st.st_size == BIG_SIZE, "size after ftruncate");
    check(truncate(FILE_NAME, 12345) == 0, "truncate");
    check(fstat(fd, &st) == 0 && st.st_size == 12345, "size after truncate");
    check(truncate(FILE_NAME, BIG_SIZE + 1) == 0, "truncate past 4 GiB");
    check(stat(FILE_NAME, &st) == 0 && st.st_size == BIG_SIZE + 1, "size after big truncate");
    check(ftruncate(fd, 0) == 0, "ftruncate to zero");

    /* futimens: a modification time past 2038, leaving the access time alone */
    check(fstat(fd, &st) == 0, "fstat before futimens");
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = (time_t)2208988800LL;
    times[1].tv_nsec = 500;
    if (sizeof(time_t) >= 8) {
        struct timespec atime = st.st_atim;

        check(futimens(fd, times) == 0, "futimens");
        check(fstat(fd, &st) == 0, "fstat after futimens");
        check(st.st_mtim.tv_sec == times[1].tv_sec && st.st_mtim.tv_nsec == 500,
              "modification time in 2040");
        check(st.st_atim.tv_sec == atime.tv_sec && st.st_atim.tv_nsec == atime.tv_nsec,
              "UTIME_OMIT kept the access time");
    }
    times[0].tv_nsec = UTIME_NOW;
    times[1].tv_nsec = UTIME_NOW;
    check(futimens(fd, times) == 0, "futimens UTIME_NOW");
    check(futimens(fd, NULL) == 0, "futimens NULL");
    check(close(fd) == 0, "close");

    /* fchdir back to where we started */
    check(getcwd(here, sizeof(here)) != NULL, "getcwd");
    dirfd = open(".", O_RDONLY | O_DIRECTORY);
    check(dirfd >= 0, "open .");
    check(chdir(DIR_NAME) == 0, "chdir");
    check(fchdir(dirfd) == 0, "fchdir");
    check(getcwd(cwd, sizeof(cwd)) != NULL && strcmp(cwd, here) == 0, "fchdir returned");
    check(close(dirfd) == 0, "close .");

#ifdef __PICOLIBC__
    /* posix_getdents (POSIX.1-2024; glibc lacks it) */
    {
        static const char * const names[] = { "a", "bb", "ccc", "file" };
        union {
            char       bytes[1024];
            __uint64_t align;
        } buf;
        ssize_t n;
        int     seen = 0;

        for (int i = 0; i < 3; i++) {
            char path[64];

            snprintf(path, sizeof(path), DIR_NAME "/%s", names[i]);
            fd = open(path, O_WRONLY | O_CREAT, 0600);
            check(fd >= 0 && close(fd) == 0, "create entry");
        }
        dirfd = open(DIR_NAME, O_RDONLY | O_DIRECTORY);
        check(dirfd >= 0, "open directory");
        errno = 0;
        check(posix_getdents(dirfd, buf.bytes, sizeof(buf.bytes), 1) == -1 && errno == EINVAL,
              "posix_getdents flags");
        errno = 0;
        check(posix_getdents(dirfd, buf.bytes, 8, 0) == -1 && errno == EINVAL,
              "posix_getdents buffer too small");
        while ((n = posix_getdents(dirfd, buf.bytes, sizeof(buf.bytes), 0)) > 0) {
            for (ssize_t off = 0; off < n;) {
                struct posix_dent *d = (struct posix_dent *)(buf.bytes + off);

                check(d->d_reclen > 0 && off + (ssize_t)d->d_reclen <= n, "d_reclen");
                check(d->d_reclen % _Alignof(struct posix_dent) == 0, "d_reclen alignment");
                for (int i = 0; i < 4; i++)
                    if (strcmp(d->d_name, names[i]) == 0) {
                        check(d->d_type == DT_REG || d->d_type == DT_UNKNOWN, "d_type");
                        seen |= 1 << i;
                    }
                off += d->d_reclen;
            }
        }
        check(n == 0, "posix_getdents end of directory");
        check(seen == 0xf, "posix_getdents found every name");
        check(close(dirfd) == 0, "close directory");
    }
#endif

    cleanup();
    printf("success\n");
    return 0;
}
