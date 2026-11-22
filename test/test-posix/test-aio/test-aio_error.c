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
#if TEST_POSIX_ASYNCHRONOUS_IO && TEST_POSIX_THREADS && TEST_POSIX_FILE_SYSTEM \
    && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_PIPE

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* aio_error reports EINPROGRESS until the request finishes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

/* Makes a scratch file in a new directory, holding 'contents' */
static int
scratch(const char *contents)
{
    int fd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(chdir(dir) == 0, "chdir");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0, "create file");
    check(write(fd, contents, strlen(contents)) == (ssize_t)strlen(contents), "write");
    return fd;
}

static void
cleanup(int fd)
{
    check(close(fd) == 0 && unlink("file") == 0, "remove file");
    check(chdir("..") == 0 && rmdir(dir) == 0, "remove directory");
}

/* Waits for one request */
static void
wait_for(struct aiocb *cb)
{
    const struct aiocb *list[1] = { cb };

    while (aio_error(cb) == EINPROGRESS)
        check(aio_suspend(list, 1, NULL) == 0 || errno == EINTR, "aio_suspend");
}

int
main(void)
{
    struct aiocb cb;
    char         buf[4];
    int          fd = scratch("data"), wfd, error;

    /* A descriptor open only for writing can't be read */
    wfd = open("file", O_WRONLY);
    check(wfd >= 0, "open write-only");
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = wfd;
    cb.aio_buf = buf;
    cb.aio_nbytes = 1;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    if (aio_read(&cb) == 0) {
        wait_for(&cb);
        error = aio_error(&cb);
        check(error == EBADF, "EBADF status");
        check(aio_return(&cb) == -1, "aio_return");
    } else {
        check(errno == EBADF, "EBADF");
    }

    cb.aio_fildes = fd;
    check(aio_read(&cb) == 0, "aio_read");
    error = aio_error(&cb);
    check(error == EINPROGRESS || error == 0, "in progress or done");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 1, "done");
    check(close(wfd) == 0, "close");

    /* A read from an empty pipe stays in progress until there is data */
    {
        int pipefd[2];

        check(pipe(pipefd) == 0, "pipe");
        cb.aio_fildes = pipefd[0];
        check(aio_read(&cb) == 0, "aio_read from a pipe");
        check(aio_error(&cb) == EINPROGRESS, "EINPROGRESS");
        check(write(pipefd[1], "x", 1) == 1, "write to the pipe");
        wait_for(&cb);
        check(aio_error(&cb) == 0 && aio_return(&cb) == 1 && buf[0] == 'x', "done");
        check(close(pipefd[0]) == 0 && close(pipefd[1]) == 0, "close the pipe");
    }
    cleanup(fd);
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
