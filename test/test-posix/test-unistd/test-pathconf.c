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
/* _XOPEN_NAME_MAX, the XSI minimum for _PC_TEXTDOMAIN_MAX, is XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * pathconf and fpathconf report limits of at least the POSIX minimums,
 * agree with the <unistd.h> options, leave errno alone on success and
 * fail as POSIX lists
 */

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
    struct statvfs sv;
    int            fds[2], fd;
    long           v;

    /* A limit with no maximum is -1 without changing errno */
    errno = 0;
    v = pathconf(".", _PC_NAME_MAX);
    check(v >= _POSIX_NAME_MAX || (v == -1 && errno == 0), "_PC_NAME_MAX");
    v = pathconf(".", _PC_PATH_MAX);
    check(v >= _POSIX_PATH_MAX || (v == -1 && errno == 0), "_PC_PATH_MAX");
    v = pathconf(".", _PC_LINK_MAX);
    check(v >= _POSIX_LINK_MAX || (v == -1 && errno == 0), "_PC_LINK_MAX");
    v = pathconf(".", _PC_NO_TRUNC);
    check(v != 0 || errno == 0, "_PC_NO_TRUNC");

    /* The name limit is the file system's */
    check(statvfs(".", &sv) == 0, "statvfs");
    check(pathconf(".", _PC_NAME_MAX) == (long)sv.f_namemax, "f_namemax");

    fd = open(".", O_RDONLY);
    check(fd >= 0, "open .");
    check(fpathconf(fd, _PC_NAME_MAX) == pathconf(".", _PC_NAME_MAX), "fpathconf agrees");
    check(close(fd) == 0, "close");

    check(pipe(fds) == 0, "pipe");
    v = fpathconf(fds[0], _PC_PIPE_BUF);
    check(v >= _POSIX_PIPE_BUF || (v == -1 && errno == 0), "_PC_PIPE_BUF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");

    /* Options <unistd.h> claims for every file aren't -1 */
    errno = 0;
#if defined(_POSIX_CHOWN_RESTRICTED) && _POSIX_CHOWN_RESTRICTED != -1
    check(pathconf(".", _PC_CHOWN_RESTRICTED) != -1, "_PC_CHOWN_RESTRICTED");
#endif
#if defined(_POSIX_NO_TRUNC) && _POSIX_NO_TRUNC != -1
    check(pathconf(".", _PC_NO_TRUNC) != -1, "_PC_NO_TRUNC");
#endif
    v = pathconf(".", _PC_FILESIZEBITS);
    check(v >= 32 || v == -1, "_PC_FILESIZEBITS");
    v = pathconf(".", _PC_SYMLINK_MAX);
    check(v >= _POSIX_SYMLINK_MAX || v == -1, "_PC_SYMLINK_MAX");
#ifdef _PC_TIMESTAMP_RESOLUTION
    v = pathconf(".", _PC_TIMESTAMP_RESOLUTION);
    check(v > 0 || v == -1, "_PC_TIMESTAMP_RESOLUTION");
#endif
#ifdef _PC_TEXTDOMAIN_MAX
    v = pathconf(".", _PC_TEXTDOMAIN_MAX);
    check(v >= _POSIX_NAME_MAX - 3 || v == -1, "_PC_TEXTDOMAIN_MAX");
#ifdef _XOPEN_NAME_MAX
    check(v >= _XOPEN_NAME_MAX - 3 || v == -1, "XSI _PC_TEXTDOMAIN_MAX");
#endif
#endif
    check(errno == 0, "success leaves errno alone");

    char dir[] = "test-pathconf-XXXXXX";
    char path[PATH_MAX];

    check(mkdtemp(dir) != NULL, "mkdtemp");

