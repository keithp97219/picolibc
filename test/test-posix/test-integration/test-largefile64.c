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

#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include "../test-flags.h"

#include <stdio.h>
#include <sys/types.h>

/*
 * picolibc's off_t is 64 bits on every target, so the transitional
 * large file names (_LARGEFILE64_SOURCE) are the plain ones
 */
#ifdef __PICOLIBC__
_Static_assert(sizeof(off_t) == 8, "off_t is 64 bits");
#endif

/*
 * Without the system support this test needs, it is skipped. Other C
 * libraries than glibc and picolibc needn't have these names at all
 */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_FILE_SYSTEM_FD \
    && (defined(__PICOLIBC__) || defined(TEST_GLIBC))

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/vfs.h>
#define HAVE_STATFS64
#elif defined(__FreeBSD__)
#include <sys/mount.h>
#define HAVE_STATFS64
#endif

#if !defined(_LFS64_LARGEFILE) || !defined(_LFS64_STDIO)
#error "_LFS64_LARGEFILE and _LFS64_STDIO"
#endif

_Static_assert(sizeof(off64_t) == 8, "off64_t");
_Static_assert(sizeof(fpos64_t) >= 8, "fpos64_t");
_Static_assert(sizeof(ino64_t) == 8, "ino64_t");
_Static_assert(sizeof(blkcnt64_t) == 8, "blkcnt64_t");
_Static_assert(sizeof(fsblkcnt64_t) == 8, "fsblkcnt64_t");
_Static_assert(sizeof(fsfilcnt64_t) == 8, "fsfilcnt64_t");
_Static_assert(sizeof(rlim64_t) == 8, "rlim64_t");

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("big");
    (void)unlink("new");
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

/* Past 4 GiB, so 32-bit offsets would lose the high bits */
#define BIG (((off64_t)5 << 30) + 8192)

static int seen;

static int
visit(const char *path, const struct stat64 *st, int type, struct FTW *ftw)
{
    (void)type;
    (void)ftw;
    if (strcmp(path + strlen(path) - 3, "big") == 0 && st->st_size == BIG + 1)
        seen = 1;
    return 0;
}

int
main(void)
{
    struct stat64     st;
    struct statvfs64  sv;
    struct rlimit64   rl;
    struct dirent64  *e;
    struct dirent64 **list;
    glob64_t          g;
    fpos64_t          pos;
    DIR              *d;
    FILE             *f;
    char              c;
    int               fd, n, found;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* <fcntl.h>, <unistd.h> and <sys/stat.h> */
    fd = open64("big", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open64");
    check(ftruncate64(fd, BIG) == 0, "ftruncate64");
    check(fstat64(fd, &st) == 0 && st.st_size == BIG, "fstat64");
    check(lseek64(fd, 0, SEEK_END) == BIG, "lseek64");
    check(pwrite64(fd, "x", 1, BIG - 1) == 1, "pwrite64");
    check(pread64(fd, &c, 1, BIG - 1) == 1 && c == 'x', "pread64");
    check(truncate64("big", BIG + 1) == 0, "truncate64");
    check(stat64("big", &st) == 0 && st.st_size == BIG + 1, "stat64");
    check(lstat64("big", &st) == 0 && st.st_size == BIG + 1, "lstat64");
    check(fstatat64(AT_FDCWD, "big", &st, 0) == 0 && st.st_size == BIG + 1, "fstatat64");
    check(lockf64(fd, F_TLOCK, 0) == 0 && lockf64(fd, F_ULOCK, 0) == 0, "lockf64");
#if TEST_POSIX_ADVISORY_INFO
    check(posix_fadvise64(fd, 0, BIG, POSIX_FADV_NORMAL) == 0, "posix_fadvise64");
    n = posix_fallocate64(fd, 0, 4096);
    check(n == 0 || n == EINVAL || n == ENOTSUP || n == EOPNOTSUPP, "posix_fallocate64");
#endif
#if TEST_POSIX_MAPPED_FILES
    {
        /* The page holding the last byte, which sits past 4 GiB */
        off64_t page = (BIG - 1) & ~(off64_t)4095;
        char   *p = mmap64(NULL, 4096, PROT_READ, MAP_SHARED, fd, page);

        check(p != MAP_FAILED && p[(BIG - 1) - page] == 'x', "mmap64");
        check(munmap(p, 4096) == 0, "munmap");
    }
#endif
#ifdef HAVE_STATFS64
    {
        struct statfs64 sfs;

        check(statfs64(".", &sfs) == 0 && fstatfs64(fd, &sfs) == 0, "statfs64");
    }
#endif
    check(fstatvfs64(fd, &sv) == 0 && statvfs64(".", &sv) == 0, "statvfs64");
    check(close(fd) == 0, "close");
    fd = openat64(AT_FDCWD, "big", O_RDONLY);
    check(fd >= 0 && close(fd) == 0, "openat64");
    fd = creat64("new", 0600);
    check(fd >= 0 && close(fd) == 0, "creat64");

    /* <stdio.h> */
    f = fopen64("big", "r");
    check(f != NULL, "fopen64");
    check(fseeko64(f, BIG - 1, SEEK_SET) == 0 && ftello64(f) == BIG - 1, "fseeko64");
    check(fgetpos64(f, &pos) == 0, "fgetpos64");
    check(fgetc(f) == 'x' && ftello64(f) == BIG, "read past 4 GiB");
    check(fsetpos64(f, &pos) == 0 && ftello64(f) == BIG - 1, "fsetpos64");
    f = freopen64("new", "r+", f);
    check(f != NULL && fputc('y', f) == 'y' && fclose(f) == 0, "freopen64");
    f = tmpfile64();
    check(f != NULL && fclose(f) == 0, "tmpfile64");

    /* <dirent.h>, <glob.h> and <ftw.h> */
    d = opendir(".");
    check(d != NULL, "opendir");
    found = 0;
    while ((e = readdir64(d)) != NULL)
        if (strcmp(e->d_name, "big") == 0)
            found = 1;
    check(found && closedir(d) == 0, "readdir64");
    n = scandir64(".", &list, NULL, alphasort64);
    check(n >= 2, "scandir64");
    while (n--)
        free(list[n]);
    free(list);
    check(glob64("b*", 0, NULL, &g) == 0 && g.gl_pathc == 1, "glob64");
    globfree64(&g);
    check(nftw64(".", visit, 4, FTW_PHYS) == 0 && seen, "nftw64");

    /* <sys/resource.h> */
    check(getrlimit64(RLIMIT_NOFILE, &rl) == 0, "getrlimit64");
    check(setrlimit64(RLIMIT_NOFILE, &rl) == 0, "setrlimit64");

    cleanup();
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
