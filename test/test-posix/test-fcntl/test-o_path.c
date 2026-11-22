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

/* O_PATH is an extension (Linux, FreeBSD 14) */
#define _GNU_SOURCE
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_FD && TEST_POSIX_FD_MGMT \
    && TEST_POSIX_SYMBOLIC_LINKS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * O_PATH: a descriptor that names a file without opening it for
 * reading or writing. fstat works, read and write fail with EBADF,
 * F_GETFL reports O_PATH, a directory serves openat and fchdir, and
 * with O_NOFOLLOW a symbolic link names the link itself.
 */

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

int
main(void)
{
#ifdef O_PATH
    static char dir[] = "o_path-XXXXXX";
    static char cwd[4096], sub[4096];
    struct stat st;
    char        c;
    int         fd, dfd, ffd, top;

    check((O_PATH & O_ACCMODE) == 0, "O_PATH isn't an access mode");
    check(getcwd(cwd, sizeof(cwd)) != NULL, "getcwd");
    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(chdir(dir) == 0, "chdir");
    fd = open("file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && write(fd, "x", 1) == 1 && close(fd) == 0, "create file");
    check(symlink("file", "link") == 0, "symlink");
    check(chdir(cwd) == 0, "chdir back");

    /* A file */
    check(snprintf(sub, sizeof(sub), "%s/file", dir) > 0, "name");
    fd = open(sub, O_PATH);
    check(fd >= 0, "open file O_PATH");
    check(fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size == 1, "fstat file");
    check_err(read(fd, &c, 1), EBADF, "read");
    check_err(write(fd, &c, 1), EBADF, "write");
    check((fcntl(fd, F_GETFL) & O_PATH) != 0, "F_GETFL reports O_PATH");
    check(close(fd) == 0, "close");

    /* A directory: openat and fchdir */
    dfd = open(dir, O_PATH | O_DIRECTORY);
    check(dfd >= 0, "open directory O_PATH");
    check(fstat(dfd, &st) == 0 && S_ISDIR(st.st_mode), "fstat directory");
    check((fcntl(dfd, F_GETFL) & O_PATH) != 0, "F_GETFL reports O_PATH");
    check_err(read(dfd, &c, 1), EBADF, "read directory");
    ffd = openat(dfd, "file", O_RDONLY);
    check(ffd >= 0, "openat relative to an O_PATH directory");
    check(read(ffd, &c, 1) == 1 && c == 'x', "read through openat");
    check(close(ffd) == 0, "close");
    top = open(".", O_RDONLY | O_DIRECTORY);
    check(top >= 0, "open the working directory");
    check(fchdir(dfd) == 0, "fchdir to an O_PATH directory");
    check(getcwd(sub, sizeof(sub)) != NULL && strlen(sub) > strlen(dir)
              && strcmp(sub + strlen(sub) - strlen(dir), dir) == 0,
          "in the directory");
    check(fchdir(top) == 0 && close(top) == 0, "fchdir back");

    /* A symbolic link itself */
    fd = openat(dfd, "link", O_PATH | O_NOFOLLOW);
    check(fd >= 0, "open a symbolic link O_PATH | O_NOFOLLOW");
    check(fstat(fd, &st) == 0 && S_ISLNK(st.st_mode), "fstat the link");
    check(close(fd) == 0, "close");

#if defined(__linux__) && defined(__PICOLIBC__)
    /*
     * picolibc on Linux changes the file an O_SEARCH descriptor names
     * through /proc/self/fd; Linux O_PATH descriptors, which O_PATH and
     * O_SEARCH both give, get the same
     */
    check(fchmod(dfd, 0750) == 0, "fchmod");
    check(stat(dir, &st) == 0 && (st.st_mode & 0777) == 0750, "mode changed");
#endif
    check(close(dfd) == 0, "close");

    check(chdir(dir) == 0 && unlink("link") == 0 && unlink("file") == 0, "cleanup files");
    check(chdir(cwd) == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
#else
    printf("no O_PATH, skipping\n");
    return 77;
#endif
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
