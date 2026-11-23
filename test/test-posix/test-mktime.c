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
 * mktime never leaves tm_isdst negative, sets the members as
 * localtime would, and can return -1 as a valid result
 */

#include <errno.h>
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

static struct tm
mk(int year, int mon, int mday, int hour, int min, int sec, int isdst)
{
    struct tm tm;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = isdst;
    tm.tm_wday = 9;
    tm.tm_yday = 999;
    return tm;
}

/* The members must be those localtime gives for the result */
static void
check_members(const struct tm *tm, time_t t, const char *what)
{
    struct tm lt;

    check(localtime_r(&t, &lt) != NULL, what);
    if (tm->tm_year != lt.tm_year || tm->tm_mon != lt.tm_mon || tm->tm_mday != lt.tm_mday
        || tm->tm_hour != lt.tm_hour || tm->tm_min != lt.tm_min || tm->tm_sec != lt.tm_sec
        || tm->tm_wday != lt.tm_wday || tm->tm_yday != lt.tm_yday || tm->tm_isdst != lt.tm_isdst) {
        printf("%s: members differ from localtime\n", what);
        exit(1);
    }
    check(tm->tm_isdst >= 0, what);
}

int
main(void)
{
    struct tm tm;
    time_t    t;

    /* -1 is a valid result, distinguished by the change to tm_wday */
    set_tz("UTC0");
    tm = mk(1969, 12, 31, 23, 59, 59, 0);
    errno = 0;
    check(mktime(&tm) == (time_t)-1 && tm.tm_wday == 3 && errno == 0, "a result of -1");

    set_tz("EST5EDT,M3.2.0,M11.1.0");

    /* tm_isdst 0: no DST adjustment, even in summer */
    tm = mk(2025, 7, 1, 12, 0, 0, 0);
    t = mktime(&tm);
    check(t == 1751389200, "isdst 0 in summer is standard time");
    check_members(&tm, t, "isdst 0 members");

    /* The repeated and the skipped hour: either reading, never -1 */
    tm = mk(2025, 11, 2, 1, 30, 0, -1);
    t = mktime(&tm);
    check(t == 1762061400 || t == 1762065000, "repeated hour");
    check_members(&tm, t, "repeated hour members");
    tm = mk(2025, 3, 9, 2, 30, 0, -1);
    t = mktime(&tm);
    check(t == 1741505400 || t == 1741501800, "skipped hour");
    check_members(&tm, t, "skipped hour members");

    /* Before 1970 the rule still applies (not in glibc) */
#ifndef __GLIBC__
    tm = mk(1960, 7, 1, 12, 0, 0, -1);
    t = mktime(&tm);
    check(t == -299836800 && tm.tm_isdst == 1, "1960 summer");
    check_members(&tm, t, "1960 members");
#endif

    printf("success\n");
    return 0;
}
