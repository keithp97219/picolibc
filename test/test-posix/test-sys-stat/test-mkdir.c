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

#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && __PICOLIBC_POSIX_FILE_SYSTEM_FD > 0)   \
        && (defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS) && __PICOLIBC_POSIX_SYMBOLIC_LINKS > 0)   \
        && (defined(__PICOLIBC_POSIX_FIFO) && __PICOLIBC_POSIX_FIFO > 0)                       \
        && (defined(__PICOLIBC_POSIX_FIFO_FD) && __PICOLIBC_POSIX_FIFO_FD > 0))

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * mkdir, mkdirat, mkfifo, mkfifoat, mknod and mknodat apply the file
 * creation mask, set the owner, refuse existing names and symbolic
 * links, mark the parent for update, and report errors
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "/tmp/mkdir-XXXXXX";
static char path[PATH_MAX];

static const char *
name(const char *n)
{
    snprintf(path, sizeof(path), "%s/%s", dir, n);
    return path;
}

/* Sets the directory's times far in the past */
static void
backdate(int dfd)
{
    struct timespec times[2] = {
        { 1000000000, 0 },
        { 1000000000, 0 }
    };

    check(futimens(dfd, times) == 0, "backdate");
}

static void
check_parent_updated(int dfd, const char *what)
{
    struct stat st;

    check(fstat(dfd, &st) == 0, "fstat parent");
    if (st.st_mtim.tv_sec <= 1000000000 || st.st_ctim.tv_sec <= 1000000000) {
        printf("%s: parent mtime %lld ctime %lld\n", what, (long long)st.st_mtim.tv_sec,
               (long long)st.st_ctim.tv_sec);
        check(0, "the parent's timestamps are marked for update");
    }
}

