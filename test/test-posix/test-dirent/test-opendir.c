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
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * opendir opens close-on-exec streams at the first entry and reports
 * its errors; readdir returns each entry once with its serial number,
 * leaves errno alone at the end and keeps streams apart; rewinddir sees
 * the directory as it is now; closedir closes the descriptor.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

#define MAX_NAMES 16

static const char * const expect[] = { ".", "..", "a", "bb", "sub", "lnk", "dangling", "loop" };
#define NEXPECT (sizeof(expect) / sizeof(expect[0]))

static void
touch(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT, 0600);

    check(fd >= 0 && close(fd) == 0, "create");
}

/* Reads the rest of the stream, returning the number of times name appears */
static int
count_name(DIR *d, const char *name)
{
    struct dirent *ent;
    int            n = 0;

    while ((ent = readdir(d)) != NULL)
        if (strcmp(ent->d_name, name) == 0)
            n++;
    return n;
}

int
main(void)
{
    int            seen[NEXPECT] = { 0 };
    char           first[NAME_MAX + 1];
    char           longname[NAME_MAX + 2];
    struct dirent *ent;
    struct stat    st;
    DIR           *d, *d2;
    size_t         i;
    int            fd, total = 0;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    touch("a");
    touch("bb");
    check(mkdir("sub", 0700) == 0, "mkdir");
    check(symlink("a", "lnk") == 0 && symlink("nowhere", "dangling") == 0
              && symlink("loop", "loop") == 0,
          "symlink");

    /* The stream's descriptor is close-on-exec */
    d = opendir(".");
    check(d != NULL, "opendir");
    fd = dirfd(d);
    check(fd >= 0 && (fcntl(fd, F_GETFD) & FD_CLOEXEC), "FD_CLOEXEC");

    /* Every entry once, from the first, with the serial number of the entry itself */
    while ((ent = readdir(d)) != NULL) {
        check(ent->d_name[0] != '\0', "no empty names");
        total++;
        for (i = 0; i < NEXPECT; i++)
            if (strcmp(ent->d_name, expect[i]) == 0)
                break;
        check(i < NEXPECT, "only the files made here");
        seen[i]++;
        if (strcmp(ent->d_name, "..") != 0) {
            check(lstat(ent->d_name, &st) == 0, "lstat");
            check(ent->d_ino == st.st_ino, "d_ino is the serial number (of a link itself)");
        }
    }
    for (i = 0; i < NEXPECT; i++)
        check(seen[i] == 1, "each entry once, dot and dot-dot included");
    check(total == (int)NEXPECT, "nothing else");

    /* At the end, errno is left alone */
    errno = EINTR;
    check(readdir(d) == NULL && errno == EINTR, "end leaves errno");

    /* rewinddir starts again and sees what's there now */
    touch("new");
    check(unlink("bb") == 0, "unlink");
    rewinddir(d);
    check(count_name(d, "new") == 1, "rewinddir sees a new file");
    rewinddir(d);
    check(count_name(d, "bb") == 0, "rewinddir misses a removed file");
    rewinddir(d);
    check(count_name(d, ".") == 1, "rewinddir to the first entry");

    /* readdir on one stream leaves another's entry alone */
    rewinddir(d);
    d2 = opendir(".");
    check(d2 != NULL, "second opendir");
    ent = readdir(d);
    check(ent != NULL, "readdir");
    strcpy(first, ent->d_name);
    check(readdir(d2) != NULL && readdir(d2) != NULL && readdir(d2) != NULL, "other stream");
    check(strcmp(ent->d_name, first) == 0, "streams are independent");
    check(closedir(d2) == 0, "closedir");

    /* closedir closes the descriptor */
    check(closedir(d) == 0, "closedir");
    errno = 0;
    check(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "closedir closes the descriptor");

    /* A symbolic link to a directory works */
    check(symlink("sub", "lnkdir") == 0, "symlink");
    d = opendir("lnkdir");
    check(d != NULL && closedir(d) == 0, "opendir through a link");

    /* Errors */
    errno = 0;
    check(opendir("") == NULL && errno == ENOENT, "ENOENT for an empty name");
    errno = 0;
    check(opendir("missing") == NULL && errno == ENOENT, "ENOENT");
    errno = 0;
    check(opendir("missing/sub") == NULL && errno == ENOENT, "ENOENT in the prefix");
    errno = 0;
    check(opendir("dangling") == NULL && errno == ENOENT, "ENOENT through a dangling link");
    errno = 0;
    check(opendir("a") == NULL && errno == ENOTDIR, "ENOTDIR");
    errno = 0;
    check(opendir("a/sub") == NULL && errno == ENOTDIR, "ENOTDIR in the prefix");
    errno = 0;
    check(opendir("lnk") == NULL && errno == ENOTDIR, "ENOTDIR through a link");
    errno = 0;
    check(opendir("loop") == NULL && errno == ELOOP, "ELOOP");
    memset(longname, 'n', NAME_MAX + 1);
    longname[NAME_MAX + 1] = '\0';
    errno = 0;
    check(opendir(longname) == NULL && errno == ENAMETOOLONG, "ENAMETOOLONG");

    if (geteuid() != 0) {
        /* Permissions don't apply to root */
        check(chmod("sub", 0300) == 0, "chmod");
        errno = 0;
        check(opendir("sub") == NULL && errno == EACCES, "EACCES without read permission");
        check(chmod("sub", 0700) == 0 && mkdir("sub/inner", 0700) == 0 && chmod("sub", 0600) == 0,
              "chmod");
        errno = 0;
        check(opendir("sub/inner") == NULL && errno == EACCES, "EACCES without search permission");
        check(chmod("sub", 0700) == 0 && rmdir("sub/inner") == 0, "cleanup");
    } else {
        printf("running as root, skipping the permission checks\n");
    }

    check(unlink("a") == 0 && unlink("new") == 0 && unlink("lnk") == 0 && unlink("dangling") == 0
              && unlink("loop") == 0 && unlink("lnkdir") == 0 && rmdir("sub") == 0,
          "remove");
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
