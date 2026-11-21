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
#if _POSIX_THREADS > 0

#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>

/*
 * thrd_exit: called from the initial thread, the other threads keep
 * running and the process exits with status 0 once the last one
 * ends. Thread exit runs destructors, keeps mutexes locked and runs
 * no atexit handlers.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int   wfd;
static tss_t key;
static mtx_t mtx;
static sem_t dtor_ran;
static int   value;

static void
mark(char c)
{
    if (write(wfd, &c, 1) != 1)
        _exit(13);
}

static void
at_exit(void)
{
    mark('A');
}

static void
dtor(void *p)
{
    mark(p == &value && tss_get(key) == NULL ? 'D' : 'd');
    sem_post(&dtor_ran);
}

static int
locker(void *arg)
{
    (void)arg;
    mtx_lock(&mtx);
    thrd_exit(3);
}

/* Runs after the initial thread has called thrd_exit */
static int
last(void *arg)
{
    (void)arg;
    while (sem_wait(&dtor_ran) != 0)
        ;
    mark('T');
    return 5;
}

static void
child(void)
{
    thrd_t thr;
    int    res = 0;

    if (atexit(at_exit) != 0 || tss_create(&key, dtor) != thrd_success
        || mtx_init(&mtx, mtx_plain) != thrd_success || sem_init(&dtor_ran, 0, 0) != 0)
        _exit(10);

    /* Thread exit keeps mutexes locked and runs no atexit handlers */
    if (thrd_create(&thr, locker, NULL) != thrd_success || thrd_join(thr, &res) != thrd_success
        || res != 3)
        _exit(11);
    mark(mtx_trylock(&mtx) == thrd_busy ? 'M' : 'm');

    if (tss_set(key, &value) != thrd_success || thrd_create(&thr, last, NULL) != thrd_success)
        _exit(12);
    thrd_exit(7);
}

int
main(void)
{
    char  buf[16];
    int   fds[2], status;
    pid_t pid;
    long  n, len = 0;

    alarm(60);
    check(pipe(fds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(fds[0]);
        wfd = fds[1];
        child();
    }
    close(fds[1]);
    while ((n = read(fds[0], buf + len, sizeof(buf) - 1 - len)) > 0)
        len += n;
    buf[len] = '\0';
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    printf("events %s, status %#x\n", buf, status);
    check(WIFEXITED(status), "process exited");
    check(WEXITSTATUS(status) != 10 && WEXITSTATUS(status) != 11 && WEXITSTATUS(status) != 12,
          "setup");
    check(WEXITSTATUS(status) == 0, "exit status 0 after the last thread ends");
    check(strcmp(buf, "MDTA") == 0, "order of events");
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
