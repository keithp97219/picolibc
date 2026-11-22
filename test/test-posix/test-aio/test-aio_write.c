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
/* SIGXFSZ and setrlimit are XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_ASYNCHRONOUS_IO && TEST_POSIX_THREADS && TEST_POSIX_DEVICE_IO     \
    && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_PIPE   \
    && TEST_POSIX_SIGNALS && TEST_POSIX_MULTI_PROCESS && TEST_XSI_SINGLE_PROCESS

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * aio_write writes asynchronously at the requested offset, or in the
 * order of the calls when the file appends
 */

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

/* Reads 'skip' bytes, then up to 'len' into 'out'; returns the count */
static size_t
drain(int fd, size_t skip, char *out, size_t len)
{
    static char buf[4096];
    size_t      got = 0;
    ssize_t     n;

    while (skip > 0) {
        n = read(fd, buf, skip < sizeof(buf) ? skip : sizeof(buf));
        check(n > 0, "read the filler");
        skip -= n;
    }
    while (got < len && (n = read(fd, out + got, len - got)) > 0)
        got += n;
    return got;
}

static int notify_pipe[2];
static void * volatile notify_value;
static volatile int notify_status;
static volatile int notify_other_thread;
static pthread_t    main_thread;

/* A SIGEV_THREAD notification, in a thread of its own */
static void
notified(union sigval value)
{
    notify_value = value.sival_ptr;
    notify_status = aio_error(value.sival_ptr);
    notify_other_thread = !pthread_equal(pthread_self(), main_thread);
    check(write(notify_pipe[1], "", 1) == 1, "notify");
}

static volatile sig_atomic_t got_xfsz;

static void
xfsz(int sig)
{
    (void)sig;
    got_xfsz = 1;
}

/* Past the file size limit, a write fails with EFBIG and raises SIGXFSZ */
static void
file_size_limit(int fd)
{
    struct aiocb  cb;
    struct rlimit rl = { 10, 10 };
    sigset_t      block, pending;
    pid_t         pid;
    int           status;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        alarm(30);
        memset(&cb, 0, sizeof(cb));
        cb.aio_fildes = fd;
        cb.aio_offset = 10;
        cb.aio_buf = "abc";
        cb.aio_nbytes = 3;
        cb.aio_sigevent.sigev_notify = SIGEV_NONE;
        check(setrlimit(RLIMIT_FSIZE, &rl) == 0, "setrlimit");

        /* Ignored, the signal does nothing */
        signal(SIGXFSZ, SIG_IGN);
        check(aio_write(&cb) == 0, "aio_write past the limit");
        wait_for(&cb);
        check(aio_error(&cb) == EFBIG && aio_return(&cb) == -1, "EFBIG");

#ifndef __GLIBC__
        /* glibc's request threads block SIGXFSZ, which then never arrives */
        signal(SIGXFSZ, xfsz);
        sigemptyset(&block);
        sigaddset(&block, SIGXFSZ);
        check(sigprocmask(SIG_BLOCK, &block, NULL) == 0, "sigprocmask");
        check(aio_write(&cb) == 0, "aio_write past the limit");
        wait_for(&cb);
        check(aio_error(&cb) == EFBIG && aio_return(&cb) == -1, "EFBIG");
        check(sigpending(&pending) == 0 && sigismember(&pending, SIGXFSZ), "SIGXFSZ");
        check(sigprocmask(SIG_UNBLOCK, &block, NULL) == 0 && got_xfsz, "SIGXFSZ handled");
#else
        (void)block;
        (void)pending;
        (void)xfsz;
#endif
        exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "file size limit");
}

int
main(void)
{
    struct aiocb        cb, cb2, appends[32];
    const struct aiocb *list[1];
    struct timespec     brief = { 0, 100000000 };
    pthread_attr_t      attr;
    char                buf[64], c;
    int                 fd = scratch("0123456789"), afd, pipefd[2], i;
    size_t              filled;

    /* A request that hangs ends the test */
    alarm(60);
    main_thread = pthread_self();

    /*
     * Before any other request: a child of a process that has used
     * glibc's aio can wait for request threads it doesn't have
     */
    file_size_limit(fd);

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_offset = 2;
    cb.aio_buf = "abc";
    cb.aio_nbytes = 3;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;
    check(aio_write(&cb) == 0, "aio_write");
    wait_for(&cb);
    check(aio_error(&cb) == 0 && aio_return(&cb) == 3, "status");
    check(pread(fd, buf, 10, 0) == 10 && memcmp(buf, "01abc56789", 10) == 0, "contents");

    cb.aio_reqprio = -1;
    check(aio_write(&cb) == -1 && errno == EINVAL, "bad aio_reqprio");
    cb.aio_reqprio = 0;

    /* SIGEV_THREAD calls the function in a new thread, after completion */
    check(pipe(notify_pipe) == 0, "pipe");
    cb.aio_offset = 0;
    cb.aio_buf = "AB";
    cb.aio_nbytes = 2;
    cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
    cb.aio_sigevent.sigev_notify_function = notified;
    cb.aio_sigevent.sigev_value.sival_ptr = &cb;
    cb.aio_sigevent.sigev_notify_attributes = NULL;
    check(aio_write(&cb) == 0, "aio_write with SIGEV_THREAD");
    check(read(notify_pipe[0], &c, 1) == 1, "notified");
    check(notify_value == &cb && notify_status == 0 && notify_other_thread, "notification");
    check(aio_return(&cb) == 2, "notified write");

    /* With attributes too */
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0, "detached");
    check(pthread_attr_setstacksize(&attr, 256 * 1024) == 0, "stack size");
    cb.aio_sigevent.sigev_notify_attributes = &attr;
    notify_value = NULL;
    check(aio_write(&cb) == 0, "aio_write with attributes");
    check(read(notify_pipe[0], &c, 1) == 1, "notified");
    check(notify_value == &cb && notify_status == 0 && notify_other_thread, "notification");
    check(aio_return(&cb) == 2, "notified write");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;

    /* A close after queuing doesn't stop the request (glibc's may fail with EBADF) */
    cb.aio_fildes = dup(fd);
    cb.aio_offset = 8;
    cb.aio_buf = "YZ";
    check(cb.aio_fildes >= 0, "dup");
    check(aio_write(&cb) == 0, "aio_write before close");
    check(close(cb.aio_fildes) == 0, "close");
    wait_for(&cb);
