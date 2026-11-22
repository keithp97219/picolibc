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
#if TEST_POSIX_ASYNCHRONOUS_IO && TEST_POSIX_THREADS && TEST_POSIX_DEVICE_IO      \
    && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_SIGNALS \
    && TEST_POSIX_PIPE && TEST_POSIX_MULTI_PROCESS

#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "../is-qemu.h"

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

static volatile sig_atomic_t interrupted;
static volatile int          stop_interrupting;
static pthread_t             main_thread;

static void
interrupt(int sig)
{
    (void)sig;
    interrupted = 1;
}

/* Signals the main thread until told to stop */
static void *
interrupter(void *arg)
{
    struct timespec pause = { 0, 5000000 };

    (void)arg;
    while (!__atomic_load_n(&stop_interrupting, __ATOMIC_ACQUIRE)) {
        pthread_kill(main_thread, SIGUSR2);
        nanosleep(&pause, NULL);
    }
    return NULL;
}

static int           notify_pipe[2];
static struct aiocb *notify_list[2];
static volatile int  notify_all_done;

/* The SIGEV_THREAD list notification */
static void
list_notified(union sigval value)
{
    struct aiocb **l = value.sival_ptr;

    notify_all_done = aio_error(l[0]) != EINPROGRESS && aio_error(l[1]) != EINPROGRESS;
    check(write(notify_pipe[1], "", 1) == 1, "notify");
}

/* Reads the process's address space size from /proc, without allocating */
static long
address_space(void)
{
    char    buf[64];
    ssize_t n;
    int     fd = open("/proc/self/statm", O_RDONLY);

    if (fd < 0)
        return -1;
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return -1;
    buf[n] = '\0';
    return atol(buf) * sysconf(_SC_PAGESIZE);
}

