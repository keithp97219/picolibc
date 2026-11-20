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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* fcntl reports and changes descriptor and file status flags */

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
    int fds[2], fd, flags;

    check(pipe(fds) == 0, "pipe");

    flags = fcntl(fds[0], F_GETFL);
    check(flags >= 0, "F_GETFL");
    check((flags & O_ACCMODE) == O_RDONLY, "read end access mode");
    check((fcntl(fds[1], F_GETFL) & O_ACCMODE) == O_WRONLY, "write end access mode");
    check(!(flags & O_NONBLOCK), "blocking");

    check(fcntl(fds[0], F_SETFL, flags | O_NONBLOCK) == 0, "F_SETFL");
    check(fcntl(fds[0], F_GETFL) & O_NONBLOCK, "O_NONBLOCK set");
    check(read(fds[0], &flags, 1) == -1 && errno == EAGAIN, "non-blocking read");

    check(fcntl(fds[1], F_SETFL, O_APPEND) == 0, "F_SETFL O_APPEND");
    check(fcntl(fds[1], F_GETFL) & O_APPEND, "O_APPEND set");

    check(!(fcntl(fds[0], F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC");
    check(fcntl(fds[0], F_SETFD, FD_CLOEXEC) == 0, "F_SETFD");
    check(fcntl(fds[0], F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC set");

    fd = fcntl(fds[1], F_DUPFD, 10);
    check(fd >= 10, "F_DUPFD");
    check(!(fcntl(fd, F_GETFD) & FD_CLOEXEC), "F_DUPFD clears FD_CLOEXEC");
    check(close(fd) == 0, "close");
    fd = fcntl(fds[1], F_DUPFD_CLOEXEC, 10);
    check(fd >= 10 && (fcntl(fd, F_GETFD) & FD_CLOEXEC), "F_DUPFD_CLOEXEC");
    check(close(fd) == 0, "close");

    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    check(fcntl(fds[0], F_GETFL) == -1 && errno == EBADF, "EBADF");
    printf("success\n");
    return 0;
}