int
main(void)
{
    struct stat    st;
    struct dirent *ent;
    DIR           *d;
    char           longname[NAME_MAX + 2];
    mode_t         old = umask(022);
    gid_t          parent_gid;
    int            dfd, fd, n;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0 && fstat(dfd, &st) == 0, "open directory");
    parent_gid = st.st_gid;

    /* mkdir: the mask applies, the owner is the effective user, it's empty */
    backdate(dfd);
    check(mkdir(name("sub"), 0777) == 0, "mkdir");
    check(stat(path, &st) == 0 && S_ISDIR(st.st_mode) && (st.st_mode & 0777) == 0755,
          "mkdir mode with umask 022");
    check(st.st_uid == geteuid(), "mkdir owner");
    check(st.st_gid == parent_gid || st.st_gid == getegid(), "mkdir group");
    check_parent_updated(dfd, "mkdir");
    d = opendir(path);
    check(d != NULL, "opendir");
    n = 0;
    while ((ent = readdir(d)) != NULL)
        check(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 || !++n,
              "the new directory is empty");
    check(closedir(d) == 0, "closedir");
    check(rmdir(path) == 0, "rmdir");
    /* S_ISVTX comes from mode too */
    check(mkdir(name("sub"), S_ISVTX | 0755) == 0 && stat(path, &st) == 0
              && (st.st_mode & 07777) == (S_ISVTX | 0755),
          "mkdir S_ISVTX");
    check(rmdir(path) == 0, "rmdir");
    umask(077);
    check(mkdir(name("sub"), 0777) == 0 && stat(path, &st) == 0 && (st.st_mode & 0777) == 0700,
          "mkdir mode with umask 077");
    check(rmdir(path) == 0, "rmdir");
    umask(022);

    /* mkdirat: relative to the descriptor, AT_FDCWD, absolute paths */
    check(mkdirat(dfd, "sub", 0750) == 0 && stat(name("sub"), &st) == 0
              && (st.st_mode & 0777) == 0750,
          "mkdirat");
    check(rmdir(path) == 0, "rmdir");
    check(fchdir(dfd) == 0 && mkdirat(AT_FDCWD, "sub", 0700) == 0 && chdir("/") == 0,
          "mkdirat AT_FDCWD");
    check(stat(name("sub"), &st) == 0 && S_ISDIR(st.st_mode), "AT_FDCWD made it");
    check(rmdir(path) == 0, "rmdir");
    check(mkdirat(-1, name("sub"), 0700) == 0 && rmdir(path) == 0,
          "an absolute path ignores the descriptor");

    /* mkfifo and mknod: the mask applies, the owner is the effective user */
    backdate(dfd);
    check(mkfifo(name("fifo"), 0666) == 0, "mkfifo");
    check(stat(path, &st) == 0 && S_ISFIFO(st.st_mode) && (st.st_mode & 0777) == 0644,
          "mkfifo mode");
    check(st.st_uid == geteuid(), "mkfifo owner");
    check(st.st_gid == parent_gid || st.st_gid == getegid(), "mkfifo group");
    check_parent_updated(dfd, "mkfifo");
    check(unlink(path) == 0, "unlink");
    check(mkfifoat(dfd, "fifo", 0666) == 0 && stat(name("fifo"), &st) == 0
              && (st.st_mode & 0777) == 0644,
          "mkfifoat mode");
    check(unlink(path) == 0, "unlink");
    backdate(dfd);
    check(mknod(name("fifo"), S_IFIFO | 0666, 0) == 0, "mknod");
    check(stat(path, &st) == 0 && S_ISFIFO(st.st_mode) && (st.st_mode & 0777) == 0644,
          "mknod mode");
    check(st.st_uid == geteuid(), "mknod owner");
    check(st.st_gid == parent_gid || st.st_gid == getegid(), "mknod group");
    check_parent_updated(dfd, "mknod");
    check(unlink(path) == 0, "unlink");
    check(mknodat(dfd, "fifo", S_IFIFO | 0640, 0) == 0 && stat(name("fifo"), &st) == 0
              && S_ISFIFO(st.st_mode) && (st.st_mode & 0777) == 0640,
          "mknodat");
    check(unlink(path) == 0, "unlink");

    /* Existing names and symbolic links, even dangling ones, are EEXIST */
    fd = openat(dfd, "file", O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0 && close(fd) == 0, "create file");
    check(symlink("nowhere", name("dangling")) == 0, "dangling symlink");
    check(symlink("file", name("link")) == 0, "symlink");
    static const char * const existing[] = { "file", "dangling", "link" };
    for (n = 0; n < 3; n++) {
        errno = 0;
        check(mkdir(name(existing[n]), 0700) == -1 && errno == EEXIST, "mkdir EEXIST");
        errno = 0;
        check(mkdirat(dfd, existing[n], 0700) == -1 && errno == EEXIST, "mkdirat EEXIST");
        errno = 0;
        check(mkfifo(name(existing[n]), 0600) == -1 && errno == EEXIST, "mkfifo EEXIST");
        errno = 0;
        check(mkfifoat(dfd, existing[n], 0600) == -1 && errno == EEXIST, "mkfifoat EEXIST");
        errno = 0;
        check(mknod(name(existing[n]), S_IFIFO | 0600, 0) == -1 && errno == EEXIST, "mknod EEXIST");
        errno = 0;
        check(mknodat(dfd, existing[n], S_IFIFO | 0600, 0) == -1 && errno == EEXIST,
              "mknodat EEXIST");
    }
    check(lstat(name("dangling"), &st) == 0 && S_ISLNK(st.st_mode), "the link is still there");
    errno = 0;
    check(lstat(name("nowhere"), &st) == -1 && errno == ENOENT, "nothing made at the target");

    /* Errors */
    errno = 0;
    check(mkdir("", 0700) == -1 && errno == ENOENT, "mkdir ENOENT empty path");
    errno = 0;
    check(mkfifo("", 0600) == -1 && errno == ENOENT, "mkfifo ENOENT empty path");
    errno = 0;
    check(mknod("", S_IFIFO | 0600, 0) == -1 && errno == ENOENT, "mknod ENOENT empty path");
    errno = 0;
    check(mkdir(name("missing/sub"), 0700) == -1 && errno == ENOENT, "mkdir ENOENT prefix");
    errno = 0;
    check(mkfifo(name("missing/fifo"), 0600) == -1 && errno == ENOENT, "mkfifo ENOENT prefix");
    errno = 0;
    check(mkdir(name("file/sub"), 0700) == -1 && errno == ENOTDIR, "mkdir ENOTDIR");
    errno = 0;
    check(mkfifo(name("file/fifo"), 0600) == -1 && errno == ENOTDIR, "mkfifo ENOTDIR");
    errno = 0;
    check(mknod(name("file/fifo"), S_IFIFO | 0600, 0) == -1 && errno == ENOTDIR, "mknod ENOTDIR");
    errno = 0;
    check(mkfifo(name("fifo/"), 0600) == -1 && (errno == ENOENT || errno == ENOTDIR),
          "mkfifo trailing slash");
    errno = 0;
    check(mkfifo(name("file/"), 0600) == -1 && errno != ENOENT,
          "mkfifo trailing slash on an existing file");
    memset(longname, 'n', sizeof(longname) - 1);
    longname[sizeof(longname) - 1] = '\0';
    errno = 0;
    check(mkdir(name(longname), 0700) == -1 && errno == ENAMETOOLONG, "mkdir ENAMETOOLONG");
    errno = 0;
    check(mkfifo(name(longname), 0600) == -1 && errno == ENAMETOOLONG, "mkfifo ENAMETOOLONG");
    check(symlink("loop", name("loop")) == 0, "symlink loop");
    errno = 0;
    check(mkdir(name("loop/sub"), 0700) == -1 && errno == ELOOP, "mkdir ELOOP");
    errno = 0;
    check(mkfifo(name("loop/fifo"), 0600) == -1 && errno == ELOOP, "mkfifo ELOOP");
    check(unlink(name("loop")) == 0, "unlink loop");
    /* Only FIFOs can be made without privileges; mode type 0170000 is invalid */
    errno = 0;
    check(mknod(name("bad"), S_IFMT | 0600, 0) == -1 && errno == EINVAL, "mknod EINVAL");
    if (geteuid() != 0) {
        errno = 0;
        /* Linux lets anyone make a 0:0 character device (an overlayfs whiteout), so use 1:3 */
        check(mknod(name("chr"), S_IFCHR | 0600, (dev_t)0x103) == -1 && errno == EPERM,
              "mknod EPERM");
        errno = 0;
        check(mknodat(dfd, "blk", S_IFBLK | 0600, 0) == -1 && errno == EPERM, "mknodat EPERM");

        /* Write and search permission on the parent; root has both */
        check(chmod(dir, 0500) == 0, "remove write permission");
        errno = 0;
        check(mkdir(name("sub"), 0700) == -1 && errno == EACCES, "mkdir EACCES write");
        errno = 0;
        check(mkfifoat(dfd, "fifo", 0600) == -1 && errno == EACCES, "mkfifoat EACCES write");
        errno = 0;
        check(mknod(name("fifo"), S_IFIFO | 0600, 0) == -1 && errno == EACCES,
              "mknod EACCES write");
        check(chmod(dir, 0600) == 0, "remove search permission");
        errno = 0;
        check(mkdirat(dfd, "sub", 0700) == -1 && errno == EACCES, "mkdirat EACCES search");
        errno = 0;
        check(mkfifo(name("fifo"), 0600) == -1 && errno == EACCES, "mkfifo EACCES search");
        check(chmod(dir, 0700) == 0, "restore permission");
    }
    fd = openat(dfd, "file", O_RDONLY);
    check(fd >= 0, "open file");
    errno = 0;
    check(mkdirat(fd, "sub", 0700) == -1 && errno == ENOTDIR, "mkdirat ENOTDIR descriptor");
    errno = 0;
    check(mkfifoat(fd, "fifo", 0600) == -1 && errno == ENOTDIR, "mkfifoat ENOTDIR descriptor");
    errno = 0;
    check(mknodat(fd, "fifo", S_IFIFO | 0600, 0) == -1 && errno == ENOTDIR,
          "mknodat ENOTDIR descriptor");
    check(close(fd) == 0, "close file");
    errno = 0;
    check(mkdirat(-1, "sub", 0700) == -1 && errno == EBADF, "mkdirat EBADF");
    errno = 0;
    check(mkfifoat(-1, "fifo", 0600) == -1 && errno == EBADF, "mkfifoat EBADF");
    errno = 0;
    check(mknodat(-1, "fifo", S_IFIFO | 0600, 0) == -1 && errno == EBADF, "mknodat EBADF");
    /* Nothing was made by the failed calls */
    d = opendir(dir);
    check(d != NULL, "opendir");
    n = 0;
    while ((ent = readdir(d)) != NULL)
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
            n++;
    check(closedir(d) == 0, "closedir");
    check(n == 3, "only file, dangling and link remain");

    check(unlink(name("file")) == 0 && unlink(name("dangling")) == 0 && unlink(name("link")) == 0,
          "unlink");
    check(close(dfd) == 0 && rmdir(dir) == 0, "clean up");
    umask(old);
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
