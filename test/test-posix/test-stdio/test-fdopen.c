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
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * fdopen: the stream starts at the descriptor's offset, the descriptor's
 * flags are kept, 'e' sets FD_CLOEXEC, and nothing is truncated or moved
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define DATA "0123456789"

static int
open_data(int flags)
{
    int fd = open("data", flags);

    check(fd >= 0, "open data");
    return fd;
}

int
main(void)
{
    char        dir[] = "fdopen-XXXXXX";
    FILE       *f;
    int         fd;
    struct stat st;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("data", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0 && write(fd, DATA, 10) == 10 && close(fd) == 0, "make data");

    /* The position starts at the descriptor's offset */
    fd = open_data(O_RDONLY);
    check(lseek(fd, 4, SEEK_SET) == 4, "lseek");
    f = fdopen(fd, "r");
    check(f != NULL, "fdopen r");
    check(!ferror(f) && !feof(f), "indicators clear");
    check(fileno(f) == fd, "fileno");
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
    errno = 0;
    check(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "fclose closes the descriptor");

    /* 'e' sets FD_CLOEXEC */
    fd = open_data(O_RDONLY);
    f = fdopen(fd, "re");
    check(f != NULL, "fdopen re");
#ifndef __GLIBC__
    /* glibc ignores 'e' in fdopen */
    check(fcntl(fd, F_GETFD) & FD_CLOEXEC, "e sets FD_CLOEXEC");
#endif
    check(fclose(f) == 0, "fclose re");

    /* Without 'e', a set FD_CLOEXEC stays set */
    fd = open("data", O_RDONLY | O_CLOEXEC);
    check(fd >= 0, "open O_CLOEXEC");
    f = fdopen(fd, "r");
    check(f != NULL && (fcntl(fd, F_GETFD) & FD_CLOEXEC), "FD_CLOEXEC unchanged");
    check(fclose(f) == 0, "fclose");

    /* 'w' doesn't truncate, and 'x' has no effect */
    fd = open_data(O_RDWR);
    check(lseek(fd, 2, SEEK_SET) == 2, "lseek");
    f = fdopen(fd, "wx");
    check(f != NULL, "fdopen wx");
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

    /* O_APPEND on the descriptor stays set, whatever the mode */
    fd = open_data(O_RDWR | O_APPEND);
    f = fdopen(fd, "r+");
    check(f != NULL && (fcntl(fd, F_GETFL) & O_APPEND), "O_APPEND kept");
    check(fputs("Z", f) >= 0 && fflush(f) == 0, "append");
    check(fstat(fd, &st) == 0 && st.st_size == 11, "written at the end");
    check(fclose(f) == 0, "fclose r+");

#ifdef __PICOLIBC__
    /* An invalid descriptor is a may-fail; picolibc checks it */
    errno = 0;
    f = fdopen(-1, "r");
    check(f == NULL && errno == EBADF, "EBADF");
#endif

    check(unlink("data") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
