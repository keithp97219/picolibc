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
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_SYMBOLIC_LINKS \
    && TEST_POSIX_SYMBOLIC_LINKS_FD

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * symlink, symlinkat, link and linkat: what they make, and the errors,
 * which leave nothing behind
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    static const char * const names[]
        = { "file", "sym", "dangling", "hard",  "hard2", "lsym",  "long",
            "loop", "w/x", "w/f",      "sub/x", "sub/s", "sub/h", "fsym" };
    unsigned i;

    (void)chmod("w", 0700);
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        (void)unlink(names[i]);
    (void)rmdir("sub");
    (void)rmdir("w");
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

static int
exists(const char *path)
{
    struct stat st;

    return lstat(path, &st) == 0;
}

static nlink_t
links(const char *path)
{
    struct stat st;

    check(lstat(path, &st) == 0, "lstat");
    return st.st_nlink;
}

static char long_name[NAME_MAX + 2];
static char long_target[8192];

int
main(void)
{
    struct stat     st, st2;
    struct timespec old[2] = {
        { 1000, 0 },
        { 1000, 0 }
    };
    char buf[64];
    long symlink_max;
    int  fd, dfd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0 && close(fd) == 0, "create");
    check(mkdir("sub", 0700) == 0, "mkdir");

    /* symlink stores the string as given, unchecked */
    check(utimensat(AT_FDCWD, ".", old, 0) == 0, "set old directory times");
    check(symlink("no/such//../path", "dangling") == 0, "symlink to anything");
    check(readlink("dangling", buf, sizeof(buf)) == 16 && memcmp(buf, "no/such//../path", 16) == 0,
          "contents kept");
    check(stat(".", &st) == 0 && st.st_mtime > 1000, "directory times updated");
    check(lstat("dangling", &st) == 0 && S_ISLNK(st.st_mode), "a symbolic link");
    check(st.st_uid == geteuid(), "owned by the effective user");
    check(st.st_gid == getegid() || st.st_gid == (stat(".", &st2) == 0 ? st2.st_gid : (gid_t)-1),
          "group of the process or the directory");

    /* EEXIST for any existing name, links included; nothing changes */
    check(symlink("file", "sym") == 0, "symlink");
    check_err(symlink("x", "file"), EEXIST, "symlink over a file");
    check_err(symlink("x", "sym"), EEXIST, "symlink over a symbolic link");
    check_err(symlink("x", "dangling"), EEXIST, "symlink over a dangling link");
    check_err(symlink("x", "sub"), EEXIST, "symlink over a directory");
    check(readlink("sym", buf, sizeof(buf)) == 4 && memcmp(buf, "file", 4) == 0, "link kept");

    /* Pathname errors in path2 */
    check_err(symlink("x", ""), ENOENT, "empty path2");
    check_err(symlink("x", "missing/x"), ENOENT, "missing prefix");
    check_err(symlink("x", "file/x"), ENOTDIR, "prefix not a directory");
    errno = 0;
    check(symlink("x", "new/") == -1 && (errno == ENOENT || errno == ENOTDIR),
          "trailing slash on a new name");
    check(!exists("new"), "nothing made");
    /* "file/" both names an existing file and fails to resolve; either error fits */
    errno = 0;
    check(symlink("x", "file/") == -1 && (errno == EEXIST || errno == ENOTDIR),
          "trailing slash on an existing file");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(symlink("x", "loop/x"), ELOOP, "loop in the prefix");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(symlink("x", long_name), ENAMETOOLONG, "component longer than NAME_MAX");

    /* ENAMETOOLONG past {SYMLINK_MAX}; anything up to it is kept */
    symlink_max = pathconf(".", _PC_SYMLINK_MAX);
    if (symlink_max > 0 && symlink_max < (long)sizeof(long_target) - 1) {
        memset(long_target, 't', (size_t)symlink_max + 1);
        check_err(symlink(long_target, "long"), ENAMETOOLONG, "contents longer than SYMLINK_MAX");
        check(!exists("long"), "nothing made");
        long_target[symlink_max] = '\0';
        check(symlink(long_target, "long") == 0, "contents of SYMLINK_MAX bytes");
        check(lstat("long", &st) == 0 && st.st_size == symlink_max, "length kept");
        check(unlink("long") == 0, "unlink");
    }

    /* EACCES without write or search permission (not as root) */
    if (geteuid() != 0) {
        check(mkdir("w", 0500) == 0, "read-only directory");
        check_err(symlink("x", "w/x"), EACCES, "no write permission");
        check_err(link("file", "w/x"), EACCES, "link without write permission");
        check(chmod("w", 0600) == 0, "no search permission");
        check_err(symlink("x", "w/x"), EACCES, "no search permission");
        check(chmod("w", 0700) == 0 && !exists("w/x"), "nothing made");
    }

    /* symlinkat: relative to the directory, absolute paths ignore it */
    dfd = open("sub", O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open directory");
    check(symlinkat("../file", dfd, "s") == 0 && exists("sub/s"), "symlinkat");
    check(symlinkat("x", AT_FDCWD, "fsym") == 0 && exists("fsym"), "symlinkat AT_FDCWD");
    check_err(symlinkat("x", -1, "y"), EBADF, "symlinkat with a bad descriptor");
    fd = open("file", O_RDONLY);
    check(fd >= 0, "open file");
    check_err(symlinkat("x", fd, "y"), ENOTDIR, "symlinkat with a file descriptor");

    /* link: one more name for the same file */
    check(link("file", "hard") == 0, "link");
    check(stat("file", &st) == 0 && stat("hard", &st2) == 0, "stat");
    check(st.st_ino == st2.st_ino && st.st_dev == st2.st_dev && links("file") == 2, "same file");
    /* Linux links a symbolic link itself (implementation-defined) */
    check(link("sym", "lsym") == 0, "link a symbolic link");
    check(lstat("lsym", &st) == 0 && S_ISLNK(st.st_mode), "link to the symbolic link");

    /* Errors leave the link count alone */
    check_err(link("file", "hard"), EEXIST, "link to an existing name");
    check_err(link("file", "sym"), EEXIST, "link over a symbolic link");
    check_err(link("file", "dangling"), EEXIST, "link over a dangling link");
    check_err(link("missing", "x"), ENOENT, "missing path1");
    check_err(link("", "x"), ENOENT, "empty path1");
    check_err(link("file", ""), ENOENT, "empty path2");
    check_err(link("file", "missing/x"), ENOENT, "missing prefix of path2");
    check_err(link("file/", "x"), ENOTDIR, "trailing slash on path1");
    check_err(link("file/x", "x"), ENOTDIR, "path1 prefix not a directory");
    errno = 0;
    check(link("file", "new/") == -1 && (errno == ENOENT || errno == ENOTDIR),
          "trailing slash on path2");
    check_err(link("sub", "x"), EPERM, "link a directory");
    check_err(link("loop/x", "x"), ELOOP, "loop");
    check_err(link("file", long_name), ENAMETOOLONG, "component longer than NAME_MAX");
    check(links("file") == 2 && !exists("x") && !exists("new"), "nothing changed");

    /* EXDEV to another file system, where there is one at hand */
    if (stat("/dev/shm", &st) == 0 && stat(".", &st2) == 0 && st.st_dev != st2.st_dev) {
        char other[64];

        snprintf(other, sizeof(other), "/dev/shm/picolibc-link-%ld", (long)getpid());
        errno = 0;
        if (link("file", other) == 0) {
            (void)unlink(other);
            check(0, "link across file systems");
        }
        check(errno == EXDEV || errno == EACCES, "EXDEV across file systems");
    }

    /* linkat: relative to each descriptor; AT_SYMLINK_FOLLOW */
    check(linkat(AT_FDCWD, "file", dfd, "h", 0) == 0 && links("file") == 3, "linkat");
    check(linkat(dfd, "s", AT_FDCWD, "hard2", 0) == 0, "linkat of a symbolic link");
    check(lstat("hard2", &st) == 0 && S_ISLNK(st.st_mode), "link to the link itself");
    check(unlink("hard2") == 0, "unlink");
    check(linkat(dfd, "s", AT_FDCWD, "hard2", AT_SYMLINK_FOLLOW) == 0, "AT_SYMLINK_FOLLOW");
    check(lstat("hard2", &st) == 0 && S_ISREG(st.st_mode) && links("file") == 4,
          "link to the target");
    check_err(linkat(-1, "file", AT_FDCWD, "x", 0), EBADF, "bad fd1");
    check_err(linkat(AT_FDCWD, "file", -1, "x", 0), EBADF, "bad fd2");
    check_err(linkat(fd, "x", AT_FDCWD, "y", 0), ENOTDIR, "fd1 not a directory");
    check_err(linkat(AT_FDCWD, "file", fd, "y", 0), ENOTDIR, "fd2 not a directory");
    check(linkat(-1, "/nonexistent-picolibc-test", -1, "/nonexistent-picolibc-test2", 0) == -1
              && errno == ENOENT,
          "absolute paths ignore the descriptors");
    check_err(linkat(AT_FDCWD, "file", AT_FDCWD, "x", 0x40000000), EINVAL, "bad flag");
    check(close(fd) == 0 && close(dfd) == 0, "close");

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
