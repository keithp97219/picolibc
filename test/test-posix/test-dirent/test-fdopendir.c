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
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* fdopendir reads a directory open on a descriptor */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

int
main(void)
{
    struct dirent *ent;
    DIR           *d;
    int            dfd, fd, found = 0;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    fd = openat(dfd, "file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0, "create file");

    check(fdopendir(fd) == NULL && errno == ENOTDIR, "ENOTDIR");
    check(close(fd) == 0, "close");

    d = fdopendir(dfd);
    check(d != NULL, "fdopendir");
    while ((ent = readdir(d)) != NULL)
        if (strcmp(ent->d_name, "file") == 0)
            found = 1;
    check(found, "entry");
    check(closedir(d) == 0, "closedir");
    errno = 0;
    check(fcntl(dfd, F_GETFD) == -1 && errno == EBADF, "closedir closes the descriptor");

    /* Not a descriptor */
    errno = 0;
    check(fdopendir(-1) == NULL && errno == EBADF, "EBADF for -1");
    errno = 0;
    check(fdopendir(dfd) == NULL && errno == EBADF, "EBADF for a closed descriptor");

#ifdef O_SEARCH
    /* A descriptor open only for searching can't be read */
    dfd = open(dir, O_SEARCH | O_DIRECTORY);
    check(dfd >= 0, "open dir O_SEARCH");
    errno = 0;
    check(fdopendir(dfd) == NULL && errno == EBADF, "EBADF for O_SEARCH");
    check(close(dfd) == 0, "close");
#endif

    /* FD_CLOEXEC stays set */
    dfd = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    check(dfd >= 0, "open dir O_CLOEXEC");
    d = fdopendir(dfd);
    check(d != NULL, "fdopendir");
    check(fcntl(dfd, F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC not cleared");
    check(closedir(d) == 0, "closedir");

    /* The descriptor's offset picks the entries; where the end can be sought, none are left */
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    if (lseek(dfd, 0, SEEK_END) > 0) {
        d = fdopendir(dfd);
        check(d != NULL, "fdopendir");
        errno = 0;
        check(readdir(d) == NULL && errno == 0, "the offset decides the entries");
        check(closedir(d) == 0, "closedir");
    } else {
        check(close(dfd) == 0, "close");
    }

    check(unlink("file") == -1, "relative to cwd");
    check(chdir(dir) == 0 && unlink("file") == 0 && chdir("..") == 0, "cleanup");
    check(rmdir(dir) == 0, "rmdir");
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
