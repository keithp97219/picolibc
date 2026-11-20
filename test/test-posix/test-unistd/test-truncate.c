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

#define _GNU_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                              \
    || ((defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)             \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0))

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/* Past what a 32-bit offset can hold; files are sparse, so this costs no space */
#define BIG_SIZE ((off_t)5 * 1024 * 1024 * 1024)

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "TRUNCATE.TXT"
#endif

/* truncate and ftruncate set sizes past what a 32-bit offset holds */

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
    struct stat st;
    int         fd;

    (void)unlink(TEST_FILE_NAME);
    fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");

    check(ftruncate(fd, BIG_SIZE) == 0, "ftruncate past 4 GiB");
    check(fstat(fd, &st) == 0 && st.st_size == BIG_SIZE, "size after ftruncate");
    check(truncate(TEST_FILE_NAME, 12345) == 0, "truncate");
    check(fstat(fd, &st) == 0 && st.st_size == 12345, "size after truncate");
    check(truncate(TEST_FILE_NAME, BIG_SIZE + 1) == 0, "truncate past 4 GiB");
    check(stat(TEST_FILE_NAME, &st) == 0 && st.st_size == BIG_SIZE + 1, "size after big truncate");
    check(ftruncate(fd, 0) == 0, "ftruncate to zero");

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
