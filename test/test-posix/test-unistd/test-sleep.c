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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * sleep waits the full time unless a signal is caught or ends the
 * process, and then returns the time it didn't sleep
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    caught = sig;
}

static double
now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int
main(void)
{
    struct sigaction  sa;
    struct sigevent   sev;
    struct itimerspec its;
    timer_t           timer;
    double            start, elapsed;
    unsigned          left;
    pid_t             pid;
    int               status;

    check(sleep(0) == 0, "sleep(0)");

    /* A signal that is ignored doesn't end the sleep */
    signal(SIGUSR1, SIG_IGN);
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    check(timer_create(CLOCK_MONOTONIC, &sev, &timer) == 0, "timer_create");
    memset(&its, 0, sizeof(its));
    its.it_value.tv_nsec = 200000000;
    check(timer_settime(timer, 0, &its, NULL) == 0, "timer_settime");
    start = now();
    check(sleep(1) == 0, "sleep(1)");
    elapsed = now() - start;
    check(elapsed >= 1.0, "slept a second");
    check(timer_delete(timer) == 0, "timer_delete");

    /* A caught signal ends it early; the rest is returned */
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    alarm(1);
    start = now();
    left = sleep(10);
    elapsed = now() - start;
    check(caught == SIGALRM, "the handler ran");
    check(elapsed < 5.0, "woken by the signal");
    check(left >= 8 && left <= 9, "the time left");

    /* A signal whose action is to end the process ends it during the sleep */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        sleep(30);
        _exit(1);
    }
    check(kill(pid, SIGTERM) == 0, "kill");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM, "terminated");

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
