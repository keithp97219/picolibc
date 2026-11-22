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

/* aio_read reads asynchronously at the requested offset */

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

static volatile sig_atomic_t got_signal, got_code;
static void * volatile got_value;

static void
handler(int sig, siginfo_t *info, void *context)
{
    (void)context;
    got_signal = sig == SIGUSR1 && info->si_signo == SIGUSR1;
    got_code = info->si_code;
    got_value = info->si_value.sival_ptr;
}

int
main(void)
{
    struct aiocb        cb;
    const struct aiocb *list[1] = { &cb };
    struct timespec     brief = { 0, 50000000 };
    struct sigaction    sa;
    sigset_t            block, old, pending;
    char                buf[8];
    int                 fd = scratch("0123456789"), pipefd[2];

    /* A request that hangs ends the test */
    alarm(60);

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_offset = 3;
    cb.aio_buf = buf;
    cb.aio_nbytes = 4;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_read(&cb) == 0, "aio_read");
    wait_for(&cb);
    check(aio_error(&cb) == 0, "no error");
    check(aio_return(&cb) == 4 && memcmp(buf, "3456", 4) == 0, "contents");

    /* Reading past the end returns 0 */
    cb.aio_offset = 100;
    check(aio_read(&cb) == 0, "aio_read past end");
    wait_for(&cb);
    check(aio_return(&cb) == 0, "end of file");

    /* An invalid offset fails, now or as the request's status */
    cb.aio_offset = -1;
    if (aio_read(&cb) == 0) {
        wait_for(&cb);
        check(aio_error(&cb) == EINVAL && aio_return(&cb) == -1, "EINVAL status");
    } else {
        check(errno == EINVAL, "EINVAL");
    }
    cb.aio_offset = 0;

    /* An invalid priority fails at once */
    cb.aio_reqprio = -1;
    check(aio_read(&cb) == -1 && errno == EINVAL, "bad aio_reqprio");
    cb.aio_reqprio = 0;

    /* aio_lio_opcode is ignored */
    cb.aio_lio_opcode = LIO_WRITE;
    check(aio_read(&cb) == 0, "aio_read with LIO_WRITE");
    wait_for(&cb);
    check(aio_return(&cb) == 4 && memcmp(buf, "0123", 4) == 0, "read, not written");
    cb.aio_lio_opcode = 0;

    /*
     * A pipe can't seek, so the offset doesn't apply; the request stays
     * in progress until there is something to read
     */
    check(pipe(pipefd) == 0, "pipe");
    cb.aio_fildes = pipefd[0];
    cb.aio_offset = 1000;
    check(aio_read(&cb) == 0, "aio_read from a pipe");
    check(aio_error(&cb) == EINPROGRESS, "waiting for data");
    check(aio_suspend(list, 1, &brief) == -1 && errno == EAGAIN, "still waiting");
    check(write(pipefd[1], "xyz", 3) == 3, "write to the pipe");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 3 && memcmp(buf, "xyz", 3) == 0, "pipe read");

    /* SIGEV_SIGNAL sends the signal, with SI_ASYNCIO and the value */
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    /* Blocked until sigsuspend, so the signal can't arrive before the wait */
    check(sigprocmask(SIG_BLOCK, &block, &old) == 0, "sigprocmask");
    cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb.aio_sigevent.sigev_signo = SIGUSR1;
    cb.aio_sigevent.sigev_value.sival_ptr = &cb;
    check(aio_read(&cb) == 0, "aio_read with SIGEV_SIGNAL");
    check(write(pipefd[1], "abc", 3) == 3, "write to the pipe");
    while (!got_signal)
        sigsuspend(&old);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 3, "signaled read");
    check(got_value == &cb, "si_value");
    check(got_code == SI_ASYNCIO, "si_code");

    /* SIGEV_NONE sends nothing, whatever sigev_signo says */
    got_signal = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_read(&cb) == 0, "aio_read with SIGEV_NONE");
    check(write(pipefd[1], "d", 1) == 1, "write to the pipe");
    wait_for(&cb);
    check(aio_return(&cb) == 1, "quiet read");
    check(sigpending(&pending) == 0 && !sigismember(&pending, SIGUSR1) && !got_signal, "no signal");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "sigprocmask");
    check(close(pipefd[0]) == 0 && close(pipefd[1]) == 0, "close the pipe");

    /* A bad descriptor fails, now or as the request's status */
    cb.aio_fildes = -1;
    cb.aio_offset = 0;
    if (aio_read(&cb) == 0) {
        wait_for(&cb);
        check(aio_error(&cb) == EBADF && aio_return(&cb) == -1, "EBADF status");
    } else {
        check(errno == EBADF, "EBADF");
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
