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
/* nftw is an XSI interface */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                  \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0           \
        && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS) && __PICOLIBC_POSIX_SYMBOLIC_LINKS > 0)

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * nftw walks a tree, before or after the directories' contents, follows
 * links or not without walking a directory inside itself, reports what
 * it can't read or examine, stays on one file system when asked, uses
 * at most fd_limit close-on-exec descriptors, changes directory only
 * for FTW_CHDIR, and stops at the first non-zero result.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define MAX_SEEN 64

static char dir[] = "tmp-XXXXXX";
static char seen[MAX_SEEN][PATH_MAX];
static int  types[MAX_SEEN], levels[MAX_SEEN], count;
static int  in_dir_ok, stop_after;
static char start_cwd[PATH_MAX];

static int
record(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)st;
    check(count < MAX_SEEN, "too many entries");
    snprintf(seen[count], sizeof(seen[count]), "%s", path);
    types[count] = type;
    levels[count] = ftw->level;
    count++;
    return 0;
}

/* The entry with this path, or -1 */
static int
find(const char *path)
{
    int i;

    for (i = 0; i < count; i++)
        if (strcmp(seen[i], path) == 0)
            return i;
    return -1;
}

static void
walk(const char *path, int fd_limit, int flags)
{
    count = 0;
    check(nftw(path, record, fd_limit, flags) == 0, "nftw");
}

static int
visit(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    char cwd[PATH_MAX];

    /* base is where the name starts; the levels follow the slashes */
    check(ftw->base == 0 || path[ftw->base - 1] == '/', "base follows a slash");
    check(strchr(path + ftw->base, '/') == NULL, "base starts the last component");
    check(getcwd(cwd, sizeof(cwd)) != NULL && strcmp(cwd, start_cwd) == 0,
          "without FTW_CHDIR the directory doesn't change");
    if (type == FTW_SLN)
        check(S_ISLNK(st->st_mode), "FTW_SLN reports the link");
    if (type == FTW_F)
        check(!S_ISDIR(st->st_mode) && !S_ISLNK(st->st_mode), "FTW_F reports a file");
    if (type == FTW_D || type == FTW_DP)
        check(S_ISDIR(st->st_mode), "FTW_D reports a directory");
    return record(path, st, type, ftw);
}

static int
check_cwd(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    struct stat here;

    (void)type;
    /* With FTW_CHDIR, the base name works relative to the working directory */
    if (lstat(path + ftw->base, &here) == 0 && here.st_ino == st->st_ino)
        in_dir_ok++;
    return 0;
}

static int
stop(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    record(path, st, type, ftw);
    return count == stop_after ? 42 : 0;
}

static int baseline_fds, baseline_inherited, max_fds, extra_inherited;

/* The number of open descriptors, and in *inherited those without FD_CLOEXEC */
static int
open_fds(int *inherited)
{
    int fd, n = 0;

    *inherited = 0;
    for (fd = 0; fd < 1024; fd++) {
        int fl = fcntl(fd, F_GETFD);

        if (fl == -1)
            continue;
        n++;
        if (!(fl & FD_CLOEXEC))
            (*inherited)++;
    }
    return n;
}

static int
count_fds(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    int inherited;
    int n = open_fds(&inherited) - baseline_fds;

    (void)path;
    (void)st;
    (void)type;
    (void)ftw;
    if (n > max_fds)
        max_fds = n;
    if (inherited > baseline_inherited)
        extra_inherited++;
    return 0;
}

static void
touch(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT, 0600);

    check(fd >= 0 && close(fd) == 0, "create");
}

/* FTW_MOUNT and FTW_XDEV, where /dev/shm is a separate file system inside /dev */
static dev_t dev_dev;
static int   other_dev, mine;
static char  shm_dir[] = "/dev/shm/nftw-XXXXXX";

static int
mount_visit(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)ftw;
    if (type != FTW_NS && st->st_dev != dev_dev)
        other_dev++;
    if (strcmp(path, shm_dir) == 0 || strcmp(path, "/dev/shm") == 0)
        mine++;
    return 0;
}

