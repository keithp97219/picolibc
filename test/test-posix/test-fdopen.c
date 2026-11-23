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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * fdopen: the stream starts at the descriptor's offset, the descriptor's
 * flags are kept, 'e' sets FD_CLOEXEC, and nothing is truncated or moved
 */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-fdopen.txt"
#endif

#define NAME TEST_FILE_NAME

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(NAME);                        \
            exit(1);                                   \
        }                                              \
    } while (0)

#define DATA "0123456789"

static int
open_data(int flags)
{
    int fd = open(NAME, flags);

    check(fd >= 0, "open data");
    return fd;
}

int
main(void)
{
    FILE       *f;
    int         fd;
    struct stat st;

    fd = open(NAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0 && write(fd, DATA, 10) == 10 && close(fd) == 0, "make data");

    /* The position starts at the descriptor's offset */
    fd = open_data(O_RDONLY);
    check(lseek(fd, 4, SEEK_SET) == 4, "lseek");
    f = fdopen(fd, "r");
    check(f != NULL, "fdopen r");
    check(ftell(f) == 4, "ftell starts at the offset");
    check(fgetc(f) == '4', "read at the offset");
    check(ftell(f) == 5, "ftell after a read");
    /* fflush moves the descriptor to the stream's position */
    check(fflush(f) == 0 && lseek(fd, 0, SEEK_CUR) == 5, "fflush sets the offset");
    /* A seek back into data that was buffered */
    check(fgetc(f) == '5' && fseek(f, 4, SEEK_SET) == 0 && fgetc(f) == '4',
          "seek within the buffer");
    check(!(fcntl(fd, F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC without e");
    check(fclose(f) == 0, "fclose");

    /* 'e' sets FD_CLOEXEC */
    fd = open_data(O_RDONLY);
    f = fdopen(fd, "re");
    check(f != NULL, "fdopen re");
#ifndef __GLIBC__
    /* glibc ignores 'e' in fdopen */
    check(fcntl(fd, F_GETFD) & FD_CLOEXEC, "e sets FD_CLOEXEC");
#endif
    check(fclose(f) == 0, "fclose re");

    /* 'w' doesn't truncate, and writes at the offset */
    fd = open_data(O_RDWR);
    check(lseek(fd, 2, SEEK_SET) == 2, "lseek");
    f = fdopen(fd, "w");
    check(f != NULL, "fdopen w");
    check(fstat(fd, &st) == 0 && st.st_size == 10, "no truncation");
    check(fputs("ab", f) >= 0 && fclose(f) == 0, "write at the offset");
    fd = open_data(O_RDONLY);
    f = fdopen(fd, "r");
    {
        char buf[16];
        check(f != NULL && fgets(buf, sizeof(buf), f) != NULL, "read back");
        check(strcmp(buf, "01ab456789") == 0, "written at the offset");
    }
    check(fclose(f) == 0, "fclose");

    /* 'a' on a descriptor without O_APPEND leaves the offset alone */
    fd = open_data(O_WRONLY);
    check(lseek(fd, 3, SEEK_SET) == 3, "lseek");
    f = fdopen(fd, "a");
    check(f != NULL, "fdopen a");
#ifndef __GLIBC__
    /* glibc seeks to the end */
    check(lseek(fd, 0, SEEK_CUR) == 3, "fdopen a doesn't move the offset");
    check(ftell(f) == 3, "ftell a");
#endif
    check(fclose(f) == 0, "fclose a");

    /* fopen with 'a' still starts at the end */
    f = fopen(NAME, "a");
    check(f != NULL && ftell(f) == 10, "fopen a starts at the end");
    check(fclose(f) == 0, "fclose fopen a");

#ifdef __PICOLIBC__
    /* An invalid descriptor is a may-fail; picolibc checks it */
    errno = 0;
    f = fdopen(-1, "r");
    check(f == NULL && errno == EBADF, "EBADF");
#endif

    check(unlink(NAME) == 0, "cleanup");
    printf("success\n");
    return 0;
}
