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
#include <pthread.h>
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

static volatile sig_atomic_t interrupted;
static volatile int          stop_interrupting;
static pthread_t             main_thread;

static void
handler(int sig)
{
    (void)sig;
    interrupted = 1;
}

/*
 * Signals the main thread until told to stop; one of them arrives while
 * it waits, whenever it starts to
 */
static void *
interrupter(void *arg)
{
    struct timespec pause = { 0, 5000000 };

    (void)arg;
    while (!__atomic_load_n(&stop_interrupting, __ATOMIC_ACQUIRE)) {
        pthread_kill(main_thread, SIGUSR1);
        nanosleep(&pause, NULL);
    }
    return NULL;
}

/* Calls aio_suspend with a cancellation pending */
static void *
cancel_pending(void *arg)
{
    const struct aiocb * const *list = arg;

    pthread_cancel(pthread_self());
    aio_suspend(list, 1, NULL);
    return NULL;
}

int
main(void)
{
    struct aiocb        cb, *none = NULL;
    const struct aiocb *list[2];
    struct timespec     timeout = { 0, 50000000 };
    struct sigaction    sa;
    pthread_t           thread;
    void               *result;
    char                buf[4];
    int                 fd = scratch("data"), pipefd[2], ret;

    /* A request that hangs ends the test */
    alarm(60);
    main_thread = pthread_self();

    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = 1;
    cb.aio_offset = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_NONE;

#ifndef __GLIBC__
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

#ifndef __GLIBC__
    /* Even so, a pending cancellation is acted on (glibc returns 0) */
    check(pthread_create(&thread, NULL, cancel_pending, &list[1]) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED, "cancelled");
#else
    (void)cancel_pending;
    (void)result;
#endif
    (void)aio_return(&cb);

    /* A request that can't finish times out */
    check(pipe(pipefd) == 0, "pipe");
    cb.aio_fildes = pipefd[0];
    check(aio_read(&cb) == 0, "aio_read from an empty pipe");
    check(aio_suspend(list, 2, &timeout) == -1 && errno == EAGAIN, "timed out");

    /* A signal handler interrupts the wait */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    check(pthread_create(&thread, NULL, interrupter, NULL) == 0, "pthread_create");
    ret = aio_suspend(list, 2, NULL);
    check(ret == -1 && errno == EINTR && interrupted, "EINTR");
    __atomic_store_n(&stop_interrupting, 1, __ATOMIC_RELEASE);
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(aio_error(&cb) == EINPROGRESS, "still waiting");

    /* Then the request completes */
    check(write(pipefd[1], "x", 1) == 1, "write to the pipe");
    check(aio_suspend(list, 2, NULL) == 0 || errno == EINTR, "aio_suspend");
    wait_for(&cb);
    check(aio_return(&cb) == 1, "pipe read");
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