static void
check_mount(void)
{
    struct stat st_dev, st_shm;

    if (stat("/dev", &st_dev) != 0 || stat("/dev/shm", &st_shm) != 0 || !S_ISDIR(st_shm.st_mode)
        || st_dev.st_dev == st_shm.st_dev || mkdtemp(shm_dir) == NULL) {
        printf("no separate /dev/shm, skipping FTW_MOUNT and FTW_XDEV\n");
        return;
    }
    dev_dev = st_dev.st_dev;

    /* Without either flag, the walk crosses into /dev/shm */
    other_dev = mine = 0;
    check(nftw("/dev", mount_visit, 8, FTW_PHYS) == 0, "walk /dev");
    check(other_dev > 0 && mine == 2, "the walk crosses file systems");

    /* FTW_MOUNT: nothing from another file system, not even the directory */
    other_dev = mine = 0;
    check(nftw("/dev", mount_visit, 8, FTW_PHYS | FTW_MOUNT) == 0, "walk /dev FTW_MOUNT");
    check(other_dev == 0 && mine == 0, "FTW_MOUNT stays on one file system");

#ifdef FTW_XDEV
    /* FTW_XDEV: the directory, but not what is below it (glibc 2.43 lacks FTW_XDEV) */
    other_dev = mine = 0;
    check(nftw("/dev", mount_visit, 8, FTW_PHYS | FTW_XDEV) == 0, "walk /dev FTW_XDEV");
    check(mine == 1 && other_dev > 0, "FTW_XDEV reports the mount point only");
#endif
    check(rmdir(shm_dir) == 0, "remove");
}

