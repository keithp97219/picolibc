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
#if _POSIX_REALTIME_SIGNALS > 0 && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* sigtimedwait takes a pending signal, or times out */

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

/* Catch SIGUSR1 and SIGUSR2 with handler */
static void
catch_usr(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(sigaction(SIGUSR2, &act, NULL) == 0, "sigaction");
}

/* Block SIGUSR1 and SIGUSR2 so they can be waited for */
static void
block_usr(sigset_t *set)
{
    sigemptyset(set);
    sigaddset(set, SIGUSR1);
    sigaddset(set, SIGUSR2);
    check(sigprocmask(SIG_BLOCK, set, NULL) == 0, "sigprocmask");
}

int
main(void)
{
    sigset_t        set, pending;
    siginfo_t       si;
    struct timespec ts;
    union sigval    value;

    catch_usr();
    block_usr(&set);
    check(kill(getpid(), SIGUSR2) == 0, "kill");
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    check(sigtimedwait(&set, &si, &ts) == SIGUSR2, "sigtimedwait pending");
    check(si.si_code == SI_USER && si.si_pid == getpid(), "sigtimedwait info");

    ts.tv_sec = 0;
    ts.tv_nsec = 20 * 1000 * 1000;
    errno = 0;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EAGAIN, "sigtimedwait times out");
    ts.tv_nsec = 1000000000L;
    errno = 0;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EINVAL, "sigtimedwait bad timeout");
    ts.tv_nsec = -1;
    errno = 0;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EINVAL, "negative tv_nsec");

    /* A zero timeout returns at once */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    errno = 0;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EAGAIN, "zero timeout");

    /* Lower real-time signals are selected first, and a queued value is dequeued */
    sigaddset(&set, SIGRTMIN);
    sigaddset(&set, SIGRTMIN + 1);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block real-time signals");
    value.sival_int = 11;
    check(sigqueue(getpid(), SIGRTMIN + 1, value) == 0, "sigqueue SIGRTMIN+1");
    value.sival_int = 10;
    check(sigqueue(getpid(), SIGRTMIN, value) == 0, "sigqueue SIGRTMIN");
    ts.tv_sec = 1;
    check(sigtimedwait(&set, &si, &ts) == SIGRTMIN && si.si_signo == SIGRTMIN
              && si.si_code == SI_QUEUE && si.si_value.sival_int == 10,
          "lowest real-time signal first");
    check(sigtimedwait(&set, NULL, &ts) == SIGRTMIN + 1, "null info");
    check(sigpending(&pending) == 0 && !sigismember(&pending, SIGRTMIN + 1),
          "accepted signal no longer pending");

#ifndef __GLIBC__
    /*
     * The timeout need only be checked when the call would wait; glibc
     * passes it to Linux, which always checks it
     */
    check(kill(getpid(), SIGUSR1) == 0, "kill");
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000000L;
    check(sigtimedwait(&set, &si, &ts) == SIGUSR1, "bad timeout with a pending signal");
    /* A negative interval has already expired */
    ts.tv_sec = -1;
    ts.tv_nsec = 0;
    errno = 0;
    check(sigtimedwait(&set, &si, &ts) == -1 && errno == EAGAIN, "negative timeout");
#endif
    check(caught == 0, "waited-for signals were not caught");

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
