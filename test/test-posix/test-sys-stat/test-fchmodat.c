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
#if TEST_POSIX_FILE_ATTRIBUTES_FD && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT \
    && TEST_POSIX_FILE_SYSTEM_FD && TEST_POSIX_SYMBOLIC_LINKS_FD

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* fchmodat changes modes relative to a directory descriptor */

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
    struct stat st;
    char        abs_dir[PATH_MAX], path[PATH_MAX + 8];
    int         dfd, fd, ret;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(getcwd(abs_dir, sizeof(abs_dir) - sizeof(dir) - 1) != NULL, "getcwd");
    strcat(abs_dir, "/");
    strcat(abs_dir, dir);
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    fd = openat(dfd, "file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create file");
    check(fchmodat(dfd, "file", 0640, 0) == 0, "fchmodat");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0640, "mode");
    check(fchmodat(dfd, "missing", 0640, 0) == -1 && errno == ENOENT, "ENOENT");

    /* Symbolic links may not support modes at all */
    check(symlinkat("file", dfd, "link") == 0, "symlinkat");
    ret = fchmodat(dfd, "link", 0600, AT_SYMLINK_NOFOLLOW);
    /* Linux has a single value for EOPNOTSUPP and ENOTSUP */
    check(ret == 0 || errno == EOPNOTSUPP || errno == ENOTSUP, "AT_SYMLINK_NOFOLLOW");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0640, "target unchanged");

    /* AT_SYMLINK_NOFOLLOW changes the mode of anything but a symbolic link */
    check(fchmodat(dfd, "file", 0604, AT_SYMLINK_NOFOLLOW) == 0, "AT_SYMLINK_NOFOLLOW on a file");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0604, "file mode changed");
    check(mkdirat(dfd, "sub", 0700) == 0, "mkdirat");
    check(fchmodat(dfd, "sub", 0750, AT_SYMLINK_NOFOLLOW) == 0,
          "AT_SYMLINK_NOFOLLOW on a directory");
    check(fstatat(dfd, "sub", &st, 0) == 0 && (st.st_mode & 07777) == 0750, "directory mode");
    errno = 0;
    check(fchmodat(dfd, "missing", 0600, AT_SYMLINK_NOFOLLOW) == -1 && errno == ENOENT,
          "AT_SYMLINK_NOFOLLOW ENOENT");
    check(unlinkat(dfd, "sub", AT_REMOVEDIR) == 0, "remove sub");

    /* AT_FDCWD and absolute paths */
    check(fchdir(dfd) == 0, "fchdir");
    check(fchmodat(AT_FDCWD, "file", 0600, 0) == 0, "AT_FDCWD");
    check(chdir("/") == 0, "chdir /");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0600, "AT_FDCWD mode");
    snprintf(path, sizeof(path), "%s/file", abs_dir);
    check(fchmodat(-1, path, 0640, 0) == 0, "absolute path ignores the descriptor");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0640, "absolute mode");

    /* Errors */
    errno = 0;
    check(fchmodat(-1, "file", 0600, 0) == -1 && errno == EBADF, "EBADF");
    fd = openat(dfd, "file", O_RDONLY);
    check(fd >= 0, "open file");
    errno = 0;
    check(fchmodat(fd, "file", 0600, 0) == -1 && errno == ENOTDIR, "ENOTDIR descriptor");
    check(close(fd) == 0, "close file");
    errno = 0;
    check(fchmodat(dfd, "file", 0600, AT_REMOVEDIR) == -1 && errno == EINVAL, "EINVAL flag");
    check(fstatat(dfd, "file", &st, 0) == 0 && (st.st_mode & 07777) == 0640,
          "a failed call changes nothing");
    errno = 0;
    check(fchmodat(dfd, "", 0600, 0) == -1 && errno == ENOENT, "ENOENT empty path");

    check(unlinkat(dfd, "link", 0) == 0 && unlinkat(dfd, "file", 0) == 0, "cleanup");
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
