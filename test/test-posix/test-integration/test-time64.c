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

/* wait3 is an extension POSIX no longer has */
#define _DEFAULT_SOURCE
/*
 * Times beyond 32 bits must reach the kernel intact on 32-bit
 * architectures, and kernel structures must be converted, not copied.
 */

#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_CLOCK_SELECTION && TEST_POSIX_TIMERS && TEST_POSIX_MULTI_PROCESS \
    && TEST_POSIX_SIGNALS

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* A sleep longer than 2^31 seconds, so a 32-bit time can't hold it */
#define LONG_SLEEP ((time_t)INT32_MAX + 100)

static void
on_alarm(int sig)
{
    (void)sig;
}

static void
check_long_sleeps(void)
{
    struct timespec req, rem;
    int             ret;

    if (sizeof(time_t) < 8)
        return;

    req.tv_sec = LONG_SLEEP;
    req.tv_nsec = 0;

    alarm(1);
    errno = 0;
    ret = nanosleep(&req, &rem);
    check(ret == -1 && errno == EINTR, "nanosleep interrupted");
    check(rem.tv_sec > INT32_MAX, "nanosleep remaining time beyond 32 bits");

    alarm(1);
    ret = clock_nanosleep(CLOCK_REALTIME, 0, &req, &rem);
    check(ret == EINTR, "clock_nanosleep interrupted");
    check(rem.tv_sec > INT32_MAX, "clock_nanosleep remaining time beyond 32 bits");
}

static void
check_abs_timer(void)
{
    struct sigevent   sev = { 0 };
    struct itimerspec its = { 0 }, cur;
    struct timespec   now;
    timer_t           timer;
    time_t            expire;

    if (sizeof(time_t) < 8)
        return;

    /* An absolute expiry in 2040, past the end of 32-bit time */
    expire = (time_t)2208988800LL;
    sev.sigev_notify = SIGEV_NONE;
    check(timer_create(CLOCK_REALTIME, &sev, &timer) == 0, "timer_create");
    its.it_value.tv_sec = expire;
    check(timer_settime(timer, TIMER_ABSTIME, &its, NULL) == 0, "timer_settime past 2038");
    check(clock_gettime(CLOCK_REALTIME, &now) == 0, "clock_gettime");
    check(timer_gettime(timer, &cur) == 0, "timer_gettime");
    check(cur.it_value.tv_sec > expire - now.tv_sec - 60, "timer kept its expiry past 2038");
    check(timer_delete(timer) == 0, "timer_delete");
}

static void
check_itimers(void)
{
#ifdef __PICOLIBC__
    unsigned int left;

    /*
     * alarm can't fail; a huge request must stay huge, not wrap. The
     * kernel may cap it: FreeBSD's interval timers take at most
     * INT32_MAX / 2 seconds (34 years)
     */
    alarm(UINT_MAX);
    left = alarm(0);
    check(left >= 0x3fffffffu, "alarm(UINT_MAX) left a long alarm");

    /* setitimer rejects seconds the kernel's long can't hold */
#if LONG_MAX < INT64_MAX
    {
        struct itimerval itv = { 0 };

        itv.it_value.tv_sec = (time_t)LONG_MAX + 1;
        errno = 0;
        check(setitimer(ITIMER_REAL, &itv, NULL) == -1 && errno == EINVAL,
              "setitimer beyond the kernel's range");
    }
#endif
#endif
}

static void
check_wait3_rusage(void)
{
    struct rusage   ru;
    struct timespec start, now;
    int             status;
    pid_t           pid;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        /* Use about 0.2 s of CPU time */
        clock_gettime(CLOCK_MONOTONIC, &start);
        do {
            clock_gettime(CLOCK_MONOTONIC, &now);
        } while ((now.tv_sec - start.tv_sec) * 1000000000L + (now.tv_nsec - start.tv_nsec)
                 < 200000000L);
        _exit(0);
    }
    check(wait3(&status, 0, &ru) == pid, "wait3");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
    check(ru.ru_utime.tv_sec >= 0 && ru.ru_utime.tv_sec < 100, "ru_utime seconds");
    check(ru.ru_utime.tv_usec >= 0 && ru.ru_utime.tv_usec < 1000000, "ru_utime microseconds");
    check(ru.ru_stime.tv_sec >= 0 && ru.ru_stime.tv_sec < 100, "ru_stime seconds");
    check(ru.ru_stime.tv_usec >= 0 && ru.ru_stime.tv_usec < 1000000, "ru_stime microseconds");
    check(ru.ru_utime.tv_sec + ru.ru_utime.tv_usec + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec > 0,
          "child used some CPU time");
}

int
main(void)
{
    check(signal(SIGALRM, on_alarm) != SIG_ERR, "signal");
    check_long_sleeps();
    check_abs_timer();
    check_itimers();
    check_wait3_rusage();
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
