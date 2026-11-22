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
/* glibc declares ppoll only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_PIPE

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ppoll reports a readable pipe, times out and rejects a bad timeout */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;

static void
on_usr1(int sig)
{
    (void)sig;
    caught++;
}

int
main(void)
{
    int              fds[2];
    struct pollfd    pfd[2];
    struct timespec  ts;
    sigset_t         mask, old, now;
    struct sigaction act;
    char             c;

    check(pipe(fds) == 0, "pipe");
    check(write(fds[1], "x", 1) == 1, "write");

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pfd[0].fd = fds[1];
    pfd[0].events = POLLIN;
    pfd[1].fd = fds[0];
    pfd[1].events = POLLIN;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(ppoll(pfd, 2, &ts, &mask) == 1 && (pfd[1].revents & POLLIN), "ppoll ready");

    check(read(fds[0], &c, 1) == 1 && c == 'x', "read");
    ts.tv_nsec = 20 * 1000 * 1000;
    check(ppoll(&pfd[1], 1, &ts, NULL) == 0 && pfd[1].revents == 0, "ppoll times out");
    ts.tv_nsec = -1;
    errno = 0;
    check(ppoll(&pfd[1], 1, &ts, NULL) == -1 && errno == EINVAL, "ppoll bad timeout");
    ts.tv_nsec = 1000000000;
    errno = 0;
    check(ppoll(&pfd[1], 1, &ts, NULL) == -1 && errno == EINVAL, "ppoll too many nanoseconds");
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    errno = 0;
    check(ppoll(&pfd[1], 1, &ts, NULL) == -1 && errno == EINVAL, "ppoll negative timeout");

    /* A long timeout, with something ready, is limited rather than rejected */
    check(write(fds[1], "z", 1) == 1, "write");
    ts.tv_sec = (time_t)1 << (sizeof(time_t) * 8 - 2);
    ts.tv_nsec = 999999999;
    check(ppoll(&pfd[1], 1, &ts, NULL) == 1 && pfd[1].revents == POLLIN, "long timeout");
    ts.tv_sec = 31 * 24 * 60 * 60;
    check(ppoll(&pfd[1], 1, &ts, NULL) == 1 && pfd[1].revents == POLLIN, "31 days");
    check(read(fds[0], &c, 1) == 1, "read");
    /* A negative descriptor is ignored */
    pfd[0].fd = -1;
    pfd[0].events = POLLIN;
    pfd[0].revents = POLLIN;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(ppoll(pfd, 1, &ts, NULL) == 0 && pfd[0].revents == 0, "negative descriptor");

    /*
     * The mask replaces the caller's during the call: a blocked, pending
     * SIGUSR1 is delivered, which interrupts it, and the old mask comes back
     */
    memset(&act, 0, sizeof(act));
    act.sa_handler = on_usr1;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &mask, &old) == 0, "block SIGUSR1");
    check(raise(SIGUSR1) == 0 && caught == 0, "pending");
    sigemptyset(&mask);
    errno = 0;
    check(ppoll(&pfd[1], 1, NULL, &mask) == -1 && errno == EINTR && caught == 1, "EINTR");
    check(sigprocmask(SIG_BLOCK, NULL, &now) == 0 && sigismember(&now, SIGUSR1), "mask restored");
    /* A mask that keeps it blocked leaves it pending */
    check(raise(SIGUSR1) == 0, "raise");
    sigaddset(&mask, SIGUSR1);
    ts.tv_nsec = 1000000;
    check(ppoll(&pfd[1], 1, &ts, &mask) == 0 && caught == 1, "still blocked");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0 && caught == 2, "delivered after");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

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
