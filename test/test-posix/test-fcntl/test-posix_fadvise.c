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
#if _POSIX_ADVISORY_INFO > 0 && defined(__PICOLIBC_POSIX_FILE_SYSTEM)              \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_FIFO)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/* posix_fadvise accepts advice about file access */

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
    static const int advice[] = {
        POSIX_FADV_NORMAL,   POSIX_FADV_RANDOM,   POSIX_FADV_SEQUENTIAL,
        POSIX_FADV_WILLNEED, POSIX_FADV_DONTNEED, POSIX_FADV_NOREUSE,
    };
    unsigned i;
    int      fd, fds[2];

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(chdir(dir) == 0, "chdir");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0 && write(fd, "data", 4) == 4, "create file");
    for (i = 0; i < sizeof(advice) / sizeof(advice[0]); i++)
        check(posix_fadvise(fd, 0, 0, advice[i]) == 0, "advice");
    check(posix_fadvise(fd, 0, 4, 1000) == EINVAL, "bad advice");
    check(posix_fadvise(-1, 0, 0, POSIX_FADV_NORMAL) == EBADF, "EBADF");
    check(pipe(fds) == 0, "pipe");
    check(posix_fadvise(fds[0], 0, 0, POSIX_FADV_NORMAL) == ESPIPE, "ESPIPE");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");
    /* A range past the end of the file is fine; a negative length isn't */
    check(posix_fadvise(fd, 1000000, 4096, POSIX_FADV_WILLNEED) == 0, "past the end");
    check(posix_fadvise(fd, 0, -1, POSIX_FADV_NORMAL) == EINVAL, "negative length");
    /* A FIFO is ESPIPE too */
    check(mkfifo("fifo", 0600) == 0, "mkfifo");
    fds[0] = open("fifo", O_RDONLY | O_NONBLOCK);
    check(fds[0] >= 0, "open FIFO");
    check(posix_fadvise(fds[0], 0, 0, POSIX_FADV_NORMAL) == ESPIPE, "FIFO ESPIPE");
    check(close(fds[0]) == 0 && unlink("fifo") == 0, "close FIFO");
    /* errno isn't the way errors are reported */
    errno = 0;
    check(posix_fadvise(-1, 0, 0, POSIX_FADV_NORMAL) == EBADF && errno == 0, "errno untouched");
    check(close(fd) == 0 && unlink("file") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
