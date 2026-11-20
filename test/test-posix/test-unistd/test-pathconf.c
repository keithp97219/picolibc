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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                          \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>

/* pathconf and fpathconf report limits of at least the POSIX minimums */

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
    struct statvfs sv;
    int            fds[2], fd;
    long           v;

    /* A limit with no maximum is -1 without changing errno */
    errno = 0;
    v = pathconf(".", _PC_NAME_MAX);
    check(v >= _POSIX_NAME_MAX || (v == -1 && errno == 0), "_PC_NAME_MAX");
    v = pathconf(".", _PC_PATH_MAX);
    check(v >= _POSIX_PATH_MAX || (v == -1 && errno == 0), "_PC_PATH_MAX");
    v = pathconf(".", _PC_LINK_MAX);
    check(v >= _POSIX_LINK_MAX || (v == -1 && errno == 0), "_PC_LINK_MAX");
    v = pathconf(".", _PC_NO_TRUNC);
    check(v != 0 || errno == 0, "_PC_NO_TRUNC");

    /* The name limit is the file system's */
    check(statvfs(".", &sv) == 0, "statvfs");
    check(pathconf(".", _PC_NAME_MAX) == (long)sv.f_namemax, "f_namemax");

    fd = open(".", O_RDONLY);
    check(fd >= 0, "open .");
    check(fpathconf(fd, _PC_NAME_MAX) == pathconf(".", _PC_NAME_MAX), "fpathconf agrees");
    check(close(fd) == 0, "close");

    check(pipe(fds) == 0, "pipe");
    v = fpathconf(fds[0], _PC_PIPE_BUF);
    check(v >= _POSIX_PIPE_BUF || (v == -1 && errno == 0), "_PC_PIPE_BUF");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");

    errno = 0;
    check(pathconf(".", -1) == -1 && errno == EINVAL, "bad name");
    errno = 0;
    check(fpathconf(-1, _PC_NAME_MAX) == -1 && errno == EBADF, "EBADF");
    errno = 0;
    check(pathconf("no/such/path", _PC_NAME_MAX) == -1 && errno == ENOENT, "ENOENT");
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
