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
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FD_MGMT)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* fopen: the mode table, 'e' and 'x', creation and the errors */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
make_file(const char *name, const char *contents)
{
    FILE *f = fopen(name, "w");

    check(f != NULL && fputs(contents, f) >= 0 && fclose(f) == 0, "make file");
}

static void
check_contents(const char *name, const char *contents)
{
    char   buf[64];
    FILE  *f = fopen(name, "r");
    size_t len;

    check(f != NULL, "open to read back");
    len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    check(fclose(f) == 0, "close after read back");
    if (strcmp(buf, contents) != 0)
        printf("%s holds \"%s\", expected \"%s\"\n", name, buf, contents);
    check(strcmp(buf, contents) == 0, "contents");
}

/* Each mode opens the descriptor with the flags in the table */
static const struct {
    const char *mode;
    int         accmode;
    int         append;
    int         truncates;
} modes[] = {
    { "r",   O_RDONLY, 0, 0 },
    { "w",   O_WRONLY, 0, 1 },
    { "a",   O_WRONLY, 1, 0 },
    { "r+",  O_RDWR,   0, 0 },
    { "w+",  O_RDWR,   0, 1 },
    { "a+",  O_RDWR,   1, 0 },
    { "rb",  O_RDONLY, 0, 0 },
    { "wb",  O_WRONLY, 0, 1 },
    { "ab",  O_WRONLY, 1, 0 },
    { "rb+", O_RDWR,   0, 0 },
    { "w+b", O_RDWR,   0, 1 },
    { "a+b", O_RDWR,   1, 0 },
};

int
main(void)
{
    char        dir[] = "fopen-XXXXXX";
    FILE       *f;
    struct stat st;
    size_t      i;
    int         fl;
    char        longname[NAME_MAX + 2];

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    umask(022);

    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        make_file("modes", "old");
        f = fopen("modes", modes[i].mode);
        check(f != NULL, modes[i].mode);
        fl = fcntl(fileno(f), F_GETFL);
        check((fl & O_ACCMODE) == modes[i].accmode, "access mode");
        check(!!(fl & O_APPEND) == modes[i].append, "O_APPEND");
        check(!(fcntl(fileno(f), F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC without e");
        check(!ferror(f) && !feof(f), "indicators clear");
        check(fstat(fileno(f), &st) == 0 && (st.st_size == 0) == modes[i].truncates, "truncation");
        check(fclose(f) == 0, "close");
    }

    /* 'a' writes at the end, whatever fseek did */
    make_file("append", "abc");
    f = fopen("append", "a+");
    check(f != NULL, "a+");
    check(fseek(f, 0, SEEK_SET) == 0 && fgetc(f) == 'a', "read from the start");
    check(fseek(f, 1, SEEK_SET) == 0 && fputs("XY", f) >= 0 && fclose(f) == 0, "write");
    check_contents("append", "abcXY");

    /* Creation uses S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH less the umask */
    f = fopen("created", "w");
    check(f != NULL && fstat(fileno(f), &st) == 0, "create");
    check((st.st_mode & 0777) == 0644, "creation mode");
    check(fclose(f) == 0, "close created");

    /* x with w or a: O_EXCL */
    f = fopen("excl", "wxe");
    check(f != NULL, "wx creates");
    check(fcntl(fileno(f), F_GETFD) & FD_CLOEXEC, "e sets FD_CLOEXEC");
    check(fclose(f) == 0, "close excl");
    errno = 0;
    check(fopen("excl", "wx") == NULL && errno == EEXIST, "wx fails when the file exists");
    errno = 0;
    check(fopen("excl", "w+x") == NULL && errno == EEXIST, "w+x");
    errno = 0;
    check(fopen("excl", "ax") == NULL && errno == EEXIST, "ax fails when the file exists");
    errno = 0;
    check(fopen("excl", "a+x") == NULL && errno == EEXIST, "a+x");
    f = fopen("excl2", "ax");
    check(f != NULL && fclose(f) == 0, "ax creates");
    f = fopen("excl", "re");
    check(f != NULL && (fcntl(fileno(f), F_GETFD) & FD_CLOEXEC), "re");
    check(fclose(f) == 0, "close re");

    /* Errors */
    errno = 0;
    check(fopen("missing", "r") == NULL && errno == ENOENT, "ENOENT for r");
    errno = 0;
    check(fopen("missing/x", "w") == NULL && errno == ENOENT, "ENOENT in the prefix");
    errno = 0;
    check(fopen("", "r") == NULL && errno == ENOENT, "ENOENT for an empty path");
    errno = 0;
    check(fopen(".", "w") == NULL && errno == EISDIR, "EISDIR");
    errno = 0;
    check(fopen("created/", "r") == NULL && (errno == ENOTDIR || errno == ENOENT),
          "trailing slash on a file");
    errno = 0;
    check(fopen("created/x", "w") == NULL && errno == ENOTDIR, "ENOTDIR");
    check(symlink("loop", "loop") == 0, "symlink");
    errno = 0;
    check(fopen("loop", "r") == NULL && errno == ELOOP, "ELOOP");
    memset(longname, 'n', sizeof(longname) - 1);
    longname[sizeof(longname) - 1] = '\0';
    errno = 0;
    check(fopen(longname, "w") == NULL && errno == ENAMETOOLONG, "ENAMETOOLONG");
    if (geteuid() != 0) {
        check(chmod("created", 0) == 0, "chmod");
        errno = 0;
        check(fopen("created", "r") == NULL && errno == EACCES, "EACCES");
    }

    check(unlink("modes") == 0 && unlink("append") == 0 && unlink("created") == 0
              && unlink("excl") == 0 && unlink("excl2") == 0 && unlink("loop") == 0,
          "cleanup");
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
