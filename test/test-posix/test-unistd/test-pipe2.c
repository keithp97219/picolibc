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

/* glibc declares pipe2 only for _GNU_SOURCE */
#define _GNU_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                  \
    || ((defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* pipe2 makes pipes with O_CLOEXEC and O_NONBLOCK */

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
    int  fds[2];
    char buf[4];

    check(pipe2(fds, 0) == 0, "pipe2");
    check(write(fds[1], "abc", 3) == 3 && read(fds[0], buf, 3) == 3, "data");
    check(!(fcntl(fds[0], F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    check(pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0, "pipe2 flags");
    check(fcntl(fds[0], F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC read");
    check(fcntl(fds[1], F_GETFD) & FD_CLOEXEC, "FD_CLOEXEC write");
    check(fcntl(fds[0], F_GETFL) & O_NONBLOCK, "O_NONBLOCK");
    check(read(fds[0], buf, 1) == -1 && errno == EAGAIN, "EAGAIN");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    check(pipe2(fds, O_TRUNC) == -1 && errno == EINVAL, "bad flag");
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
