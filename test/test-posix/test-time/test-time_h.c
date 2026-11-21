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
/* daylight, timezone, strptime, getdate and getdate_err are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <time.h>
#include <unistd.h>

/* The types, macros, structures and prototypes in <time.h> */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Types from <sys/types.h>, <locale.h> and <signal.h> */
static clock_t           c;
static size_t            sz;
static time_t            tt;
static clockid_t         clk;
static timer_t           tmr;
static locale_t          loc;
static pid_t             pid;
static struct sigevent  *sev;
static struct timespec   ts;
static struct itimerspec its;

/* struct tm members and their types */
static struct tm         tm;
_Static_assert(_Generic(tm.tm_sec, int: 1, default: 0), "tm_sec");
_Static_assert(_Generic(tm.tm_min, int: 1, default: 0), "tm_min");
_Static_assert(_Generic(tm.tm_hour, int: 1, default: 0), "tm_hour");
_Static_assert(_Generic(tm.tm_mday, int: 1, default: 0), "tm_mday");
_Static_assert(_Generic(tm.tm_mon, int: 1, default: 0), "tm_mon");
_Static_assert(_Generic(tm.tm_year, int: 1, default: 0), "tm_year");
_Static_assert(_Generic(tm.tm_wday, int: 1, default: 0), "tm_wday");
_Static_assert(_Generic(tm.tm_yday, int: 1, default: 0), "tm_yday");
_Static_assert(_Generic(tm.tm_isdst, int: 1, default: 0), "tm_isdst");
/* glibc names these members only with _DEFAULT_SOURCE */
#if defined(__PICOLIBC__) || defined(__USE_MISC)
_Static_assert(_Generic(tm.tm_gmtoff, long: 1, default: 0), "tm_gmtoff");
_Static_assert(_Generic(tm.tm_zone, const char *: 1, default: 0), "tm_zone");
#endif

_Static_assert(_Generic(ts.tv_sec, time_t: 1, default: 0), "tv_sec");
_Static_assert(_Generic(ts.tv_nsec, long: 1, default: 0), "tv_nsec");
_Static_assert(_Generic(its.it_interval, struct timespec: 1, default: 0), "it_interval");
_Static_assert(_Generic(its.it_value, struct timespec: 1, default: 0), "it_value");

/* Macros */
_Static_assert(_Generic(CLOCKS_PER_SEC, clock_t: 1, default: 0), "CLOCKS_PER_SEC is a clock_t");
#if TIME_UTC <= 0
#error "TIME_UTC must be a positive constant usable in #if"
#endif
static clockid_t clocks[]
    = { CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_REALTIME, CLOCK_THREAD_CPUTIME_ID };
static int    flags = TIMER_ABSTIME;

/* Variables */
static int   *p_daylight = &daylight;
static long  *p_timezone = &timezone;
static char **p_tzname = tzname;

/* Prototypes */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static char      *(*p_asctime)(const struct tm *) = asctime;
static char      *(*p_ctime)(const time_t *) = ctime;
static double     (*p_difftime)(time_t, time_t) = difftime;
static struct tm *(*p_gmtime)(const time_t *) = gmtime;
static struct tm *(*p_gmtime_r)(const time_t * restrict, struct tm * restrict) = gmtime_r;
static struct tm *(*p_localtime)(const time_t *) = localtime;
static struct tm *(*p_localtime_r)(const time_t * restrict, struct tm * restrict) = localtime_r;
static time_t     (*p_mktime)(struct tm *) = mktime;
static size_t     (*p_strftime)(char     *restrict, size_t, const char     *restrict,
                            const struct tm     *restrict)
    = strftime;
static size_t (*p_strftime_l)(char * restrict, size_t, const char * restrict,
                              const struct tm * restrict, locale_t)
    = strftime_l;
static char *(*p_strptime)(const char * restrict, const char * restrict, struct tm * restrict)
    = strptime;
static void (*p_tzset)(void) = tzset;

/* The functions that need system support */
#if _POSIX_TIMERS > 0
#define CHECK_OS_PROTOTYPES
static clock_t (*p_clock)(void) = clock;
static int     (*p_clock_getcpuclockid)(pid_t, clockid_t *) = clock_getcpuclockid;
static int     (*p_clock_getres)(clockid_t, struct timespec *) = clock_getres;
static int     (*p_clock_gettime)(clockid_t, struct timespec *) = clock_gettime;
static int     (*p_clock_nanosleep)(clockid_t, int, const struct timespec *, struct timespec *)
    = clock_nanosleep;
static int        (*p_clock_settime)(clockid_t, const struct timespec *) = clock_settime;
static struct tm *(*p_getdate)(const char *) = getdate;
static int        (*p_nanosleep)(const struct timespec *, struct timespec *) = nanosleep;
static time_t     (*p_time)(time_t *) = time;
static int        (*p_timer_create)(clockid_t, struct sigevent        *restrict, timer_t        *restrict)
    = timer_create;
static int (*p_timer_delete)(timer_t) = timer_delete;
static int (*p_timer_getoverrun)(timer_t) = timer_getoverrun;
static int (*p_timer_gettime)(timer_t, struct itimerspec *) = timer_gettime;
static int (*p_timer_settime)(timer_t, int, const struct itimerspec * restrict,
                              struct itimerspec * restrict)
    = timer_settime;
static int (*p_timespec_get)(struct timespec *, int) = timespec_get;
#endif

int
main(void)
{
    size_t i, j;
    void  *ptrs[] = {
        (void *)p_asctime,
        (void *)p_ctime,
        (void *)p_difftime,
        (void *)p_gmtime,
        (void *)p_gmtime_r,
        (void *)p_localtime,
        (void *)p_localtime_r,
        (void *)p_mktime,
        (void *)p_strftime,
        (void *)p_strftime_l,
        (void *)p_strptime,
        (void *)p_tzset,
#ifdef CHECK_OS_PROTOTYPES
        (void *)p_clock,
        (void *)p_clock_getcpuclockid,
        (void *)p_clock_getres,
        (void *)p_clock_gettime,
        (void *)p_clock_nanosleep,
        (void *)p_clock_settime,
        (void *)p_getdate,
        (void *)p_nanosleep,
        (void *)p_time,
        (void *)p_timer_create,
        (void *)p_timer_delete,
        (void *)p_timer_getoverrun,
        (void *)p_timer_gettime,
        (void *)p_timer_settime,
        (void *)p_timespec_get,
#endif
    };

    (void)c;
    (void)sz;
    (void)tt;
    (void)clk;
    (void)tmr;
    (void)loc;
    (void)pid;
    (void)sev;
    (void)flags;

    for (i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++)
        check(ptrs[i] != NULL, "function present");

    /* XSI: CLOCKS_PER_SEC is one million */
    check(CLOCKS_PER_SEC == 1000000, "CLOCKS_PER_SEC");
    check(NULL == (void *)0, "NULL");

    /* The four clock ids are distinct */
    for (i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++)
        for (j = i + 1; j < sizeof(clocks) / sizeof(clocks[0]); j++)
            check(clocks[i] != clocks[j], "distinct clock ids");

    check(p_daylight && p_timezone && p_tzname, "variables");
    /* getdate_err is an int expression */
    check(_Generic(getdate_err, int: 1, default: 0), "getdate_err is an int");

    printf("success\n");
    return 0;
}
