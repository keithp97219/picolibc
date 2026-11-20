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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* unlinkat removes files and, with AT_REMOVEDIR, directories */

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
    int dfd, fd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    fd = openat(dfd, "file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create file");
    check(mkdirat(dfd, "sub", 0700) == 0, "mkdirat");

    check(unlinkat(dfd, "sub", 0) == -1, "directory without AT_REMOVEDIR");
    check(unlinkat(dfd, "file", AT_REMOVEDIR) == -1 && errno == ENOTDIR, "file with AT_REMOVEDIR");
    check(unlinkat(dfd, "file", 0) == 0, "unlink file");
    check(unlinkat(dfd, "sub", AT_REMOVEDIR) == 0, "remove directory");
    check(unlinkat(dfd, "file", 0) == -1 && errno == ENOENT, "ENOENT");
    check(unlinkat(dfd, "file", AT_SYMLINK_NOFOLLOW) == -1 && errno == EINVAL, "bad flag");
    check(close(dfd) == 0 && rmdir(dir) == 0, "rmdir");
    printf("success\n");
    return 0;
}
