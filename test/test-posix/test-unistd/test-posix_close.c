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
#if TEST_POSIX_DEVICE_IO >= 202405L && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE \
    && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * close and posix_close: the descriptor is freed for reuse, pipes see
 * end-of-file, and an invalid flag still closes
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("file");
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

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

int
main(void)
{
    char c;
    int  fd, fd2, p[2];

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* close frees the descriptor for the next open */
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");
    fd2 = dup(fd);
    check(fd2 > fd, "dup");
    check(close(fd) == 0, "close");
    check(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "closed");
    check(open("file", O_RDONLY) == fd, "lowest descriptor reused");
    check(close(fd) == 0 && close(fd2) == 0, "close");
    check_err(close(fd), EBADF, "close of a closed descriptor");
    check_err(close(-1), EBADF, "close(-1)");

    /* Closing the last write end gives the reader end-of-file */
    check(pipe(p) == 0, "pipe");
    fd2 = dup(p[1]);
    check(fd2 >= 0 && close(p[1]) == 0, "close one write end");
    check(write(fd2, "x", 1) == 1 && read(p[0], &c, 1) == 1, "pipe still open for writing");
    check(close(fd2) == 0 && read(p[0], &c, 1) == 0, "end-of-file after the last close");
    check(close(p[0]) == 0, "close");

#ifdef POSIX_CLOSE_RESTART /* defined with posix_close; older C libraries lack both */
    /* posix_close with 0 or POSIX_CLOSE_RESTART closes */
    fd = open("file", O_RDONLY);
    check(fd >= 0, "open");
    check(posix_close(fd, 0) == 0 && fcntl(fd, F_GETFD) == -1, "posix_close(fd, 0)");
    check_err(posix_close(fd, 0), EBADF, "posix_close of a closed descriptor");
    fd = open("file", O_RDONLY);
    check(fd >= 0, "open");
    check(posix_close(fd, POSIX_CLOSE_RESTART) == 0 && fcntl(fd, F_GETFD) == -1,
          "posix_close(fd, POSIX_CLOSE_RESTART)");

    /* An invalid flag may give EINVAL, but the descriptor is closed as for 0 */
    fd = open("file", O_RDONLY);
    check(fd >= 0, "open");
    errno = 0;
    check(posix_close(fd, POSIX_CLOSE_RESTART | 0x40000000) == 0 || errno == EINVAL,
          "posix_close with an invalid flag");
    check(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "closed despite the invalid flag");
    check_err(posix_close(fd, 0x40000000), EBADF, "invalid flag on a closed descriptor");
#endif

    cleanup();
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
