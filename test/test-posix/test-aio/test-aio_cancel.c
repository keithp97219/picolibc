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
    && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_PIPE && TEST_POSIX_SIGNALS

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* aio_cancel reports requests that are done or can't be canceled */

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

/* Reads 'len' bytes */
static void
drain(int fd, size_t len)
{
    static char buf[4096];
    ssize_t     n;

    while (len > 0 && (n = read(fd, buf, len < sizeof(buf) ? len : sizeof(buf))) > 0)
        len -= n;
}

static volatile sig_atomic_t notified;
static void * volatile notified_value;

static void
handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    notified_value = info->si_value.sival_ptr;
    notified = 1;
}

int
main(void)
{
    struct aiocb     cb, first, second;
    struct sigaction sa;
    sigset_t         block, old;
    char             buf[4];
    int              fd = scratch("data"), ret, pipefd[2];
    size_t           filled;

    /* A request that hangs ends the test */
    alarm(60);

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = 4;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_read(&cb) == 0, "aio_read");
    ret = aio_cancel(fd, &cb);
    check(ret == AIO_CANCELED || ret == AIO_NOTCANCELED || ret == AIO_ALLDONE, "result");
    wait_for(&cb);
    if (ret == AIO_CANCELED)
        check(aio_error(&cb) == ECANCELED && aio_return(&cb) == -1, "ECANCELED");
    else
        check(aio_return(&cb) == 4, "finished");
    check(aio_cancel(fd, NULL) == AIO_ALLDONE, "all done");
    check(aio_cancel(-1, NULL) == -1 && errno == EBADF, "EBADF");

    /*
     * A write blocked on a full pipe is in progress. The one queued
     * behind it, which must wait its turn, can be canceled; then it
     * gets ECANCELED and its notification.
     */
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &block, &old) == 0, "sigprocmask");

    check(pipe(pipefd) == 0, "pipe");
    filled = fill(pipefd[1]);
    memset(&first, 0, sizeof(first));
    first.aio_fildes = pipefd[1];
    first.aio_buf = "first";
    first.aio_nbytes = 5;
    first.aio_sigevent.sigev_notify = SIGEV_NONE;
    second = first;
    second.aio_buf = "second";
    second.aio_nbytes = 6;
    second.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    second.aio_sigevent.sigev_signo = SIGUSR1;
    second.aio_sigevent.sigev_value.sival_ptr = &second;
    check(aio_write(&first) == 0 && aio_write(&second) == 0, "aio_write to a full pipe");
    ret = aio_cancel(pipefd[1], &second);
#ifdef __PICOLIBC__
    /* picolibc cancels requests that haven't started */
    check(ret == AIO_CANCELED, "queued request canceled");
#endif
    check(ret == AIO_CANCELED || ret == AIO_NOTCANCELED, "result");
    if (ret == AIO_CANCELED) {
        check(aio_error(&second) == ECANCELED && aio_return(&second) == -1, "ECANCELED");
        while (!notified)
            sigsuspend(&old);
        check(notified_value == &second, "canceled request's notification");
    }

    /*
     * The first is blocked, or its thread hasn't started yet; only then
     * can it be canceled
     */
    check(aio_error(&first) == EINPROGRESS, "first in progress");
    ret = aio_cancel(pipefd[1], NULL);
    check(ret == AIO_NOTCANCELED || ret == AIO_CANCELED, "result");
    if (ret == AIO_NOTCANCELED)
        check(aio_error(&first) == EINPROGRESS, "aiocb untouched");
    else
        check(aio_error(&first) == ECANCELED, "first canceled");
    drain(pipefd[0], filled);
    wait_for(&first);
    wait_for(&second);
    if (ret == AIO_NOTCANCELED)
        check(aio_return(&first) == 5, "first written");
    (void)aio_return(&second);
    check(aio_cancel(pipefd[1], NULL) == AIO_ALLDONE, "all done");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "sigprocmask");
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
