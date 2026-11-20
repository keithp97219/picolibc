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
/* telldir and seekdir are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* telldir and seekdir return to positions in a directory, and rewinddir to its start */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

/*
 * A position that doesn't fit in a 32-bit long still works. tmpfs lets
 * a directory's offset be set anywhere, and fdopendir starts from it.
 */
static void
big_offset(void)
{
    char  shm[] = "/dev/shm/telldir-XXXXXX";
    off_t big = (off_t)1 << 32;
    long  pos;
    DIR  *d;
    int   fd;

    if (sizeof(off_t) < 8 || mkdtemp(shm) == NULL) {
        printf("no 64-bit offsets or /dev/shm, skipping the large position check\n");
        return;
    }
    fd = open(shm, O_RDONLY | O_DIRECTORY);
    check(fd >= 0, "open");
    if (lseek(fd, big, SEEK_SET) != big) {
        printf("can't seek the directory to 2^32, skipping the large position check\n");
        check(close(fd) == 0 && rmdir(shm) == 0, "cleanup");
        return;
    }
    d = fdopendir(fd);
    check(d != NULL, "fdopendir");
    errno = 0;
    pos = telldir(d);
    check(pos != -1, "telldir at 2^32");
    check(readdir(d) == NULL, "nothing at 2^32");
    seekdir(d, pos);
    check(telldir(d) == pos, "telldir after seekdir at 2^32");
#ifdef __PICOLIBC__
    /* picolibc's seekdir sets the descriptor's offset at once */
    check(lseek(dirfd(d), 0, SEEK_CUR) == big, "seekdir back to 2^32");
#endif
    check(closedir(d) == 0 && rmdir(shm) == 0, "cleanup");
}

int
main(void)
{
    char           names[8][16], again[16];
    long           pos, positions[9];
    DIR           *d;
    struct dirent *ent;
    int            i, n = 0;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    for (i = 0; i < 6; i++) {
        char name[8];
        int  fd;

        snprintf(name, sizeof(name), "f%d", i);
        fd = open(name, O_WRONLY | O_CREAT, 0600);
        check(fd >= 0 && close(fd) == 0, "create file");
    }
    d = opendir(".");
    check(d != NULL, "opendir");
    /* Remember the position after the third entry */
    for (i = 0; i < 3; i++)
        check(readdir(d) != NULL, "readdir");
    errno = 0;
    pos = telldir(d);
#ifdef __GLIBC__
    if (pos == -1 && errno == EOVERFLOW) {
        /* glibc fails when a 32-bit program emulated on a 64-bit kernel sees 64-bit offsets */
        printf("directory offsets don't fit in long here, skipping\n");
        return 77;
    }
#endif
    check(pos != -1, "telldir");
    while ((ent = readdir(d)) != NULL && n < 8)
        strcpy(names[n++], ent->d_name);
    check(n == 5, "the rest");
    seekdir(d, pos);
    check(telldir(d) == pos, "telldir after seekdir");
    ent = readdir(d);
    check(ent && strcmp(ent->d_name, names[0]) == 0, "seekdir");

    /* Every position returns to its own entry, in any order */
    rewinddir(d);
    n = 0;
    for (;;) {
        positions[n] = telldir(d);
        check(positions[n] != -1, "telldir");
        if ((ent = readdir(d)) == NULL)
            break;
        check(n < 8, "entries");
        strcpy(names[n++], ent->d_name);
    }
    check(n == 8, "every entry");
    for (i = n - 1; i >= 0; i--) {
        seekdir(d, positions[i]);
        ent = readdir(d);
        check(ent && strcmp(ent->d_name, names[i]) == 0, "seekdir to each position");
    }
    seekdir(d, positions[n]);
    check(readdir(d) == NULL, "seekdir to the end");
    rewinddir(d);
    n = 0;
    while ((ent = readdir(d)) != NULL)
        n++;
    check(n == 8, "rewinddir starts again");
    check(closedir(d) == 0, "closedir");
    for (i = 0; i < 6; i++) {
        snprintf(again, sizeof(again), "f%d", i);
        check(unlink(again) == 0, "unlink");
    }
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    big_offset();
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