#ifndef __GLIBC__
    check(aio_error(&cb) == 0 && aio_return(&cb) == 2, "written after close");
#else
    if (aio_return(&cb) != 2)
        check(pwrite(fd, "YZ", 2, 8) == 2, "write instead");
#endif
    check(pread(fd, buf, 10, 0) == 10 && memcmp(buf, "ABabc567YZ", 10) == 0, "contents");

    /* With O_APPEND, the offset doesn't apply, and writes go in order */
    afd = open("file", O_WRONLY | O_APPEND);
    check(afd >= 0, "open with O_APPEND");
    for (i = 0; i < 32; i++) {
        memset(&appends[i], 0, sizeof(appends[i]));
        appends[i].aio_fildes = afd;
        appends[i].aio_buf = &"abcdefghijklmnopqrstuvwxyzABCDEF"[i];
        appends[i].aio_nbytes = 1;
        appends[i].aio_sigevent.sigev_notify = SIGEV_NONE;
        check(aio_write(&appends[i]) == 0, "aio_write with O_APPEND");
    }
    for (i = 0; i < 32; i++) {
        wait_for(&appends[i]);
        check(aio_return(&appends[i]) == 1, "appended");
    }
    check(pread(fd, buf, 42, 0) == 42 && memcmp(buf, "ABabc567YZ", 10) == 0
              && memcmp(buf + 10, "abcdefghijklmnopqrstuvwxyzABCDEF", 32) == 0,
          "appended in order");
    check(close(afd) == 0, "close");

    /*
     * A pipe can't seek, so writes go in order too: one that would
     * finish at once still waits for the earlier one, which blocks
     */
    check(pipe(pipefd) == 0, "pipe");
    filled = fill(pipefd[1]);
    memset(&cb2, 0, sizeof(cb2));
    cb.aio_fildes = pipefd[1];
    cb.aio_offset = 0;
    cb.aio_buf = "first";
    cb.aio_nbytes = 5;
    cb2 = cb;
    cb2.aio_offset = 1000;
    cb2.aio_buf = "";
    cb2.aio_nbytes = 0;
    check(aio_write(&cb) == 0 && aio_write(&cb2) == 0, "aio_write to a full pipe");
    list[0] = &cb2;
    check(aio_suspend(list, 1, &brief) == -1 && errno == EAGAIN, "empty write waits its turn");
    check(aio_error(&cb) == EINPROGRESS && aio_error(&cb2) == EINPROGRESS, "both waiting");
    check(drain(pipefd[0], filled, buf, 5) == 5 && memcmp(buf, "first", 5) == 0, "pipe contents");
    wait_for(&cb);
    wait_for(&cb2);
    check(aio_return(&cb) == 5 && aio_return(&cb2) == 0, "pipe writes");

#ifndef __GLIBC__
    /*
     * A request still queued when the descriptor is closed runs as if
     * the close came after it (glibc's fails with EBADF)
     */
    filled = fill(pipefd[1]);
    cb2.aio_buf = "second";
    cb2.aio_nbytes = 6;
    check(aio_write(&cb) == 0 && aio_write(&cb2) == 0, "aio_write to a full pipe");
    check(close(pipefd[1]) == 0, "close the pipe");
    check(drain(pipefd[0], filled, buf, sizeof(buf)) == 11, "read to the end");
    check(memcmp(buf, "firstsecond", 11) == 0, "pipe contents");
    wait_for(&cb);
    wait_for(&cb2);
    check(aio_return(&cb) == 5 && aio_return(&cb2) == 6, "pipe writes");
#else
    check(close(pipefd[1]) == 0, "close the pipe");
#endif
    check(close(pipefd[0]) == 0 && close(notify_pipe[0]) == 0 && close(notify_pipe[1]) == 0,
          "close the pipes");

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
