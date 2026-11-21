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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "../is-qemu.h"

/*
 * _exit and _Exit end the process at once: no atexit functions, no
 * flushing, the low eight bits of the status for wait, SIGCHLD for
 * the parent, and the children handed to a system process
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int fds[2];

static void
at_exit(void)
{
    (void)write(fds[1], "A", 1);
}

enum how { EXIT_POSIX, EXIT_C, EXIT_NORMAL };

/* Returns what the child wrote and its status */
static size_t
run(enum how how, int *status, char *buf, size_t size)
{
    pid_t   pid;
    size_t  got = 0;
    ssize_t n;

    check(pipe(fds) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        FILE *f;

        (void)close(fds[0]);
        f = fdopen(dup(fds[1]), "w");
        if (!f || setvbuf(f, NULL, _IOFBF, 256) != 0 || atexit(at_exit) != 0)
            _exit(2);
        fputs("buffered", f);
        switch (how) {
        case EXIT_POSIX:
            _exit(0x1234);
        case EXIT_C:
            _Exit(0x1234);
        default:
            exit(0x1234);
        }
    }
    (void)close(fds[1]);
    while ((n = read(fds[0], buf + got, size - got)) > 0)
        got += (size_t)n;
    (void)close(fds[0]);
    check(waitpid(pid, status, 0) == pid, "waitpid");
    return got;
}

int
main(void)
{
    char             buf[64];
    int              status, report[2];
    size_t           got;
    sigset_t         set, old;
    pid_t            pid, child;
    struct sigaction sa;
    siginfo_t        info;
    char             c;

    /* Nothing is flushed and no atexit function runs; the descriptors are closed */
    got = run(EXIT_POSIX, &status, buf, sizeof(buf));
    check(got == 0, "_exit wrote nothing");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0x34, "_exit status");
    got = run(EXIT_C, &status, buf, sizeof(buf));
    check(got == 0, "_Exit wrote nothing");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0x34, "_Exit status");
    /* Unlike exit */
    got = run(EXIT_NORMAL, &status, buf, sizeof(buf));
    /* atexit functions run first (whether exit flushes this stream depends on the configuration) */
    check(got >= 1 && buf[0] == 'A', "exit ran atexit");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0x34, "exit status");

    /* The parent gets SIGCHLD */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    check(sigprocmask(SIG_BLOCK, &set, &old) == 0, "block SIGCHLD");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0);
    /* Look before collecting it: a wait that collects the only child clears SIGCHLD */
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "child terminated");
    check(sigpending(&set) == 0 && sigismember(&set, SIGCHLD) == 1, "SIGCHLD");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "restore the mask");

    /* The children of a process that exits go to a system process */
    check(pipe(report) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pid_t parent = getpid();

        (void)close(report[0]);
        child = fork();
        if (child < 0)
            _exit(1);
        if (child == 0) {
            struct timespec nap = { 0, 1000000 };
            int             i;

            for (i = 0; i < 10000 && getppid() == parent; i++)
                nanosleep(&nap, NULL);
            c = getppid() != parent ? 'y' : 'n';
            (void)write(report[1], &c, 1);
            _exit(0);
        }
        _exit(0);
    }
    (void)close(report[1]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(read(report[0], &c, 1) == 1 && c == 'y', "the orphan has a new parent");
    (void)close(report[0]);

    /* With SIGCHLD ignored, the status is discarded */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    check(sigaction(SIGCHLD, &sa, NULL) == 0, "ignore SIGCHLD");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0);
    errno = 0;
    if (waitpid(pid, &status, 0) == pid && is_qemu())
        printf("qemu kept the status of a child with SIGCHLD ignored\n");
    else
        check(errno == ECHILD, "no status to wait for");

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
