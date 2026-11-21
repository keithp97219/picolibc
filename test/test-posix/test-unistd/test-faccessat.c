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
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)      \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * access and faccessat: each permission against the mode bits, the
 * existence test, AT_EACCESS, the directory descriptor and the errors
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    static const char * const names[] = { "r", "w", "x", "none", "loop", "sub/f" };
    unsigned                  i;

    (void)chmod("sub", 0700);
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        (void)unlink(names[i]);
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

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

static void
make(const char *name, mode_t mode)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0600);

    check(fd >= 0 && close(fd) == 0 && chmod(name, mode) == 0, "create");
}

static char long_name[NAME_MAX + 2];

int
main(void)
{
    int root = geteuid() == 0;
    int dfd, fd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    make("r", 0400);
    make("w", 0200);
    make("x", 0100);
    make("none", 0000);

    /* F_OK only asks whether the file exists */
    check(access("none", F_OK) == 0, "F_OK on a file with no permissions");
    check_err(access("missing", F_OK), ENOENT, "F_OK on a missing file");

    /* Each permission checked on its own, and together */
    check(access("r", R_OK) == 0, "R_OK");
    check(access("w", W_OK) == 0, "W_OK");
    check(access("x", X_OK) == 0, "X_OK");
    if (!root) {
        check_err(access("r", W_OK), EACCES, "W_OK on a read-only file");
        check_err(access("r", X_OK), EACCES, "X_OK on a read-only file");
        check_err(access("w", R_OK), EACCES, "R_OK on a write-only file");
        check_err(access("x", R_OK), EACCES, "R_OK on an execute-only file");
        check_err(access("r", R_OK | W_OK), EACCES, "R_OK|W_OK needs both");
        check_err(access("none", R_OK), EACCES, "no permissions");
    }
    check(chmod("r", 0700) == 0 && access("r", R_OK | W_OK | X_OK) == 0, "all three");
    /* X_OK fails for a file nobody may execute, unless privileged (may succeed) */
    check(root || access("w", X_OK) == -1, "X_OK on a file nobody may execute");

    /* Pathname errors */
    check_err(access("", F_OK), ENOENT, "empty path");
    check_err(access("r/x", F_OK), ENOTDIR, "prefix not a directory");
    check_err(access("r/", F_OK), ENOTDIR, "trailing slash on a file");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(access("loop", F_OK), ELOOP, "loop");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(access(long_name, F_OK), ENAMETOOLONG, "long component");
    check(mkdir("sub", 0700) == 0, "mkdir");
    make("sub/f", 0600);
    if (!root) {
        check(chmod("sub", 0600) == 0, "no search permission");
        check_err(access("sub/f", F_OK), EACCES, "no search permission in the prefix");
        check(chmod("sub", 0700) == 0, "restore");
    }

    /* An invalid amode may be EINVAL */
    errno = 0;
    check(access("r", 0x100) == 0 || errno == EINVAL, "invalid amode");
    errno = 0;
    check(faccessat(AT_FDCWD, "r", 0x100, 0) == 0 || errno == EINVAL, "faccessat invalid amode");

    /* faccessat: relative to the descriptor, AT_FDCWD, AT_EACCESS */
    dfd = open("sub", O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open directory");
    check(faccessat(dfd, "f", R_OK | W_OK, 0) == 0, "relative to the descriptor");
    check(faccessat(AT_FDCWD, "sub/f", R_OK | W_OK, 0) == 0, "AT_FDCWD");
    check(faccessat(dfd, "f", R_OK | W_OK, AT_EACCESS) == 0, "AT_EACCESS");
    if (!root) {
        check_err(faccessat(dfd, "f", X_OK, 0), EACCES, "EACCES through the descriptor");
        check_err(faccessat(dfd, "f", X_OK, AT_EACCESS), EACCES, "EACCES with AT_EACCESS");
        check(chmod("sub", 0600) == 0, "no search permission");
        check_err(faccessat(dfd, "f", F_OK, 0), EACCES, "no search permission on the descriptor");
        check(chmod("sub", 0700) == 0, "restore");
    }
    check(faccessat(-1, "/", F_OK, 0) == 0, "absolute path ignores the descriptor");
    check_err(faccessat(-1, "f", F_OK, 0), EBADF, "bad descriptor");
    fd = open("r", O_RDONLY);
    check(fd >= 0, "open file");
    check_err(faccessat(fd, "f", F_OK, 0), ENOTDIR, "file descriptor");
    check(close(fd) == 0 && close(dfd) == 0, "close");
    check_err(faccessat(AT_FDCWD, "r", F_OK, 0x40000000), EINVAL, "bad flag");

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
