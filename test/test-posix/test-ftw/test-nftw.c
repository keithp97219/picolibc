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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* nftw walks a tree, before or after the directories' contents */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";
static char seen[16][64];
static int  types[16], levels[16], count;
static int  in_dir_ok;

static int
visit(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)st;
    if (count < 16) {
        snprintf(seen[count], sizeof(seen[count]), "%s", path + ftw->base);
        types[count] = type;
        levels[count] = ftw->level;
        count++;
    }
    return 0;
}

static int
find(const char *name)
{
    int i;

    for (i = 0; i < count; i++)
        if (strcmp(seen[i], name) == 0)
            return i;
    return -1;
}

static int
check_cwd(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    struct stat here;

    (void)path;
    (void)type;
    /* With FTW_CHDIR, the base name works relative to the working directory */
    if (lstat(path + ftw->base, &here) == 0 && here.st_ino == st->st_ino)
        in_dir_ok++;
    return 0;
}

static int
stop(const char *path, const struct stat *st, int type, struct FTW *ftw)
{
    (void)path;
    (void)st;
    (void)ftw;
    return type == FTW_F ? 42 : 0;
}

static void
touch(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT, 0600);

    check(fd >= 0 && close(fd) == 0, "create");
}

int
main(void)
{
    int d, s, f, l;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    check(mkdir("top", 0700) == 0 && mkdir("top/sub", 0700) == 0, "mkdir");
    touch("top/file");
    touch("top/sub/inner");
    check(symlink("file", "top/link") == 0 && symlink("missing", "top/dangling") == 0, "symlink");

    check(nftw("top", visit, 4, FTW_PHYS) == 0, "nftw");
    check(count == 6, "every entry");
    d = find("top");
    s = find("sub");
    f = find("inner");
    l = find("link");
    check(d >= 0 && types[d] == FTW_D && levels[d] == 0, "top");
    check(s > d && types[s] == FTW_D && levels[s] == 1, "sub after top");
    check(f > s && types[f] == FTW_F && levels[f] == 2, "inner after sub");
    check(l >= 0 && types[l] == FTW_SL, "symbolic link");
    check(types[find("dangling")] == FTW_SL, "dangling link with FTW_PHYS");

    count = 0;
    check(nftw("top", visit, 4, FTW_DEPTH) == 0, "FTW_DEPTH");
    d = find("top");
    s = find("sub");
    f = find("inner");
    check(types[d] == FTW_DP && types[s] == FTW_DP && f < s && s < d, "directories last");
    check(types[find("link")] == FTW_F, "followed link");
    check(types[find("dangling")] == FTW_SLN, "FTW_SLN");

    check(nftw("top", check_cwd, 4, FTW_CHDIR | FTW_PHYS) == 0 && in_dir_ok == 6, "FTW_CHDIR");
    check(nftw("top", stop, 4, 0) == 42, "stops at a non-zero result");
    errno = 0;
    check(nftw("no-such-top", stop, 4, 0) == -1 && errno == ENOENT, "missing start");
    check(nftw("", stop, 4, 0) == -1 && errno == ENOENT, "empty start");

    check(unlink("top/sub/inner") == 0 && rmdir("top/sub") == 0, "remove sub");
    check(unlink("top/file") == 0 && unlink("top/link") == 0 && unlink("top/dangling") == 0,
          "remove files");
    check(rmdir("top") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
