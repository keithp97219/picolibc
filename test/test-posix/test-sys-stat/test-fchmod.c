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
#if TEST_POSIX_FILE_ATTRIBUTES && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* fchmod changes the mode of an open file */

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
    struct stat st;
    int         fd, fd2;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(chdir(dir) == 0, "chdir");
    fd = open("file", O_WRONLY | O_CREAT, 0600);
    check(fd >= 0, "create file");
    check(fchmod(fd, 0604) == 0, "fchmod");
    check(fstat(fd, &st) == 0 && (st.st_mode & 07777) == 0604, "mode");
    /* S_ISUID can be set, and the file type bits are ignored */
    check(fchmod(fd, S_IFDIR | S_ISUID | 0700) == 0, "fchmod S_ISUID");
    check(fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 07777) == (S_ISUID | 0700),
          "S_ISUID set, type kept");
    /* A descriptor open only for reading is enough */
    fd2 = open("file", O_RDONLY);
    check(fd2 >= 0 && fchmod(fd2, 0600) == 0 && fstat(fd, &st) == 0 && (st.st_mode & 07777) == 0600,
          "fchmod read-only descriptor");
    check(close(fd2) == 0, "close read-only descriptor");
    /* Only the owner can change the mode; root can change anything */
    if (geteuid() != 0) {
        fd2 = open("/", O_RDONLY);
        check(fd2 >= 0 && fstat(fd2, &st) == 0, "open /");
        if (st.st_uid != geteuid()) {
            errno = 0;
            check(fchmod(fd2, st.st_mode & 07777) == -1 && errno == EPERM, "EPERM");
        }
        check(close(fd2) == 0, "close /");
    }
    check(close(fd) == 0, "close");
    errno = 0;
    check(fchmod(fd, 0600) == -1 && errno == EBADF, "EBADF");
    check(unlink("file") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
