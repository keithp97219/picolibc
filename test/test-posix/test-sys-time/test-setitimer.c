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
/* getitimer and setitimer are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                  \
    || ((defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0) \
        && (defined(__PICOLIBC_XSI_SIGNALS) && __PICOLIBC_XSI_SIGNALS > 0))

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* setitimer arms interval timers that getitimer reports and that signal on expiry */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t alarms, vtalarms;

static int
is_set(const struct timeval *tv)
{
    return tv->tv_sec != 0 || tv->tv_usec != 0;
}

static void
handler(int sig)
{
    if (sig == SIGALRM)
        alarms++;
    else
        vtalarms++;
}

int
main(void)
{
    struct itimerval it, old, cur;
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGALRM, &sa, NULL) == 0, "SIGALRM");
    check(sigaction(SIGVTALRM, &sa, NULL) == 0, "SIGVTALRM");

    /* Disarmed at first */
    check(getitimer(ITIMER_REAL, &cur) == 0, "getitimer");
    check(!is_set(&cur.it_value) && !is_set(&cur.it_interval), "disarmed");

    /* A periodic real-time timer, reported while it runs */
    memset(&it, 0, sizeof(it));
    it.it_value.tv_usec = 20000;
    it.it_interval.tv_usec = 20000;
    check(setitimer(ITIMER_REAL, &it, &old) == 0, "setitimer");
    check(!is_set(&old.it_value), "old value");
    check(getitimer(ITIMER_REAL, &cur) == 0, "getitimer armed");
    check(is_set(&cur.it_value) && cur.it_value.tv_sec == 0 && cur.it_value.tv_usec <= 20000,
          "remaining");
    check(cur.it_interval.tv_sec == 0 && cur.it_interval.tv_usec == 20000, "interval");
    while (alarms < 3)
        pause();

    /* Disarming returns the old setting */
    memset(&it, 0, sizeof(it));
    check(setitimer(ITIMER_REAL, &it, &old) == 0, "disarm");
    check(old.it_interval.tv_usec == 20000, "old interval");
    check(getitimer(ITIMER_REAL, &cur) == 0 && !is_set(&cur.it_value), "disarmed again");

    /* A virtual timer counts only time spent running */
    it.it_value.tv_usec = 10000;
    check(setitimer(ITIMER_VIRTUAL, &it, NULL) == 0, "ITIMER_VIRTUAL");
    while (!vtalarms)
        ;

    errno = 0;
    check(setitimer(42, &it, NULL) == -1 && errno == EINVAL, "bad which");
    errno = 0;
    check(getitimer(42, &cur) == -1 && errno == EINVAL, "getitimer bad which");
    it.it_value.tv_usec = 1000000;
    errno = 0;
    check(setitimer(ITIMER_REAL, &it, NULL) == -1 && errno == EINVAL, "bad tv_usec");
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