/* Without memory for a request's thread, lio_listio and aio_read fail with EAGAIN */
static void
no_resources(int fd)
{
    struct aiocb  rd, *list[1] = { &rd };
    char          buf[4];
    struct rlimit rl;
    pid_t         pid;
    int           status;
    long          size;

    if (is_qemu()) {
        printf("qemu places its own mappings; skipping the EAGAIN check\n");
        return;
    }
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        alarm(30);
        memset(&rd, 0, sizeof(rd));
        rd.aio_fildes = fd;
        rd.aio_buf = buf;
        rd.aio_nbytes = 4;
        rd.aio_lio_opcode = LIO_READ;
        rd.aio_sigevent.sigev_notify = SIGEV_NONE;
        /* Measuring the address space needs Linux's /proc */
        size = address_space();
        if (size <= 0) {
            printf("no address space size; skipping the EAGAIN check\n");
            exit(0);
        }
        /* No new mappings fit, so no thread stacks */
        rl.rlim_cur = rl.rlim_max = size;
        check(setrlimit(RLIMIT_AS, &rl) == 0, "setrlimit");
        check(aio_read(&rd) == -1 && errno == EAGAIN, "aio_read EAGAIN");
        check(lio_listio(LIO_WAIT, list, 1, NULL) == -1 && errno == EAGAIN, "LIO_WAIT EAGAIN");
        check(aio_error(&rd) == EAGAIN || aio_error(&rd) == EINPROGRESS, "request status");
        exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "no resources");
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

    /* A request that hangs ends the test */
    alarm(60);

    /*
     * Before any other request: a child of a process that has used
     * glibc's aio can wait for request threads it doesn't have
     */
    no_resources(fd);

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
    wr.aio_lio_opcode = LIO_WRITE;

    {
        struct aiocb   **big;
        long             max = sysconf(_SC_AIO_LISTIO_MAX);
        struct aiocb     prd;
        struct sigaction isa;
        pthread_t        thread;
        int              pipefd[2], ret;
        char             c;

        /* No more than AIO_LISTIO_MAX entries */
        check(max >= _POSIX_AIO_LISTIO_MAX || max == -1, "_SC_AIO_LISTIO_MAX");
        if (max > 0 && max < 100000) {
            big = calloc(max + 1, sizeof(*big));
            check(big != NULL, "calloc");
            check(lio_listio(LIO_WAIT, big, (int)max, NULL) == 0, "AIO_LISTIO_MAX entries");
            check(lio_listio(LIO_WAIT, big, (int)max + 1, NULL) == -1 && errno == EINVAL,
                  "too many entries");
            free(big);
        }

        /* LIO_WAIT ignores sig */
        sev.sigev_notify = 12345;
        check(lio_listio(LIO_WAIT, list, 4, &sev) == 0, "LIO_WAIT ignores sig");
        check(aio_return(&rd) == 4 && aio_return(&wr) == 4, "results");

#ifndef __GLIBC__
        /* An invalid sig fails with nothing started (glibc doesn't check it) */
        check(pwrite(fd, "wxyz", 4, 4) == 4, "pwrite");
        check(lio_listio(LIO_NOWAIT, list, 4, &sev) == -1 && errno == EINVAL, "bad sig");
        check(pread(fd, buf, 4, 4) == 4 && memcmp(buf, "wxyz", 4) == 0, "nothing written");
#endif

        /* SIGEV_THREAD notifies once every request is done */
        check(pipe(pipefd) == 0 && pipe(notify_pipe) == 0, "pipe");
        memset(&prd, 0, sizeof(prd));
        prd.aio_fildes = pipefd[0];
        prd.aio_buf = buf;
        prd.aio_nbytes = 1;
        prd.aio_lio_opcode = LIO_READ;
        prd.aio_sigevent.sigev_notify = SIGEV_NONE;
        notify_list[0] = &prd;
        notify_list[1] = &wr;
        memset(&sev, 0, sizeof(sev));
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = list_notified;
        sev.sigev_value.sival_ptr = notify_list;
        check(lio_listio(LIO_NOWAIT, notify_list, 2, &sev) == 0, "LIO_NOWAIT, SIGEV_THREAD");
        wait_for(&wr);
        /* The pipe read can't be done, so neither is the list */
        check(aio_error(&prd) == EINPROGRESS, "pipe read waiting");
        check(write(pipefd[1], "p", 1) == 1, "write to the pipe");
        check(read(notify_pipe[0], &c, 1) == 1, "list notification");
        check(notify_all_done, "notified after every request");
        check(aio_return(&prd) == 1 && aio_return(&wr) == 4, "results");

#ifndef __GLIBC__
        /*
         * A signal handler interrupts LIO_WAIT, leaving the requests
         * running (glibc returns EINTR, then its request thread crashes)
         */
        main_thread = pthread_self();
        memset(&isa, 0, sizeof(isa));
        isa.sa_handler = interrupt;
        sigemptyset(&isa.sa_mask);
        check(sigaction(SIGUSR2, &isa, NULL) == 0, "sigaction");
        check(pthread_create(&thread, NULL, interrupter, NULL) == 0, "pthread_create");
        ret = lio_listio(LIO_WAIT, notify_list, 1, NULL);
        check(ret == -1 && errno == EINTR && interrupted, "EINTR");
        __atomic_store_n(&stop_interrupting, 1, __ATOMIC_RELEASE);
        check(pthread_join(thread, NULL) == 0, "pthread_join");
        check(aio_error(&prd) == EINPROGRESS, "still waiting");
        check(write(pipefd[1], "q", 1) == 1, "write to the pipe");
        wait_for(&prd);
        check(aio_return(&prd) == 1 && buf[0] == 'q', "interrupted list's read");
#else
        (void)thread;
        (void)ret;
        (void)isa;
        (void)interrupter;
        (void)interrupt;
#endif
        check(close(pipefd[0]) == 0 && close(pipefd[1]) == 0 && close(notify_pipe[0]) == 0
                  && close(notify_pipe[1]) == 0,
              "close the pipes");
    }

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