#if defined(_POSIX_ADVISORY_INFO) && _POSIX_ADVISORY_INFO >= 0
    if (sysconf(_SC_ADVISORY_INFO) > 0) {
        struct stat st;

        /* Even a one-byte file takes at least _PC_ALLOC_SIZE_MIN bytes */
        snprintf(path, sizeof(path), "%s/one", dir);
        fd = open(path, O_WRONLY | O_CREAT, 0600);
        check(fd >= 0 && write(fd, "x", 1) == 1 && fsync(fd) == 0, "write a byte");
        v = fpathconf(fd, _PC_ALLOC_SIZE_MIN);
        check(fstat(fd, &st) == 0, "fstat");
        check(v > 0 || v == -1, "_PC_ALLOC_SIZE_MIN");
        /* Some file systems keep tiny files in the inode */
        if (v > 0 && st.st_blocks != 0)
            check((long long)st.st_blocks * 512 >= v, "_PC_ALLOC_SIZE_MIN is allocated");
        v = fpathconf(fd, _PC_REC_XFER_ALIGN);
        check(v > 0 || v == -1, "_PC_REC_XFER_ALIGN");
        v = fpathconf(fd, _PC_REC_MIN_XFER_SIZE);
        check(v > 0 || v == -1, "_PC_REC_MIN_XFER_SIZE");
        check(close(fd) == 0 && unlink(path) == 0, "remove");
    }
#endif

    /* With _POSIX_NO_TRUNC, a component longer than NAME_MAX is an error */
    v = pathconf(dir, _PC_NAME_MAX);
    if (v > 0 && v < PATH_MAX - 64 && pathconf(dir, _PC_NO_TRUNC) != -1) {
        int n = snprintf(path, sizeof(path), "%s/", dir);

        memset(path + n, 'n', (size_t)v + 1);
        path[n + v + 1] = '\0';
        errno = 0;
        check(open(path, O_WRONLY | O_CREAT, 0600) == -1 && errno == ENAMETOOLONG,
              "no truncation of long names");
        /* pathconf may report the same error */
        errno = 0;
        check(pathconf(path, _PC_NAME_MAX) == -1 && (errno == ENAMETOOLONG || errno == ENOENT),
              "pathconf of a name that is too long");
    }

    /* A file in the path prefix, or before a trailing slash */
    snprintf(path, sizeof(path), "%s/file", dir);
    fd = open(path, O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create a file");
    snprintf(path, sizeof(path), "%s/file/x", dir);
    errno = 0;
    check(pathconf(path, _PC_NAME_MAX) == -1 && errno == ENOTDIR, "ENOTDIR prefix");
    snprintf(path, sizeof(path), "%s/file/", dir);
    errno = 0;
    check(pathconf(path, _PC_NAME_MAX) == -1 && errno == ENOTDIR, "ENOTDIR trailing slash");
    snprintf(path, sizeof(path), "%s/file", dir);
    check(unlink(path) == 0, "unlink");

#if TEST_POSIX_SYMBOLIC_LINKS
    /* A link to itself is a loop */
    snprintf(path, sizeof(path), "%s/loop", dir);
    check(symlink("loop", path) == 0, "symlink");
    errno = 0;
    check(pathconf(path, _PC_NAME_MAX) == -1 && errno == ELOOP, "ELOOP");
    check(pathconf(dir, _PC_2_SYMLINKS) > 0, "_PC_2_SYMLINKS where links exist");
    check(unlink(path) == 0, "unlink");
#endif

    /* Without search permission on a prefix, as an unprivileged user */
    if (geteuid() != 0) {
        snprintf(path, sizeof(path), "%s/sub", dir);
        check(mkdir(path, 0700) == 0, "mkdir");
        check(chmod(dir, 0600) == 0, "chmod");
        errno = 0;
        check(pathconf(path, _PC_NAME_MAX) == -1 && errno == EACCES, "EACCES");
        check(chmod(dir, 0700) == 0, "chmod back");
        check(rmdir(path) == 0, "rmdir");
    }
    check(rmdir(dir) == 0, "rmdir");

    errno = 0;
    check(pathconf(".", -1) == -1 && errno == EINVAL, "bad name");
    fd = open(".", O_RDONLY);
    check(fd >= 0, "open .");
    errno = 0;
    check(fpathconf(fd, -1) == -1 && errno == EINVAL, "fpathconf bad name");
    check(close(fd) == 0, "close");
    errno = 0;
    check(fpathconf(-1, _PC_NAME_MAX) == -1 && errno == EBADF, "EBADF");
    errno = 0;
    check(pathconf("no/such/path", _PC_NAME_MAX) == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(pathconf("", _PC_NAME_MAX) == -1 && errno == ENOENT, "ENOENT for an empty path");
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
