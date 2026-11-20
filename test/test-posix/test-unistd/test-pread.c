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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "PREAD.TXT"
#endif

/* Positioned reads and writes, syncing and posix_close */

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
    char  buf[16];
    off_t big = ((off_t)1 << 32) + 8;
    int   fd;

    (void)unlink(TEST_FILE_NAME);
    fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");
    check(write(fd, "0123456789", 10) == 10, "write");

    /* Positioned I/O leaves the file offset alone */
    check(pread(fd, buf, 4, 3) == 4 && memcmp(buf, "3456", 4) == 0, "pread");
    check(pwrite(fd, "ab", 2, 5) == 2, "pwrite");
    check(lseek(fd, 0, SEEK_CUR) == 10, "offset unchanged");
    check(pread(fd, buf, 10, 0) == 10 && memcmp(buf, "01234ab789", 10) == 0, "pwrite result");
    check(pread(fd, buf, 4, 20) == 0, "pread past the end");
    check(pread(fd, buf, 4, -1) == -1 && errno == EINVAL, "pread negative offset");

    /* Offsets beyond 32 bits */
    if (pwrite(fd, "high", 4, big) == 4) {
        memset(buf, 0, sizeof(buf));
        check(pread(fd, buf, 4, big) == 4 && memcmp(buf, "high", 4) == 0, "pread above 4GiB");
    } else {
        check(errno == EFBIG || errno == EINVAL, "pwrite above 4GiB");
    }

    check(fsync(fd) == 0, "fsync");
    check(fdatasync(fd) == 0, "fdatasync");

#ifdef POSIX_CLOSE_RESTART
    check(posix_close(fd, -1) == -1 && errno == EINVAL, "posix_close bad flag");
    check(posix_close(fd, POSIX_CLOSE_RESTART) == 0, "posix_close");
    check(posix_close(fd, 0) == -1 && errno == EBADF, "posix_close closed descriptor");
#else
    /* posix_close is new in POSIX.1-2024 */
    check(close(fd) == 0, "close");
#endif
    check(unlink(TEST_FILE_NAME) == 0, "unlink");

    printf("success\n");
    return 0;
}
