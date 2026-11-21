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
#if _POSIX_ASYNCHRONOUS_IO > 0 && _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_DEVICE_IO) \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)   \
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

/* lio_listio runs a list of requests, waiting or notifying */

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

static volatile sig_atomic_t list_done;
static volatile sig_atomic_t value_ok;

static void
handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    value_ok = info->si_value.sival_int == 42;
    list_done = 1;
}

int
main(void)
{
    struct aiocb     rd, wr, nop, *list[4];
    struct sigaction sa;
    struct sigevent  sev;
    sigset_t         block, old;
    char             buf[4];
    int              fd = scratch("abcdefgh");

    memset(&rd, 0, sizeof(rd));
    rd.aio_fildes = fd;
    rd.aio_buf = buf;
    rd.aio_nbytes = 4;
    rd.aio_lio_opcode = LIO_READ;
    rd.aio_sigevent.sigev_notify = SIGEV_NONE;
    wr = rd;
    wr.aio_offset = 4;
    wr.aio_buf = "WXYZ";
    wr.aio_lio_opcode = LIO_WRITE;
    nop = rd;
    nop.aio_lio_opcode = LIO_NOP;
    list[0] = &rd;
    list[1] = NULL;
    list[2] = &wr;
    list[3] = &nop;

    check(lio_listio(LIO_WAIT, list, 4, NULL) == 0, "LIO_WAIT");
    check(aio_return(&rd) == 4 && memcmp(buf, "abcd", 4) == 0, "read");
    check(aio_return(&wr) == 4, "write");
    check(pread(fd, buf, 4, 4) == 4 && memcmp(buf, "WXYZ", 4) == 0, "written");

    /* LIO_NOWAIT signals when the whole list is done */
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_int = 42;
    /* Blocked until sigsuspend, so the signal can't arrive before the wait */
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &block, &old) == 0, "sigprocmask");
    check(lio_listio(LIO_NOWAIT, list, 4, &sev) == 0, "LIO_NOWAIT");
    while (!list_done)
        sigsuspend(&old);
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "sigprocmask");
    check(value_ok, "signal value");
    check(aio_return(&rd) == 4 && aio_return(&wr) == 4, "results");

    check(lio_listio(42, list, 4, NULL) == -1 && errno == EINVAL, "bad mode");

    /* Invalid requests fail the list, each with its own status */
#ifndef __GLIBC__
    /* glibc doesn't check the priority */
    rd.aio_reqprio = -1;
#endif
    wr.aio_lio_opcode = 42;
    check(lio_listio(LIO_WAIT, list, 4, NULL) == -1 && errno == EIO, "invalid requests");
#ifndef __GLIBC__
    check(aio_error(&rd) == EINVAL && aio_return(&rd) == -1 && errno == EINVAL, "bad priority");
#endif
    check(aio_error(&wr) == EINVAL, "bad opcode");
#ifndef __GLIBC__
    /* glibc queues the others and reports success */
    check(lio_listio(LIO_NOWAIT, list, 4, NULL) == -1 && errno == EIO, "invalid LIO_NOWAIT");
#endif
    rd.aio_reqprio = 0;
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
