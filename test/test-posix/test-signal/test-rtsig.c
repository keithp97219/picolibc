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
#if TEST_POSIX_REALTIME_SIGNALS && TEST_POSIX_SIGNALS

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Real-time signals queue with their values and arrive lowest first */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static volatile int          caught_value;

static void
handler(int sig, siginfo_t *info, void *context)
{
    (void)context;
    caught = sig;
    caught_value = info->si_value.sival_int;
}

int
main(void)
{
    struct sigaction act;
    union sigval     value;
    sigset_t         set;
    siginfo_t        info;
    int              sigs[3];

    check(SIGRTMAX - SIGRTMIN + 1 >= 8, "number of real-time signals");
    check(SIGRTMIN > SIGUSR2 && SIGRTMIN > SIGALRM, "real-time signals follow the others");

    /*
     * qemu's user-mode emulation keeps the last few real-time signals
     * for itself, so stay well below SIGRTMAX
     */
    sigs[0] = SIGRTMIN;
    sigs[1] = SIGRTMIN + 5;
    sigs[2] = SIGRTMIN + 20;

    /* Sets hold every real-time signal */
    sigemptyset(&set);
    for (int i = 0; i < 3; i++)
        check(sigaddset(&set, sigs[i]) == 0, "sigaddset");
    for (int i = 0; i < 3; i++)
        check(sigismember(&set, sigs[i]), "sigismember");
    check(!sigismember(&set, SIGRTMIN + 1), "other signals not members");
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "sigprocmask block");

    /* Each queued instance stays pending with its value */
    for (int v = 0; v < 3; v++) {
        value.sival_int = 100 + v;
        check(sigqueue(getpid(), sigs[2], value) == 0, "sigqueue high signal");
    }
    value.sival_int = 200;
    check(sigqueue(getpid(), SIGRTMIN + 5, value) == 0, "sigqueue SIGRTMIN+5");
    value.sival_int = 300;
    check(sigqueue(getpid(), SIGRTMIN, value) == 0, "sigqueue SIGRTMIN");

    check(sigpending(&set) == 0, "sigpending");
    for (int i = 0; i < 3; i++)
        check(sigismember(&set, sigs[i]), "pending real-time signal");

    sigemptyset(&set);
    for (int i = 0; i < 3; i++)
        sigaddset(&set, sigs[i]);

    /* Lowest numbered first, then in the order queued */
    check(sigwaitinfo(&set, &info) == SIGRTMIN && info.si_value.sival_int == 300, "first");
    check(sigwaitinfo(&set, &info) == SIGRTMIN + 5 && info.si_value.sival_int == 200, "second");
    for (int v = 0; v < 3; v++) {
        check(sigwaitinfo(&set, &info) == sigs[2], "high signal queued");
        check(info.si_signo == sigs[2] && info.si_code == SI_QUEUE, "high signal info");
        check(info.si_value.sival_int == 100 + v, "high signal values in order");
    }

    /* Handlers for real-time signals */
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGRTMIN + 2, &act, NULL) == 0, "sigaction");
    value.sival_int = 42;
    check(sigqueue(getpid(), SIGRTMIN + 2, value) == 0, "sigqueue to handler");
    check(caught == SIGRTMIN + 2 && caught_value == 42, "handler ran");
    check(raise(SIGRTMIN + 2) == 0, "raise");
    check(kill(getpid(), SIGRTMAX + 1) == -1 && errno == EINVAL, "past SIGRTMAX");

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
