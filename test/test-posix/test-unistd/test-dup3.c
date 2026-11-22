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
/* glibc declares dup3 only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FD_MGMT && TEST_POSIX_FILE_SYSTEM

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "DUP3.TXT"
#endif

/* dup3 sets FD_CLOEXEC on request and rejects other flags */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);              \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    int fd, fd2;

    (void)unlink(TEST_FILE_NAME);
    fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");

    fd2 = dup3(fd, 100, O_CLOEXEC);
    check(fd2 == 100, "dup3");
    check(fcntl(fd2, F_GETFD) & FD_CLOEXEC, "dup3 O_CLOEXEC");
    check(close(fd2) == 0, "close dup3");
    errno = 0;
    check(dup3(fd, fd, 0) == -1 && errno == EINVAL, "dup3 to the same descriptor");
    errno = 0;
    check(dup3(fd, 100, O_APPEND) == -1 && errno == EINVAL, "dup3 bad flags");

    check(close(fd) == 0, "close");
    check(unlink(TEST_FILE_NAME) == 0, "unlink");

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
