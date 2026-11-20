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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Waiting for signals and queueing signals with values */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static siginfo_t             info;

static void
handler(int sig, siginfo_t *si, void *context)
{
    (void)context;
    caught = sig;
    info = *si;
}

int
main(void)
{
    struct sigaction act;
    struct timespec  ts;
    sigset_t         set;
    siginfo_t        si;
    union sigval     value;
    int              sig;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(sigaction(SIGUSR2, &act, NULL) == 0, "sigaction");

    /* A queued signal delivers its value to the handler */
    value.sival_int = 42;
    check(sigqueue(getpid(), SIGUSR1, value) == 0, "sigqueue");
    check(caught == SIGUSR1, "queued signal caught");
    check(info.si_signo == SIGUSR1 && info.si_code == SI_QUEUE, "queued signal info");
    check(info.si_value.sival_int == 42, "queued signal value");
    check(info.si_pid == getpid(), "queued signal sender");
    check(sigqueue(getpid(), 0, value) == 0, "sigqueue with no signal");
    check(sigqueue(getpid(), NSIG + 1, value) == -1 && errno == EINVAL, "sigqueue bad signal");

    /* Blocked signals can be waited for */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "sigprocmask");

    caught = 0;
    check(raise(SIGUSR2) == 0, "raise");
    check(caught == 0, "blocked signal pending");
    check(sigwait(&set, &sig) == 0 && sig == SIGUSR2, "sigwait");

    value.sival_int = 7;
    check(sigqueue(getpid(), SIGUSR1, value) == 0, "sigqueue blocked");
    memset(&si, 0, sizeof(si));
    check(sigwaitinfo(&set, &si) == SIGUSR1, "sigwaitinfo");
    check(si.si_signo == SIGUSR1 && si.si_code == SI_QUEUE && si.si_value.sival_int == 7,
          "sigwaitinfo info");

    check(kill(getpid(), SIGUSR2) == 0, "kill");
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    check(sigtimedwait(&set, &si, &ts) == SIGUSR2, "sigtimedwait pending");
    check(si.si_code == SI_USER && si.si_pid == getpid(), "sigtimedwait info");

    ts.tv_sec = 0;
    ts.tv_nsec = 20 * 1000 * 1000;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EAGAIN, "sigtimedwait times out");
    ts.tv_nsec = 1000000000L;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EINVAL, "sigtimedwait bad timeout");
    check(caught == 0, "waited-for signals were not caught");

    /* pause returns once a handler has run */
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0, "sigprocmask unblock");
    check(sigaction(SIGALRM, &act, NULL) == 0, "sigaction SIGALRM");
    alarm(1);
    check(pause() == -1 && errno == EINTR, "pause");
    check(caught == SIGALRM, "pause woken by SIGALRM");

    printf("success\n");
    return 0;
}
