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

#define _GNU_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "GETDENTS.DIR"
#endif

/* posix_getdents returns aligned records for every entry */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifdef __PICOLIBC__
static void
cleanup(void)
{
    (void)unlink(TEST_FILE_NAME "/a");
    (void)unlink(TEST_FILE_NAME "/bb");
    (void)unlink(TEST_FILE_NAME "/ccc");
    (void)rmdir(TEST_FILE_NAME);
}
#endif

int
main(void)
{
#ifndef __PICOLIBC__
    /* POSIX.1-2024; glibc lacks it */
    printf("no posix_getdents, skipping\n");
    return 77;
#else
    static const char * const names[] = { "a", "bb", "ccc" };
    union {
        char       bytes[1024];
        __uint64_t align;
    } buf;
    ssize_t n;
    int     seen = 0;
    int     fd, dirfd;

    cleanup();
    check(mkdir(TEST_FILE_NAME, 0700) == 0, "mkdir");
    for (int i = 0; i < 3; i++) {
        char path[64];

        snprintf(path, sizeof(path), TEST_FILE_NAME "/%s", names[i]);
        fd = open(path, O_WRONLY | O_CREAT, 0600);
        check(fd >= 0 && close(fd) == 0, "create entry");
    }
    dirfd = open(TEST_FILE_NAME, O_RDONLY | O_DIRECTORY);
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
            for (int i = 0; i < 3; i++)
                if (strcmp(d->d_name, names[i]) == 0) {
                    check(d->d_type == DT_REG || d->d_type == DT_UNKNOWN, "d_type");
                    seen |= 1 << i;
                }
            off += d->d_reclen;
        }
    }
    check(n == 0, "posix_getdents end of directory");
    check(seen == 0x7, "posix_getdents found every name");
    check(close(dirfd) == 0, "close directory");
    cleanup();
#endif

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
