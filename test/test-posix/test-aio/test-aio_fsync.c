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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_ASYNCHRONOUS_IO > 0 && _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_FILE_SYSTEM) \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && defined(__PICOLIBC_POSIX_PIPE)            \
    && defined(__PICOLIBC_POSIX_SIGNALS)

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* aio_fsync synchronizes a file asynchronously */

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

/* Fills a pipe, so that the next write to it blocks; returns the count */
static size_t
fill(int fd)
{
    static char buf[4096];
    size_t      total = 0;
    ssize_t     n;
    int         flags = fcntl(fd, F_GETFL);

    check(flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0, "set O_NONBLOCK");
    while ((n = write(fd, buf, sizeof(buf))) > 0)
        total += n;
    while ((n = write(fd, buf, 1)) > 0)
        total += n;
    check(errno == EAGAIN && fcntl(fd, F_SETFL, flags) == 0, "pipe full");
    return total;
}

/* Reads 'len' bytes, or to the end of the file */
static void
drain(int fd, size_t len)
{
    static char buf[4096];
    ssize_t     n;

    while (len > 0 && (n = read(fd, buf, len < sizeof(buf) ? len : sizeof(buf))) > 0)
        len -= n;
}

int
main(void)
{
    struct aiocb        cb, wr;
    const struct aiocb *list[1] = { &cb };
    struct timespec     brief = { 0, 100000000 };
    int                 fd = scratch("data"), pipefd[2];
    size_t              filled;

    /* A request that hangs ends the test */
    alarm(60);

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_fsync(O_SYNC, &cb) == 0, "O_SYNC");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 0, "status");
    check(aio_fsync(O_DSYNC, &cb) == 0, "O_DSYNC");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 0, "status");
    check(aio_fsync(O_RDWR, &cb) == -1 && errno == EINVAL, "bad op");

#ifndef __GLIBC__
    /* Only aio_fildes and aio_sigevent matter (glibc checks aio_reqprio) */
    cb.aio_reqprio = -1;
    cb.aio_lio_opcode = 42;
    check(aio_fsync(O_SYNC, &cb) == 0, "aio_reqprio ignored");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 0, "status");
    cb.aio_reqprio = 0;
#endif

    /* A bad descriptor fails at once */
    cb.aio_fildes = -1;
    check(aio_fsync(O_SYNC, &cb) == -1 && errno == EBADF, "EBADF");
    check(pipe(pipefd) == 0, "pipe");
    check(close(pipefd[1]) == 0, "close");
    cb.aio_fildes = pipefd[1];
    check(aio_fsync(O_DSYNC, &cb) == -1 && errno == EBADF, "EBADF after close");

    /*
     * aio_fsync completes the requests queued before it, so it can't
     * finish while a write to a full pipe blocks. A pipe can't be
     * synchronized, so then it reports EINVAL.
     */
    check(pipe(pipefd) == 0, "pipe");
    filled = fill(pipefd[1]);
    memset(&wr, 0, sizeof(wr));
    wr.aio_fildes = pipefd[1];
    wr.aio_buf = "data";
    wr.aio_nbytes = 4;
    wr.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_write(&wr) == 0, "aio_write to a full pipe");
    cb.aio_fildes = pipefd[1];
    check(aio_fsync(O_SYNC, &cb) == 0, "aio_fsync after it");
    check(aio_suspend(list, 1, &brief) == -1 && errno == EAGAIN, "waits for the write");
    check(aio_error(&wr) == EINPROGRESS && aio_error(&cb) == EINPROGRESS, "both waiting");
    drain(pipefd[0], filled + 4);
    wait_for(&wr);
    wait_for(&cb);
    check(aio_return(&wr) == 4, "write");
    check(aio_error(&cb) == EINVAL && aio_return(&cb) == -1, "fsync on a pipe");
    check(close(pipefd[0]) == 0 && close(pipefd[1]) == 0, "close the pipe");
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
