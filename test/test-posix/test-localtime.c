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

#define _POSIX_C_SOURCE 200809L
#include <time.h>

/*
 * gmtime_r fails with EOVERFLOW when the year doesn't fit, and
 * localtime applies the TZ rule to every year, including rules whose
 * transitions fall in the next or previous year
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
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

static void
set_tz(const char *tz)
{
    check(setenv("TZ", tz, 1) == 0, "setenv TZ");
    tzset();
}

/* Local time at t, as "YYYY-MM-DD HH:MM:SS isdst" */
static const char *
local(time_t t)
{
    static char buf[64];
    struct tm   tm, *ret;

    ret = localtime_r(&t, &tm);
    check(ret == &tm, "localtime_r returns its result");
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %d", tm.tm_year + 1900, tm.tm_mon + 1,
             tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_isdst);
    return buf;
}

#define check_local(t, want)                                                                     \
    do {                                                                                         \
        const char *_got = local(t);                                                             \
        if (strcmp(_got, want) != 0) {                                                           \
            printf("TZ=%s, %lld: got \"%s\", want \"%s\"\n", getenv("TZ"), (long long)(t), _got, \
                   want);                                                                        \
            exit(1);                                                                             \
        }                                                                                        \
    } while (0)

static int
leap(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

/* Seconds since the Epoch for a UTC date, as XBD 4.19 defines them */
static time_t
utc(int year, int mon, int mday, int hour, int min, int sec)
{
    static const int before[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    long long        days = before[mon - 1] + mday - 1 + (leap(year) && mon > 2);
    int              y;

    for (y = 1970; y < year; y++)
        days += 365 + leap(y);
    for (y = year; y < 1970; y++)
        days -= 365 + leap(y);
    return (time_t)(days * 86400 + hour * 3600 + min * 60 + sec);
}

int
main(void)
{
    struct tm tm;
    time_t    t;

    /* EOVERFLOW when the year doesn't fit in an int */
    if (sizeof(time_t) > sizeof(int)) {
        t = (time_t)INT64_MAX;
        errno = 0;
        check(gmtime_r(&t, &tm) == NULL && errno == EOVERFLOW, "gmtime_r EOVERFLOW");
        errno = 0;
        check(gmtime(&t) == NULL && errno == EOVERFLOW, "gmtime EOVERFLOW");
    }

    set_tz("EST5EDT,M3.2.0,M11.1.0");
    check_local(utc(2025, 3, 9, 6, 59, 59), "2025-03-09 01:59:59 0");
    check_local(utc(2025, 3, 9, 7, 0, 0), "2025-03-09 03:00:00 1");

    /* The rules apply to every year, never giving a negative tm_isdst */
#ifndef __GLIBC__ /* glibc applies them only from 1970 */
    check_local(utc(1960, 7, 1, 12, 0, 0), "1960-07-01 08:00:00 1");
    check_local(utc(1960, 1, 1, 12, 0, 0), "1960-01-01 07:00:00 0");
    check_local(utc(1901, 6, 1, 12, 0, 0), "1901-06-01 08:00:00 1");
#endif

    /* Southern hemisphere: DST spans the end of the year */
    set_tz("AEST-10AEDT,M10.1.0,M4.1.0/3");
    check_local(utc(2025, 1, 15, 0, 0, 0), "2025-01-15 11:00:00 1");
    check_local(utc(2025, 7, 15, 0, 0, 0), "2025-07-15 10:00:00 0");
    check_local(utc(2024, 12, 31, 14, 0, 0), "2025-01-01 01:00:00 1");

    /* XBD 8.3: DST all year */
    set_tz("EST5EDT,0/0,J365/25");
#ifndef __GLIBC__ /* glibc misses the transition in the next year */
    check_local(utc(2025, 1, 1, 0, 0, 0), "2024-12-31 20:00:00 1");
    check_local(utc(2025, 1, 1, 4, 30, 0), "2025-01-01 00:30:00 1");
#endif
    check_local(utc(2025, 6, 1, 0, 0, 0), "2025-05-31 20:00:00 1");
    check_local(utc(2025, 12, 31, 23, 0, 0), "2025-12-31 19:00:00 1");

    printf("success\n");
    return 0;
}
