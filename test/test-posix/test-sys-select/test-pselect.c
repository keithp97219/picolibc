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
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_PIPE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

/* pselect with a signal mask reports a readable pipe */

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
    fd_set           rset;
    struct timespec  ts;
    sigset_t         mask, old, now;
    fd_set           saved;
    struct sigaction act;
    char             c;

    check(pipe(fds) == 0, "pipe");
    check(write(fds[1], "x", 1) == 1, "write");

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, &mask) == 1, "pselect read ready");
    check(FD_ISSET(fds[0], &rset), "pselect read set");
    /* A zero timeout polls */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, NULL) == 1, "poll");
    check(read(fds[0], &c, 1) == 1 && c == 'x', "read");
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, NULL) == 0 && !FD_ISSET(fds[0], &rset),
          "nothing ready");
    /* Bad arguments; the sets are left alone */
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    saved = rset;
    ts.tv_nsec = 1000000000;
    errno = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, NULL) == -1 && errno == EINVAL,
          "tv_nsec too big");
    ts.tv_nsec = -1;
    errno = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, NULL) == -1 && errno == EINVAL,
          "tv_nsec negative");
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    errno = 0;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, NULL) == -1 && errno == EINVAL,
          "tv_sec negative");
    errno = 0;
    check(pselect(-1, &rset, NULL, NULL, NULL, NULL) == -1 && errno == EINVAL, "nfds negative");
    check(memcmp(&rset, &saved, sizeof(rset)) == 0, "unchanged after EINVAL");
    /* glibc passes on Linux, which accepts any count */
#ifndef __GLIBC__
    ts.tv_sec = 0;
    errno = 0;
    check(pselect(FD_SETSIZE + 1, NULL, NULL, NULL, &ts, NULL) == -1 && errno == EINVAL,
          "more than FD_SETSIZE");
#endif

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
    check(pselect(fds[0] + 1, &rset, NULL, NULL, NULL, &mask) == -1 && errno == EINTR
              && caught == 1,
          "EINTR");
    check(memcmp(&rset, &saved, sizeof(rset)) == 0, "unchanged after EINTR");
    check(sigprocmask(SIG_BLOCK, NULL, &now) == 0 && sigismember(&now, SIGUSR1), "mask restored");
    /* A mask that keeps it blocked leaves it pending */
    check(raise(SIGUSR1) == 0, "raise");
    sigaddset(&mask, SIGUSR1);
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    check(pselect(fds[0] + 1, &rset, NULL, NULL, &ts, &mask) == 0 && caught == 1, "still blocked");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0 && caught == 2, "delivered after");
    /* No sets: wait for the time */
    ts.tv_nsec = 1000000;
    check(pselect(0, NULL, NULL, NULL, &ts, NULL) == 0, "just a timeout");
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
