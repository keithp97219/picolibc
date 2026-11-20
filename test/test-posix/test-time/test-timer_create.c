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

#define _GNU_SOURCE /* for CLOCK_MONOTONIC until it is visible for POSIX.1-2008 */
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                              \
    || ((defined(_POSIX_CLOCK_SELECTION) && _POSIX_CLOCK_SELECTION > 0) \
        && (defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0))

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* POSIX timers signalling the process */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t fired;
static siginfo_t             last;

static void
handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    last = *info;
    fired++;
}

static void
sleep_ms(long ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };

    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) == EINTR)
        ;
}

int
main(void)
{
    struct sigaction  act;
    struct sigevent   ev;
    struct itimerspec its, old;
    struct timespec   now;
    timer_t           timer;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction SIGUSR1");
    check(sigaction(SIGALRM, &act, NULL) == 0, "sigaction SIGALRM");

    /* A one-shot timer delivers its signal and value */
    memset(&ev, 0, sizeof(ev));
    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = SIGUSR1;
    ev.sigev_value.sival_int = 1234;
    check(timer_create(CLOCK_MONOTONIC, &ev, &timer) == 0, "timer_create");
    memset(&its, 0, sizeof(its));
    its.it_value.tv_nsec = 10 * 1000 * 1000;
    check(timer_settime(timer, 0, &its, NULL) == 0, "timer_settime");
    sleep_ms(200);
    check(fired == 1, "one-shot timer fired once");
    check(last.si_signo == SIGUSR1 && last.si_code == SI_TIMER, "timer signal info");
    check(last.si_value.sival_int == 1234, "timer signal value");

    /* A periodic timer, checked with timer_gettime and stopped */
    fired = 0;
    its.it_interval.tv_nsec = 20 * 1000 * 1000;
    its.it_value.tv_nsec = 20 * 1000 * 1000;
    check(timer_settime(timer, 0, &its, &old) == 0, "timer_settime periodic");
    check(old.it_value.tv_sec == 0 && old.it_value.tv_nsec == 0, "old value of an expired timer");
    check(timer_gettime(timer, &its) == 0, "timer_gettime");
    check(its.it_interval.tv_nsec == 20 * 1000 * 1000 && its.it_value.tv_nsec > 0,
          "periodic timer armed");
    sleep_ms(300);
    check(fired >= 3, "periodic timer fired repeatedly");
    check(timer_getoverrun(timer) >= 0, "timer_getoverrun");
    memset(&its, 0, sizeof(its));
    check(timer_settime(timer, 0, &its, &old) == 0, "disarm");
    check(old.it_interval.tv_nsec == 20 * 1000 * 1000, "old interval");
    its.it_value.tv_nsec = 1000000000L;
    check(timer_settime(timer, 0, &its, NULL) == -1 && errno == EINVAL, "bad timer value");
    check(timer_delete(timer) == 0, "timer_delete");

    /* An absolute expiration time */
    fired = 0;
    check(timer_create(CLOCK_REALTIME, &ev, &timer) == 0, "timer_create realtime");
    clock_gettime(CLOCK_REALTIME, &now);
    memset(&its, 0, sizeof(its));
    its.it_value = now;
    its.it_value.tv_nsec += 20 * 1000 * 1000;
    if (its.it_value.tv_nsec >= 1000000000L) {
        its.it_value.tv_nsec -= 1000000000L;
        its.it_value.tv_sec++;
    }
    check(timer_settime(timer, TIMER_ABSTIME, &its, NULL) == 0, "timer_settime absolute");
    sleep_ms(200);
    check(fired == 1, "absolute timer fired");
    check(timer_delete(timer) == 0, "timer_delete");

    /* With no sigevent, SIGALRM carries the timer id */
    fired = 0;
    check(timer_create(CLOCK_MONOTONIC, NULL, &timer) == 0, "timer_create default");
    memset(&its, 0, sizeof(its));
    its.it_value.tv_nsec = 10 * 1000 * 1000;
    check(timer_settime(timer, 0, &its, NULL) == 0, "timer_settime default");
    sleep_ms(200);
    check(fired == 1 && last.si_signo == SIGALRM, "default timer signal");
    check(timer_delete(timer) == 0, "timer_delete default");

    /* SIGEV_NONE timers only count down */
    ev.sigev_notify = SIGEV_NONE;
    check(timer_create(CLOCK_MONOTONIC, &ev, &timer) == 0, "timer_create none");
    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    check(timer_settime(timer, 0, &its, NULL) == 0, "timer_settime none");
    check(timer_gettime(timer, &its) == 0 && its.it_value.tv_sec >= 9, "none timer counts down");
    check(timer_delete(timer) == 0, "timer_delete none");

    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = -1;
    check(timer_create(CLOCK_MONOTONIC, &ev, &timer) == -1 && errno == EINVAL, "bad signal");

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
