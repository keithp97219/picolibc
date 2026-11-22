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
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_SYMBOLIC_LINKS

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * posix_getdents returns aligned records for every entry once, with the
 * types and serial numbers of the entries themselves, in buffers as
 * small as a posix_dent plus NAME_MAX, and reports its errors.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifndef __GLIBC__
static char dir[] = "tmp-XXXXXX";

static char longname[NAME_MAX + 1];

#define NNAMES 7
static const char         *names[NNAMES] = { ".", "..", "a", "sub", "lnk", "fifo", longname };
static const unsigned char types[NNAMES]
    = { DT_DIR, DT_DIR, DT_REG, DT_DIR, DT_LNK, DT_FIFO, DT_REG };

static union {
    char        bytes[4096];
    max_align_t align;
} buf;

/* Reads the directory from fd's offset with nbyte buffers, counting each name */
static void
read_all(int fd, size_t nbyte, int seen[NNAMES])
{
    ssize_t n;

    while ((n = posix_getdents(fd, buf.bytes, nbyte, 0)) > 0) {
        check((size_t)n <= nbyte, "fits in nbyte");
        for (ssize_t off = 0; off < n;) {
            struct posix_dent *d = (struct posix_dent *)(buf.bytes + off);
            struct stat        st;
            int                i;

            check(d->d_reclen > 0 && off + (ssize_t)d->d_reclen <= n, "d_reclen");
            check(d->d_reclen % _Alignof(struct posix_dent) == 0, "d_reclen alignment");
            check(d->d_name[0] != '\0', "no empty names");
            check(strlen(d->d_name) < d->d_reclen - offsetof(struct posix_dent, d_name),
                  "name inside the record");
            for (i = 0; i < NNAMES; i++)
                if (strcmp(d->d_name, names[i]) == 0)
                    break;
            check(i < NNAMES, "only the files made here");
            seen[i]++;
            check(d->d_type == types[i] || d->d_type == DT_UNKNOWN, "d_type (of a link itself)");
            if (i != 1) {
                check(lstat(d->d_name, &st) == 0, "lstat");
                check(d->d_ino == st.st_ino, "d_ino (of a link itself)");
            }
            off += d->d_reclen;
        }
    }
    check(n == 0, "end of directory");
}
#endif

int
main(void)
{
#ifdef __GLIBC__
    /* POSIX.1-2024; glibc 2.43 lacks it */
    printf("no posix_getdents, skipping\n");
    return 77;
#else
    int    seen[NNAMES];
    int    fd, fd2, i;
    size_t small = sizeof(struct posix_dent) + NAME_MAX + 1;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    memset(longname, 'n', NAME_MAX);
    fd = open("a", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create");
    fd = open(longname, O_WRONLY | O_CREAT, 0600);
    check(fd >= 0 && close(fd) == 0, "create a NAME_MAX name");
    check(mkdir("sub", 0700) == 0 && symlink("a", "lnk") == 0 && mkfifo("fifo", 0600) == 0,
          "mkdir, symlink, mkfifo");

    fd = open(".", O_RDONLY | O_DIRECTORY);
    check(fd >= 0, "open directory");

    /* Only flags of zero are defined */
    errno = 0;
    check(posix_getdents(fd, buf.bytes, sizeof(buf.bytes), 1) == -1 && errno == EINVAL, "flags");
    /* Too small for any entry */
    errno = 0;
    check(posix_getdents(fd, buf.bytes, 8, 0) == -1 && errno == EINVAL, "EINVAL buffer too small");
    check(lseek(fd, 0, SEEK_SET) == 0, "rewind");

    /* A large buffer, then one just over a posix_dent plus NAME_MAX, each entry once */
    for (size_t nbyte = sizeof(buf.bytes); nbyte >= small; nbyte = nbyte == small ? 0 : small) {
        memset(seen, 0, sizeof(seen));
        check(lseek(fd, 0, SEEK_SET) == 0, "rewind");
        read_all(fd, nbyte, seen);
        for (i = 0; i < NNAMES; i++)
            check(seen[i] == 1, "each entry once, dot and dot-dot included");
    }

    /* At the end, 0 again; reading continues from the offset */
    check(posix_getdents(fd, buf.bytes, sizeof(buf.bytes), 0) == 0, "0 at the end");
    check(lseek(fd, 0, SEEK_SET) == 0, "rewind");
    {
        ssize_t n = posix_getdents(fd, buf.bytes, small, 0);
        int     first[NNAMES] = { 0 };
        int     rest[NNAMES] = { 0 };

        check(n > 0, "at least one entry");
        for (ssize_t off = 0; off < n; off += ((struct posix_dent *)(buf.bytes + off))->d_reclen)
            for (i = 0; i < NNAMES; i++)
                if (strcmp(((struct posix_dent *)(buf.bytes + off))->d_name, names[i]) == 0)
                    first[i]++;
        read_all(fd, sizeof(buf.bytes), rest);
        for (i = 0; i < NNAMES; i++)
            check(first[i] + rest[i] == 1, "the offset follows the last entry returned");
    }

    /* Errors */
    errno = 0;
    check(posix_getdents(-1, buf.bytes, sizeof(buf.bytes), 0) == -1 && errno == EBADF, "EBADF");
    fd2 = open("a", O_RDONLY);
    check(fd2 >= 0, "open file");
    errno = 0;
    check(posix_getdents(fd2, buf.bytes, sizeof(buf.bytes), 0) == -1 && errno == ENOTDIR,
          "ENOTDIR");
    check(close(fd2) == 0, "close");
#ifdef O_SEARCH
    fd2 = open(".", O_SEARCH | O_DIRECTORY);
    check(fd2 >= 0, "open O_SEARCH");
    errno = 0;
    check(posix_getdents(fd2, buf.bytes, sizeof(buf.bytes), 0) == -1 && errno == EBADF,
          "EBADF not open for reading");
    check(close(fd2) == 0, "close");
#endif
    check(close(fd) == 0, "close directory");

    check(unlink("a") == 0 && unlink(longname) == 0 && unlink("lnk") == 0 && unlink("fifo") == 0
              && rmdir("sub") == 0,
          "remove");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
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