int
main(void)
{
    int  d, s, f, l, i;
    char path[PATH_MAX], longname[NAME_MAX + 2];

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    check(getcwd(start_cwd, sizeof(start_cwd)) != NULL, "getcwd");
    check(mkdir("top", 0700) == 0 && mkdir("top/sub", 0700) == 0, "mkdir");
    touch("top/file");
    touch("top/sub/inner");
    check(symlink("file", "top/link") == 0 && symlink("missing", "top/dangling") == 0, "symlink");

    /* FTW_PHYS: links are reported, not followed */
    count = 0;
    check(nftw("top", visit, 4, FTW_PHYS) == 0, "nftw");
    check(count == 6, "every entry");
    d = find("top");
    s = find("top/sub");
    f = find("top/sub/inner");
    l = find("top/link");
    check(d >= 0 && types[d] == FTW_D && levels[d] == 0, "top");
    check(s > d && types[s] == FTW_D && levels[s] == 1, "sub after top");
    check(f > s && types[f] == FTW_F && levels[f] == 2, "inner after sub");
    check(l >= 0 && types[l] == FTW_SL && levels[l] == 1, "symbolic link");
    check(types[find("top/dangling")] == FTW_SL, "dangling link with FTW_PHYS");

    /* FTW_DEPTH: directories after their contents; links followed */
    count = 0;
    check(nftw("top", visit, 4, FTW_DEPTH) == 0, "FTW_DEPTH");
    check(count == 6, "every entry");
    d = find("top");
    s = find("top/sub");
    f = find("top/sub/inner");
    check(types[d] == FTW_DP && types[s] == FTW_DP && f < s && s < d, "directories last");
    check(d == count - 1, "the root last");
    check(types[find("top/link")] == FTW_F, "followed link");
    check(types[find("top/dangling")] == FTW_SLN, "FTW_SLN");
    check(find("top/file") < d, "files before their directory");

    /* FTW_CHDIR */
    check(nftw("top", check_cwd, 4, FTW_CHDIR | FTW_PHYS) == 0 && in_dir_ok == 6, "FTW_CHDIR");
    check(chdir(start_cwd) == 0, "chdir back");

    /* Stopping: the value comes back, and nothing more is visited */
    for (stop_after = 1; stop_after <= 6; stop_after++) {
        count = 0;
        check(nftw("top", stop, 4, stop_after & 1 ? 0 : FTW_DEPTH) == 42 && count == stop_after,
              "stops at a non-zero result");
    }

    /* fd_limit: at most that many descriptors, each close-on-exec, all closed at the end */
    check(mkdir("deep", 0700) == 0, "mkdir");
    strcpy(path, "deep");
    for (i = 0; i < 6; i++) {
        strcat(path, "/d");
        check(mkdir(path, 0700) == 0, "mkdir");
        snprintf(path + strlen(path), sizeof(path) - strlen(path), "/f");
        touch(path);
        path[strlen(path) - 2] = '\0';
    }
    for (i = 1; i <= 3; i++) {
        int inherited;

        baseline_fds = open_fds(&baseline_inherited);
        max_fds = 0;
        extra_inherited = 0;
        check(nftw("deep", count_fds, i, FTW_PHYS) == 0, "nftw deep");
        check(max_fds >= 1 && max_fds <= i, "no more than fd_limit descriptors");
        check(extra_inherited == 0, "FD_CLOEXEC");
        check(open_fds(&inherited) == baseline_fds, "descriptors closed at the end");
    }
    walk("deep", 1, FTW_PHYS | FTW_DEPTH);
    check(count == 13, "every entry with fd_limit 1");
    for (i = 0; i < count; i++)
        if (types[i] == FTW_DP && levels[i] < 6) {
            strcpy(path, seen[i]);
            strcat(path, "/d");
            check(find(path) >= 0 && find(path) < i, "FTW_DEPTH with fd_limit 1");
        }

    /* Links back up the tree: followed, but no directory is walked inside itself */
    check(mkdir("cyc", 0700) == 0 && mkdir("cyc/a", 0700) == 0 && mkdir("cyc/b", 0700) == 0,
          "mkdir");
    touch("cyc/a/f");
    check(symlink("..", "cyc/a/up") == 0 && symlink("../a", "cyc/b/side") == 0, "symlink");
    walk("cyc", 8, FTW_DEPTH);
    check(find("cyc/a/up") < 0, "FTW_DEPTH doesn't report a directory inside itself");
    check(find("cyc") == count - 1, "the rest of the walk");
#ifndef __GLIBC__
    /* glibc skips every directory it has visited, rather than only those inside themselves */
    check(find("cyc/a/f") >= 0, "the directory itself is walked");
    check(find("cyc/b/side/f") >= 0 && find("cyc/b/side/up") < 0, "a link to a sibling is walked");
    walk("cyc", 8, 0);
    i = find("cyc/a/up");
    check(i >= 0 && types[i] == FTW_D, "a link to a parent is reported");
    check(find("cyc/a/up/a") < 0 && find("cyc/a/up/b") < 0, "but not its contents");
#endif

    /* A link loop inside the tree is an error */
    check(mkdir("lp", 0700) == 0 && symlink("loop", "lp/loop") == 0, "symlink");
    errno = 0;
    check(nftw("lp", record, 4, 0) == -1 && errno == ELOOP, "ELOOP inside the tree");
    walk("lp", 4, FTW_PHYS);
    check(types[find("lp/loop")] == FTW_SL, "FTW_PHYS reports the loop link");

    if (geteuid() != 0) {
        /* Permissions don't apply to root */
        check(mkdir("perm", 0700) == 0 && mkdir("perm/noread", 0700) == 0
                  && mkdir("perm/nosearch", 0700) == 0,
              "mkdir");
        touch("perm/noread/f");
        touch("perm/nosearch/f");
        check(chmod("perm/noread", 0300) == 0 && chmod("perm/nosearch", 0600) == 0, "chmod");
        walk("perm", 4, FTW_PHYS);
        i = find("perm/noread");
        check(i >= 0 && types[i] == FTW_DNR, "FTW_DNR");
        check(find("perm/noread/f") < 0, "nothing inside FTW_DNR");
        i = find("perm/nosearch/f");
        check(i >= 0 && types[i] == FTW_NS, "FTW_NS");
        errno = 0;
        check(nftw("perm/nosearch/f", record, 4, 0) == -1 && errno == EACCES,
              "EACCES searching the path");
        check(chmod("perm/noread", 0700) == 0 && chmod("perm/nosearch", 0700) == 0, "chmod");
        check(unlink("perm/noread/f") == 0 && unlink("perm/nosearch/f") == 0
                  && rmdir("perm/noread") == 0 && rmdir("perm/nosearch") == 0 && rmdir("perm") == 0,
              "remove");
    } else {
        printf("running as root, skipping the permission checks\n");
    }

    /* Errors for the starting path */
    errno = 0;
    check(nftw("no-such-top", record, 4, 0) == -1 && errno == ENOENT, "missing start");
    errno = 0;
    check(nftw("", record, 4, 0) == -1 && errno == ENOENT, "empty start");
    errno = 0;
    check(nftw("top/file/x", record, 4, 0) == -1 && errno == ENOTDIR, "ENOTDIR");
    errno = 0;
    check(nftw("lp/loop", record, 4, 0) == -1 && errno == ELOOP, "ELOOP");
    memset(longname, 'n', NAME_MAX + 1);
    longname[NAME_MAX + 1] = '\0';
    errno = 0;
    check(nftw(longname, record, 4, 0) == -1 && errno == ENAMETOOLONG, "ENAMETOOLONG");

    check_mount();

    check(unlink("top/sub/inner") == 0 && rmdir("top/sub") == 0, "remove sub");
    check(unlink("top/file") == 0 && unlink("top/link") == 0 && unlink("top/dangling") == 0,
          "remove files");
    check(rmdir("top") == 0, "remove top");
    strcpy(path, "deep/d/d/d/d/d/d");
    for (i = 6; i > 0; i--) {
        size_t len = strlen(path);

        strcat(path, "/f");
        check(unlink(path) == 0, "remove");
        path[len] = '\0';
        check(rmdir(path) == 0, "remove");
        path[len - 2] = '\0';
    }
    check(rmdir("deep") == 0, "remove deep");
    check(unlink("cyc/a/up") == 0 && unlink("cyc/b/side") == 0 && unlink("cyc/a/f") == 0
              && rmdir("cyc/a") == 0 && rmdir("cyc/b") == 0 && rmdir("cyc") == 0,
          "remove cyc");
    check(unlink("lp/loop") == 0 && rmdir("lp") == 0, "remove lp");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
