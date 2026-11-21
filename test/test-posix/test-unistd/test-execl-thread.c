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
/* glibc declares _Fork only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_MULTI_PROCESS)                 \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * execl and execle are async-signal-safe, so the child of _Fork in a
 * multi-threaded process can use them whatever the other threads were
 * doing. An exec, or _exit, ends every thread without running cleanup
 * handlers or thread-specific data destructors.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define ARGV0 "helper-argv0"

static char          exe[PATH_MAX];
static volatile int  stop;
static int           report[2], block[2];
static pthread_key_t key;

/* Keeps the C library's allocator busy */
static void *
allocate(void *arg)
{
    (void)arg;
    while (!stop) {
        void *p = malloc(64);
        void *q = malloc(200);

        free(p);
        free(q);
    }
    return NULL;
}

/* Waits for a child for up to ten seconds, then kills it */
static int
wait_child(pid_t pid)
{
    struct timespec nap = { 0, 1000000 };
    int             status;

    for (int i = 0; i < 10000; i++) {
        pid_t r = waitpid(pid, &status, WNOHANG);

        check(r == 0 || r == pid, "waitpid");
        if (r == pid)
            return status;
        nanosleep(&nap, NULL);
    }
    kill(pid, SIGKILL);
    check(waitpid(pid, &status, 0) == pid, "waitpid after kill");
    printf("child hung\n");
    exit(1);
}

static void
note(void *arg)
{
    (void)write(report[1], arg, 1);
}

static void
destructor(void *arg)
{
    note(arg);
}

/* Blocks with a cleanup handler and thread-specific data, neither of which may run */
static void *
blocked(void *arg)
{
    char c;

    (void)arg;
    pthread_setspecific(key, "D");
    pthread_cleanup_push(note, "C");
    (void)write(report[1], "R", 1);
    (void)read(block[0], &c, 1);
    pthread_cleanup_pop(0);
    return NULL;
}

/* Runs a child with a second thread, which then either execs or calls _exit */
static void
end_with_thread(int use_exec)
{
    pid_t     pid;
    pthread_t t;
    char      buf[8];
    int       status;
    ssize_t   n;

    check(pipe(report) == 0 && pipe(block) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        if (pthread_key_create(&key, destructor) != 0
            || pthread_create(&t, NULL, blocked, NULL) != 0)
            _exit(2);
        /* Wait for the thread to be blocked */
        if (read(report[0], buf, 1) != 1)
            _exit(3);
        if (use_exec)
            execl(exe, ARGV0, "exit", "5", (char *)NULL);
        _exit(5);
    }
    check(close(report[1]) == 0, "close");
    status = wait_child(pid);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 5, "the process ended");
    n = read(report[0], buf, sizeof(buf));
    check(n == 0, "no cleanup handler or destructor ran");
    check(close(report[0]) == 0 && close(block[0]) == 0 && close(block[1]) == 0, "close");
}

int
main(int argc, char **argv)
{
    pthread_t t;
    ssize_t   len;

    if (argc > 2 && strcmp(argv[1], "exit") == 0)
        return atoi(argv[2]);
    if (argc > 1)
        return 99;

    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0) {
        printf("no /proc/self/exe, skipping\n");
        return 77;
    }
    exe[len] = '\0';

    check(pthread_create(&t, NULL, allocate, NULL) == 0, "pthread_create");
    for (int i = 0; i < 200; i++) {
        char * const envp[] = { NULL };
        pid_t        pid;
        int          status;

        fflush(stdout);
        pid = _Fork();
        check(pid >= 0, "_Fork");
        if (pid == 0) {
            if (i & 1)
                execl(exe, ARGV0, "exit", "3", (char *)NULL);
            else
                execle(exe, ARGV0, "exit", "3", (char *)NULL, envp);
            _exit(100 + errno);
        }
        status = wait_child(pid);
        check(WIFEXITED(status) && WEXITSTATUS(status) == 3, "the program ran");
    }
    stop = 1;
    check(pthread_join(t, NULL) == 0, "pthread_join");

    end_with_thread(1);
    end_with_thread(0);

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
