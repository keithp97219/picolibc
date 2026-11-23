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
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_TIMERS

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../is-qemu.h"

/*
 * timer_settime, timer_gettime and timer_getoverrun: disarmed timers,
 * relative readings of absolute timers, overruns, deletion and the
 * default notification. Delivery is checked with sigtimedwait and
 * generous timeouts, not with fixed sleeps.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static long long
mono_ns(void)
{
    struct timespec ts;

    check(clock_gettime(CLOCK_MONOTONIC, &ts) == 0, "clock_gettime");
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Wait up to 10 seconds for sig */
static int
wait_for(int sig, siginfo_t *info)
{
    sigset_t        set;
    struct timespec ts = { 10, 0 };

    sigemptyset(&set);
    sigaddset(&set, sig);
    return sigtimedwait(&set, info, &ts);
}

static int
pending(int sig)
{
    sigset_t set;

    check(sigpending(&set) == 0, "sigpending");
    return sigismember(&set, sig);
}

int
main(void)
{
    struct sigevent   ev;
    struct itimerspec its, old;
    struct timespec   now;
    timer_t           t1, t2;
    siginfo_t         info;
    sigset_t          block;
    long long         start;
    int               overrun;

    /* The signals are taken with sigtimedwait */
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGALRM);
    check(sigprocmask(SIG_BLOCK, &block, NULL) == 0, "sigprocmask");

    memset(&ev, 0, sizeof(ev));
    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = SIGUSR1;
    ev.sigev_value.sival_int = 42;
    check(timer_create(CLOCK_MONOTONIC, &ev, &t1) == 0, "timer_create");
    check(timer_create(CLOCK_MONOTONIC, &ev, &t2) == 0, "timer_create");
    check(t1 != t2, "timer ids are unique");

    /* A new timer is disarmed */
    memset(&its, 0x55, sizeof(its));
    check(timer_gettime(t1, &its) == 0, "timer_gettime");
    check(its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0 && its.it_interval.tv_sec == 0
              && its.it_interval.tv_nsec == 0,
          "new timer disarmed");

    /* An absolute timer reads back as the interval until it expires */
    check(clock_gettime(CLOCK_MONOTONIC, &now) == 0, "clock_gettime");
    its.it_value.tv_sec = now.tv_sec + 100;
    its.it_value.tv_nsec = now.tv_nsec;
    its.it_interval.tv_sec = 7;
    its.it_interval.tv_nsec = 0;
    check(timer_settime(t1, TIMER_ABSTIME, &its, NULL) == 0, "timer_settime absolute");
    check(timer_gettime(t1, &its) == 0, "timer_gettime");
    check(its.it_value.tv_sec <= 100 && its.it_value.tv_sec >= 90, "relative reading");
    check(its.it_interval.tv_sec == 7 && its.it_interval.tv_nsec == 0, "reload value");

    /* Resetting an armed timer; ovalue gets the old time left and reload */
    its.it_value.tv_sec = 50;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    check(timer_settime(t1, 0, &its, &old) == 0, "timer_settime reset");
    check(old.it_value.tv_sec <= 100 && old.it_value.tv_sec >= 90 && old.it_interval.tv_sec == 7,
          "ovalue");
    check(timer_gettime(t1, &its) == 0 && its.it_value.tv_sec <= 50 && its.it_value.tv_sec >= 40,
          "reset to the new value");

    /* A zero it_value disarms it; a bad interval doesn't matter then */
    memset(&its, 0, sizeof(its));
    check(timer_settime(t1, 0, &its, &old) == 0, "disarm");
    check(old.it_value.tv_sec <= 50 && old.it_value.tv_sec >= 40, "ovalue when disarming");
    check(timer_gettime(t1, &its) == 0 && its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0,
          "disarmed");

    /* EINVAL for a bad tv_nsec with a non-zero it_value */
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 1000000000L;
    errno = 0;
    check(timer_settime(t1, 0, &its, NULL) == -1 && errno == EINVAL, "it_value tv_nsec");
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_nsec = -1;
    errno = 0;
    check(timer_settime(t1, 0, &its, NULL) == -1 && errno == EINVAL, "it_interval tv_nsec");

    /* An absolute time in the past expires at once */
    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = 1;
    check(timer_settime(t1, TIMER_ABSTIME, &its, NULL) == 0, "absolute time in the past");
    check(wait_for(SIGUSR1, &info) == SIGUSR1, "past time fires");
    check(info.si_code == SI_TIMER && info.si_value.sival_int == 42, "signal information");

    /* Timers don't expire early */
    start = mono_ns();
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 50000000;
    check(timer_settime(t1, 0, &its, NULL) == 0, "relative timer");
    check(wait_for(SIGUSR1, &info) == SIGUSR1, "relative timer fires");
    check(mono_ns() - start >= 50000000, "not before its time");

    /* Overruns: one signal is queued, the rest are counted */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000000;
    its.it_interval = its.it_value;
    check(timer_settime(t1, 0, &its, NULL) == 0, "periodic timer");
    start = mono_ns();
    while (mono_ns() - start < 200000000)
        ;
    check(pending(SIGUSR1), "signal pending");
    check(wait_for(SIGUSR1, &info) == SIGUSR1, "periodic timer fired");
    overrun = timer_getoverrun(t1);
    check(overrun >= 10, "overruns counted");
    {
        long delaytimer_max = sysconf(_SC_DELAYTIMER_MAX);
        check(delaytimer_max == -1 || overrun <= delaytimer_max, "overrun within DELAYTIMER_MAX");
    }
    memset(&its, 0, sizeof(its));
    check(timer_settime(t1, 0, &its, NULL) == 0, "disarm");
    /* Drop a signal that may have been queued meanwhile */
    {
        struct timespec zero = { 0, 0 };
        sigset_t        set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        (void)sigtimedwait(&set, NULL, &zero);
    }

    /* Deleting an armed timer disarms it */
    its.it_value.tv_nsec = 50000000;
    check(timer_settime(t2, 0, &its, NULL) == 0, "arm");
    check(timer_delete(t2) == 0, "timer_delete armed");
    {
        struct timespec ts = { 0, 300000000 };
        sigset_t        set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        errno = 0;
        check(sigtimedwait(&set, NULL, &ts) == -1 && errno == EAGAIN, "no signal after delete");
    }
    check(timer_delete(t1) == 0, "timer_delete");

    /* With no sigevent: a default signal whose value is the timer id */
    check(timer_create(CLOCK_REALTIME, NULL, &t1) == 0, "timer_create default");
    memset(&its, 0, sizeof(its));
    its.it_value.tv_nsec = 10000000;
    check(timer_settime(t1, 0, &its, NULL) == 0, "arm default");
    check(wait_for(SIGALRM, &info) == SIGALRM, "default signal is SIGALRM");
    check(info.si_code == SI_TIMER, "SI_TIMER");
