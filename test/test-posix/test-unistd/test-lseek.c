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
/* glibc defines SEEK_DATA and SEEK_HOLE only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE && TEST_POSIX_FIFO \
    && TEST_POSIX_NETWORKING

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/* lseek: each whence, holes, and the errors leaving the offset alone */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("file");
    (void)unlink("fifo");
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
    struct stat st;
    int         fd, dfd, p[2], s[2];
    off_t       off;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");
    check(write(fd, "0123456789", 10) == 10, "write");

    /* SEEK_SET, SEEK_CUR and SEEK_END, returning the new offset */
    check(lseek(fd, 3, SEEK_SET) == 3, "SEEK_SET");
    check(lseek(fd, 2, SEEK_CUR) == 5, "SEEK_CUR");
    check(lseek(fd, -1, SEEK_CUR) == 4, "SEEK_CUR backwards");
    check(lseek(fd, -2, SEEK_END) == 8, "SEEK_END");
    check(lseek(fd, 0, SEEK_CUR) == 8, "offset kept");

    /* Past the end is allowed and doesn't extend the file */
    check(lseek(fd, 100, SEEK_END) == 110, "past the end");
    check(fstat(fd, &st) == 0 && st.st_size == 10, "lseek doesn't extend the file");

    /* SEEK_DATA and SEEK_HOLE: data from 0, the virtual hole at the end */
    check(lseek(fd, 0, SEEK_DATA) == 0, "SEEK_DATA at the start");
    check(lseek(fd, 4, SEEK_DATA) == 4, "SEEK_DATA inside data");
    off = lseek(fd, 0, SEEK_HOLE);
    check(off == 10, "SEEK_HOLE finds the end of the file");
    /* At or past the end of the file: ENXIO, offset unchanged */
    check(lseek(fd, 7, SEEK_SET) == 7, "lseek");
    check_err(lseek(fd, 10, SEEK_DATA), ENXIO, "SEEK_DATA at the end");
    check_err(lseek(fd, 10, SEEK_HOLE), ENXIO, "SEEK_HOLE at the end");
    check_err(lseek(fd, 50, SEEK_DATA), ENXIO, "SEEK_DATA past the end");
    check(lseek(fd, 0, SEEK_CUR) == 7, "ENXIO keeps the offset");

    /*
     * A sparse file: data after a hole. Zero bytes need not be a hole,
     * so the file system may report data anywhere up to the write
     */
    check(pwrite(fd, "x", 1, 1 << 20) == 1, "sparse write");
    off = lseek(fd, 0, SEEK_HOLE);
    check(off >= 10 && off <= (1 << 20) + 1, "SEEK_HOLE before the data");
    off = lseek(fd, 20, SEEK_DATA);
    check(off >= 20 && off <= 1 << 20, "SEEK_DATA after the hole");
    check_err(lseek(fd, (1 << 20) + 1, SEEK_HOLE), ENXIO, "SEEK_HOLE at the size");

    /* EINVAL: bad whence, negative result; the offset stays */
    check(lseek(fd, 7, SEEK_SET) == 7, "lseek");
    check_err(lseek(fd, 0, 42), EINVAL, "bad whence");
    check_err(lseek(fd, -1, SEEK_SET), EINVAL, "negative offset");
    check_err(lseek(fd, -8, SEEK_CUR), EINVAL, "negative result from SEEK_CUR");
    check_err(lseek(fd, -((1 << 20) + 2), SEEK_END), EINVAL, "negative result from SEEK_END");
    check(lseek(fd, 0, SEEK_CUR) == 7, "EINVAL keeps the offset");

    /* EBADF */
    check(close(fd) == 0, "close");
    check_err(lseek(fd, 0, SEEK_SET), EBADF, "closed descriptor");
    check_err(lseek(-1, 0, SEEK_SET), EBADF, "negative descriptor");

    /* A directory: negative results are EINVAL too */
    dfd = open(".", O_RDONLY);
    check(dfd >= 0, "open directory");
    check_err(lseek(dfd, -1, SEEK_SET), EINVAL, "negative offset on a directory");
    check(close(dfd) == 0, "close directory");

    /* ESPIPE: pipes, FIFOs and sockets */
    check(pipe(p) == 0, "pipe");
    check_err(lseek(p[0], 0, SEEK_CUR), ESPIPE, "pipe");
    check_err(lseek(p[1], 0, SEEK_SET), ESPIPE, "pipe write end");
    check(close(p[0]) == 0 && close(p[1]) == 0, "close pipe");
    check(mkfifo("fifo", 0600) == 0, "mkfifo");
    fd = open("fifo", O_RDONLY | O_NONBLOCK);
    check(fd >= 0, "open FIFO");
    check_err(lseek(fd, 0, SEEK_SET), ESPIPE, "FIFO");
    check(close(fd) == 0, "close FIFO");
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, s) == 0, "socketpair");
    check_err(lseek(s[0], 0, SEEK_SET), ESPIPE, "socket");
    check(close(s[0]) == 0 && close(s[1]) == 0, "close sockets");

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
