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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_SYMBOLIC_LINKS && TEST_POSIX_SYMBOLIC_LINKS_FD

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * readlink and readlinkat: the contents, truncation (also to zero
 * bytes), and errors that leave the buffer unchanged
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("link");
    (void)unlink("loop");
    (void)unlink("file");
    (void)chmod("sub", 0700);
    (void)unlink("sub/l");
    (void)rmdir("sub");
    if (chdir("..") == 0)
        (void)rmdir(dir);
}

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            cleanup();                                                                 \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* An error, with the buffer left as it was */
#define check_err(expr, err, message)                                                \
    check((memset(buf, '#', sizeof(buf)), errno = 0, (expr) == -1) && errno == (err) \
              && untouched(buf, sizeof(buf)),                                        \
          message)

static int
untouched(const char *b, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
        if (b[i] != '#')
            return 0;
    return 1;
}

static char long_name[NAME_MAX + 2];

int
main(void)
{
    char buf[32];
    int  dfd, fd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    check(symlink("some/target", "link") == 0, "symlink");

    /* The contents, without a terminating null, and the count */
    memset(buf, '#', sizeof(buf));
    check(readlink("link", buf, sizeof(buf)) == 11 && memcmp(buf, "some/target", 11) == 0,
          "readlink");
    check(buf[11] == '#', "no terminating null added");

    /* A short buffer gets the first bufsize bytes */
    memset(buf, '#', sizeof(buf));
    check(readlink("link", buf, 4) == 4 && memcmp(buf, "some#", 5) == 0, "truncated");
#ifndef __GLIBC__ /* glibc passes Linux's EINVAL for a zero size through */
    memset(buf, '#', sizeof(buf));
    check(readlink("link", buf, 0) == 0 && buf[0] == '#', "zero-size buffer");
#endif

    /* Errors */
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");
    check_err(readlink("file", buf, sizeof(buf)), EINVAL, "not a symbolic link");
    check_err(readlink("file", buf, 0), EINVAL, "not a symbolic link, zero size");
    check_err(readlink("missing", buf, sizeof(buf)), ENOENT, "missing");
#ifndef __GLIBC__
    check_err(readlink("missing", buf, 0), ENOENT, "missing, zero size");
#endif
    check_err(readlink("", buf, sizeof(buf)), ENOENT, "empty path");
    check_err(readlink("file/x", buf, sizeof(buf)), ENOTDIR, "prefix not a directory");
    check_err(readlink("file/", buf, sizeof(buf)), ENOTDIR, "trailing slash on a file");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(readlink("loop/x", buf, sizeof(buf)), ELOOP, "loop");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(readlink(long_name, buf, sizeof(buf)), ENAMETOOLONG, "long component");

    /* readlinkat relative to a directory, AT_FDCWD, and its errors */
    check(mkdir("sub", 0700) == 0 && symlink("../file", "sub/l") == 0, "sub/l");
    dfd = open("sub", O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open directory");
    memset(buf, '#', sizeof(buf));
    check(readlinkat(dfd, "l", buf, sizeof(buf)) == 7 && memcmp(buf, "../file", 7) == 0,
          "readlinkat");
#ifndef __GLIBC__
    check(readlinkat(dfd, "l", buf, 0) == 0, "readlinkat zero size");
#endif
    check(readlinkat(AT_FDCWD, "link", buf, sizeof(buf)) == 11, "readlinkat AT_FDCWD");
    check_err(readlinkat(-1, "l", buf, sizeof(buf)), EBADF, "bad descriptor");
    check_err(readlinkat(fd, "l", buf, sizeof(buf)), ENOTDIR, "file descriptor");
    check(readlinkat(-1, "/nonexistent-picolibc-test", buf, sizeof(buf)) == -1 && errno == ENOENT,
          "absolute path ignores the descriptor");
    if (geteuid() != 0) {
        check(chmod("sub", 0600) == 0, "no search permission");
        check_err(readlinkat(dfd, "l", buf, sizeof(buf)), EACCES, "no search permission on fd");
        check_err(readlink("sub/l", buf, sizeof(buf)), EACCES, "no search permission in prefix");
        check(chmod("sub", 0700) == 0, "restore");
    }
    check(close(dfd) == 0 && close(fd) == 0, "close");

    cleanup();
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