#ifdef __PICOLIBC__
    /* picolibc's timer_t is an integer id */
    /* qemu's user-mode emulation passes its own timer id */
    if (!is_qemu())
        check((intptr_t)info.si_value.sival_int == (intptr_t)t1, "default value is the timer id");
#endif
    check(timer_delete(t1) == 0, "timer_delete default");

#if TEST_POSIX_CPUTIME
    /* A timer on the process CPU-time clock, which the system may leave to run time */
    if (!TEST_POSIX_CPUTIME_RUNTIME || sysconf(_SC_CPUTIME) > 0) {
        ev.sigev_value.sival_int = 7;
        check(timer_create(CLOCK_PROCESS_CPUTIME_ID, &ev, &t1) == 0, "CPU-time timer");
        memset(&its, 0, sizeof(its));
        its.it_value.tv_nsec = 50000000;
        check(timer_settime(t1, 0, &its, NULL) == 0, "arm CPU-time timer");
        start = mono_ns();
        while (!pending(SIGUSR1) && mono_ns() - start < 10000000000LL)
            ;
        check(wait_for(SIGUSR1, &info) == SIGUSR1 && info.si_value.sival_int == 7,
              "CPU-time timer fires");
        check(timer_delete(t1) == 0, "timer_delete CPU-time");
    }
#endif

#if TEST_POSIX_THREAD_CPUTIME
    /* A timer on the thread CPU-time clock, which the system may leave to run time */
    if (!TEST_POSIX_THREAD_CPUTIME_RUNTIME || sysconf(_SC_THREAD_CPUTIME) > 0) {
        ev.sigev_value.sival_int = 8;
        check(timer_create(CLOCK_THREAD_CPUTIME_ID, &ev, &t1) == 0, "thread CPU-time timer");
        memset(&its, 0, sizeof(its));
        its.it_value.tv_nsec = 50000000;
        check(timer_settime(t1, 0, &its, NULL) == 0, "arm thread CPU-time timer");
        start = mono_ns();
        while (!pending(SIGUSR1) && mono_ns() - start < 10000000000LL)
            ;
        check(wait_for(SIGUSR1, &info) == SIGUSR1 && info.si_value.sival_int == 8,
              "thread CPU-time timer fires");
        check(timer_delete(t1) == 0, "timer_delete thread CPU-time");
    }
#endif

    /* EINVAL for an unknown clock */
    errno = 0;
    check(timer_create((clockid_t)12345, &ev, &t1) == -1 && errno == EINVAL, "bad clock");

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
