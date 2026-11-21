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
/* syslog is an XSI interface */
#define _XOPEN_SOURCE 800
/*
 * LOG_PERROR is an extension, and the only way to see a message
 * without reaching the system logger
 */
#define _DEFAULT_SOURCE
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_SYSTEM_LOGGING) && defined(__PICOLIBC_POSIX_PIPE)                    \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(_POSIX_THREADS) && _POSIX_THREADS > 0

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"

/*
 * A thread cancelled inside syslog, whether or not syslog is a
 * cancellation point, leaves logging usable by the other threads.
 *
 * As in test-syslog, the child has no descriptor to spare, so nothing
 * reaches the system log; LOG_PERROR writes to a pipe.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
capture(char *buf, size_t size)
{
    ssize_t n;

    n = read(0, buf, size - 1);
    buf[n > 0 ? n : 0] = '\0';
}

static void
isolate(void)
{
    struct rlimit rl;
    int           fds[2];

    check(pipe(fds) == 0, "pipe");
    check(dup2(fds[0], 0) == 0 && dup2(fds[1], 2) == 2, "redirect");
    if (fds[0] > 2)
        close(fds[0]);
    if (fds[1] > 2)
        close(fds[1]);
    check(fcntl(0, F_SETFL, O_NONBLOCK) == 0, "non-blocking");
    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    rl.rlim_cur = 3;
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "setrlimit");
    errno = 0;
    check(open("/dev/null", O_RDONLY) == -1 && errno == EMFILE, "no descriptors left");
}

/* Logs with a cancellation request already pending */
static void *
cancelled(void *arg)
{
    (void)arg;
    check(pthread_cancel(pthread_self()) == 0, "pthread_cancel");
    syslog(LOG_ERR, "while cancelled");
    pthread_testcancel();
    return NULL;
}

/* Waits to be cancelled */
static void *
waiter(void *arg)
{
    (void)arg;
    for (;;)
        pause();
    return NULL;
}

static void
child(void)
{
    char      buf[512];
    pthread_t thread;
    void     *result;

#if defined(__GLIBC__) && defined(__arm__)
    /*
     * glibc 2.43 on 32-bit ARM leaves syslog's lock held when a thread
     * is cancelled in syslog, so the next call hangs
     */
    printf("glibc on 32-bit ARM hangs after a cancel in syslog, skipping\n");
    exit(77);
#endif

    /* A hang shows up as SIGALRM */
    alarm(30);

    /*
     * Cancel a thread before the descriptors run out: glibc loads its
     * unwinder on the first cancellation
     */
    check(pthread_create(&thread, NULL, waiter, NULL) == 0, "pthread_create");
    check(pthread_cancel(thread) == 0 && pthread_join(thread, &result) == 0, "warm up");

    isolate();
    openlog("picolibc-test", LOG_PERROR, LOG_USER);

    check(pthread_create(&thread, NULL, cancelled, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == PTHREAD_CANCELED, "the thread was cancelled");
    while (read(0, buf, sizeof(buf)) > 0)
        ;

    syslog(LOG_ERR, "after the cancel");
    capture(buf, sizeof(buf));
    check(strstr(buf, "picolibc-test: after the cancel") != NULL, "syslog still works");
    closelog();
    openlog("again", LOG_PERROR, LOG_USER);
    syslog(LOG_ERR, "reopened");
    capture(buf, sizeof(buf));
    check(strstr(buf, "again: reopened") != NULL, "openlog still works");
    exit(0);
}

int
main(void)
{
    pid_t pid;
    int   status;

    fflush(stdout);
#ifdef __hexagon__
    /* qemu-hexagon can abort a forked child that creates threads; check here */
    if (is_qemu())
        child();
#endif
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        child();
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (WIFEXITED(status) && WEXITSTATUS(status) == 77)
        return 77;
    if (WIFSIGNALED(status))
        printf("child killed by signal %d\n", WTERMSIG(status));
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child");
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
