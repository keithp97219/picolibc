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
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_IPC)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>

/* ftok makes keys from a path and an id */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "/tmp/ftok-XXXXXX";
static char names[2][sizeof(dir) + 64];
static int  which;

/* A name in the directory; two may be in use at once */
static const char *
in_dir(const char *file)
{
    char *name = names[which];

    which ^= 1;
    snprintf(name, sizeof(names[0]), "%s/%s", dir, file);
    return name;
}

static void
cleanup(void)
{
    static const char * const files[]
        = { "file", "other", "link", "symlink", "loop1", "loop2", "big", "locked/file", "locked" };
    size_t i;

    chmod(in_dir("locked"), 0700);
    for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        const char *f = in_dir(files[i]);

        if (strcmp(files[i], "locked") == 0)
            rmdir(f);
        else
            unlink(f);
    }
    rmdir(dir);
}

static void
make_file(const char *file)
{
    int fd = open(in_dir(file), O_WRONLY | O_CREAT | O_EXCL, 0600);

    check(fd >= 0, "create");
    close(fd);
}

static key_t
ftok_in(const char *file, int id)
{
    return ftok(in_dir(file), id);
}

int
main(void)
{
    char  path[sizeof(names[0])], longname[NAME_MAX + 2];
    key_t a;
    int   fd;

    a = ftok(".", 1);
    check(a != (key_t)-1, "ftok");
    check(ftok(".", 1) == a, "stable");
    check(ftok(".", 2) != a, "depends on the id");
    check(ftok("no-such-file-here", 1) == (key_t)-1 && errno == ENOENT, "ENOENT");

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(atexit(cleanup) == 0, "atexit");
    make_file("file");
    make_file("other");
    a = ftok_in("file", 'k');
    check(a != (key_t)-1, "ftok");

    /* Every path naming the same file gives the same key */
    check(link(in_dir("file"), in_dir("link")) == 0, "link");
    check(symlink("file", in_dir("symlink")) == 0, "symlink");
    check(ftok_in("link", 'k') == a, "a hard link");
    check(ftok_in("symlink", 'k') == a, "a symbolic link");
    snprintf(path, sizeof(path), "%s/./../%s/file", dir, strrchr(dir, '/') + 1);
    check(ftok(path, 'k') == a, "another spelling of the path");

    /* Only the low 8 bits of id matter */
    check(ftok_in("file", 'k' | 0x100) == a, "id bits above 8 are ignored");
    check(ftok_in("file", 'k' | 0x7f00) == a, "id bits above 8 are ignored");

    /* Different ids and different files should give different keys */
    check(ftok_in("file", 'j') != a, "another id");
    check(ftok_in("other", 'k') != a, "another file");

    /* A file too big for stat's st_size in some environments still works */
    fd = open(in_dir("big"), O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");
    if (ftruncate(fd, (off_t)3 << 30) == 0) {
        check(ftok_in("big", 'k') != (key_t)-1, "a file over 2GiB");
    } else {
        printf("can't make a 3GiB file: not checking it\n");
    }
    close(fd);

    /* The errors from looking up the path */
    check(ftok("", 'k') == (key_t)-1 && errno == ENOENT, "ENOENT for an empty path");
    check(ftok_in("missing", 'k') == (key_t)-1 && errno == ENOENT, "ENOENT");
    check(ftok_in("file/x", 'k') == (key_t)-1 && errno == ENOTDIR, "ENOTDIR for a prefix");
    check(ftok_in("file/", 'k') == (key_t)-1 && errno == ENOTDIR, "ENOTDIR for a trailing slash");
    check(symlink("loop2", in_dir("loop1")) == 0 && symlink("loop1", in_dir("loop2")) == 0,
          "symlink");
    check(ftok_in("loop1", 'k') == (key_t)-1 && errno == ELOOP, "ELOOP");
    memset(longname, 'n', NAME_MAX + 1);
    longname[NAME_MAX + 1] = '\0';
    check(ftok(longname, 'k') == (key_t)-1 && errno == ENAMETOOLONG, "ENAMETOOLONG");
    if (geteuid() != 0) {
        check(mkdir(in_dir("locked"), 0700) == 0, "mkdir");
        make_file("locked/file");
        check(chmod(in_dir("locked"), 0) == 0, "chmod");
        check(ftok_in("locked/file", 'k') == (key_t)-1 && errno == EACCES, "EACCES");
    } else {
        printf("running as root: not checking EACCES\n");
    }

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
