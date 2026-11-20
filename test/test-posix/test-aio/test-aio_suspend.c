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
#if !defined(__PICOLIBC__)                                                                      \
    || ((defined(_POSIX_ASYNCHRONOUS_IO) && _POSIX_ASYNCHRONOUS_IO > 0)                         \
        && (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)                                      \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)          \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0))

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* aio_suspend waits for a request, or times out */

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
    struct aiocb        cb, *none = NULL;
    const struct aiocb *list[2];
    struct timespec     timeout = { 0, 50000000 };
    char                buf[4];
    int                 fd = scratch("data");

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = 1;
    cb.aio_offset = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;

#ifdef __PICOLIBC__
    /* Entries may be NULL; with nothing to wait for, it times out (glibc returns at once) */
    list[0] = none;
    check(aio_suspend(list, 1, &timeout) == -1 && errno == EAGAIN, "timeout");
#endif
    list[0] = none;

    /* A finished request returns at once */
    check(aio_read(&cb) == 0, "aio_read");
    list[1] = &cb;
    check(aio_suspend(list, 2, NULL) == 0, "aio_suspend");
    while (aio_error(&cb) == EINPROGRESS)
        check(aio_suspend(list, 2, NULL) == 0, "aio_suspend again");
    check(aio_suspend(list, 2, &timeout) == 0, "already done");
    (void)aio_return(&cb);

    cleanup(fd);
    (void)wait_for;
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
