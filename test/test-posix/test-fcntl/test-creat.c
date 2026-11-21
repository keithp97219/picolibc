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
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* creat opens a file for writing, creating it or truncating it */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char        dir[] = "creat-XXXXXX";
    char        path[64], buf[8];
    struct stat st;
    mode_t      old = umask(022);
    int         fd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/file", dir);

    fd = creat(path, 0644);
    check(fd >= 0, "create");
    check(fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0777) == 0644, "mode");
    check(write(fd, "hello", 5) == 5, "write");
    /* Write only */
    errno = 0;
    check(read(fd, buf, sizeof(buf)) == -1 && errno == EBADF, "not readable");
    check((fcntl(fd, F_GETFL) & O_ACCMODE) == O_WRONLY, "O_WRONLY");
    check(close(fd) == 0, "close");

    /* An existing file is truncated, keeping its mode */
    fd = creat(path, 0600);
    check(fd >= 0, "truncate");
    check(fstat(fd, &st) == 0 && st.st_size == 0 && (st.st_mode & 0777) == 0644, "truncated");
    check(close(fd) == 0, "close again");

    errno = 0;
    snprintf(path, sizeof(path), "%s/missing/file", dir);
    check(creat(path, 0644) == -1 && errno == ENOENT, "ENOENT");
    snprintf(path, sizeof(path), "%s/file", dir);
    check(unlink(path) == 0 && rmdir(dir) == 0, "clean up");
    umask(old);
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
