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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && __PICOLIBC_POSIX_FILE_SYSTEM_FD > 0))

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* openat opens files relative to a directory descriptor */

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
    int  dfd, fd;
    char buf[4];

    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    fd = openat(dfd, "file", O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "openat create");
    check(write(fd, "abc", 3) == 3, "write");
    check(close(fd) == 0, "close");
    check(openat(dfd, "file", O_WRONLY | O_CREAT | O_EXCL, 0600) == -1 && errno == EEXIST,
          "O_EXCL");
    fd = openat(dfd, "file", O_RDONLY);
    check(fd >= 0, "openat read");
    check(read(fd, buf, sizeof(buf)) == 3 && memcmp(buf, "abc", 3) == 0, "contents");
    check(close(fd) == 0, "close");
    check(openat(dfd, "missing", O_RDONLY) == -1 && errno == ENOENT, "ENOENT");
    check(openat(dfd, "file", O_RDONLY | O_DIRECTORY) == -1 && errno == ENOTDIR, "ENOTDIR");

    /* AT_FDCWD names the working directory */
    check(chdir(dir) == 0, "chdir");
    fd = openat(AT_FDCWD, "file", O_RDONLY);
    check(fd >= 0, "openat AT_FDCWD");
    check(close(fd) == 0, "close");
    check(unlink("file") == 0, "unlink");
    check(chdir("..") == 0, "chdir ..");
    check(close(dfd) == 0, "close dir");
    check(rmdir(dir) == 0, "rmdir");

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
