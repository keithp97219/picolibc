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

/* tm_gmtoff and tm_zone, which glibc names only with _DEFAULT_SOURCE */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 202405L
/* timezone and daylight are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"
#include <time.h>

/* picolibc reads TZif files only when built with tzif=true */
#if defined(__PICOLIBC__) && !defined(__TZIF)
#define TEST_TZIF 0
#else
#define TEST_TZIF 1
#endif

/* Without the system support this test needs, it is skipped */
#if TEST_TZIF && TEST_POSIX_FILE_SYSTEM

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * localtime_r, mktime and tzset with zones from the system's time
 * zone database in /usr/share/zoneinfo, around their transitions,
 * before the first and after the last. The expected values come from
 * the IANA data, as Python's zoneinfo reports it.
 */

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

/* tm as "YYYY-MM-DD HH:MM:SS isdst gmtoff zone" */
static const char *
show(const struct tm *tm)
{
    static char buf[80];

    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %d %ld %s", tm->tm_year + 1900,
             tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst,
             tm->tm_gmtoff, tm->tm_zone ? tm->tm_zone : "(null)");
    return buf;
}

/*
 * localtime_r at t, and, when roundtrip is set, mktime back from what
 * it gives. When the clock goes back without a change of tm_isdst,
 * the local time repeated is ambiguous and mktime can give either.
 */
static void
check_local_rt(long long t, const char *want, int roundtrip)
{
    time_t      tt = (time_t)t, back;
    struct tm   tm;
    const char *got;

    check(localtime_r(&tt, &tm) == &tm, "localtime_r");
    got = show(&tm);
    if (strcmp(got, want) != 0) {
        printf("TZ=%s, localtime_r(%lld): got \"%s\", want \"%s\"\n", getenv("TZ"), t, got, want);
        exit(1);
    }
    if (!roundtrip)
        return;
    back = mktime(&tm);
    if (back != tt) {
        printf("TZ=%s, mktime(\"%s\"): got %lld, want %lld\n", getenv("TZ"), want, (long long)back,
               t);
        exit(1);
    }
}

#define check_local(t, want) check_local_rt(t, want, 1)

static void
check_mktime(int year, int mon, int mday, int hour, int min, int sec, int isdst, long long want_t,
             const char *want)
{
    struct tm   tm = { 0 };
    time_t      t;
    const char *got;

    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = isdst;
    t = mktime(&tm);
    got = show(&tm);
    if ((long long)t != want_t || strcmp(got, want) != 0) {
        printf("TZ=%s, mktime(%04d-%02d-%02d %02d:%02d:%02d isdst %d): got %lld \"%s\", want "
               "%lld \"%s\"\n",
               getenv("TZ"), year, mon, mday, hour, min, sec, isdst, (long long)t, got, want_t,
               want);
        exit(1);
    }
}

static void
check_names(const char *std, const char *dst, long tz, int dl)
{
    if (strcmp(tzname[0], std) != 0 || strcmp(tzname[1], dst) != 0 || timezone != tz
        || (daylight != 0) != dl) {
        printf("TZ=%s: got tzname %s/%s timezone %ld daylight %d, want %s/%s %ld %d\n",
               getenv("TZ"), tzname[0], tzname[1], timezone, daylight, std, dst, tz, dl);
        exit(1);
    }
}

int
main(void)
{
    char      buf[64], a[80];
    time_t    t;
    struct tm tm;
    int       fd;
    long long times[] = { 0, 1000000000LL, 1750000000LL, 2500000000LL };
    unsigned  i;

    fd = open("/usr/share/zoneinfo/America/New_York", O_RDONLY);
    if (fd < 0) {
        printf("no time zone database, skipping\n");
        return 77;
    }
    close(fd);
    unsetenv("TZDIR");

    /* Asia/Tokyo: a few years of DST after 1948, and no DST in the footer */
    set_tz("Asia/Tokyo");
    check_names("JST", "JDT", -32400, 1);
    check_local(-2840140800LL, "1880-01-01 09:18:59 0 33539 LMT");
    /* Japan Standard Time is 18:59 behind LMT */
    check_local_rt(-2587712401LL, "1888-01-01 00:18:58 0 33539 LMT", 0);
    check_local_rt(-2587712400LL, "1888-01-01 00:00:00 0 32400 JST", 0);
    check_local(-647049600LL, "1949-07-01 10:00:00 1 36000 JDT");
    check_local(-577962000LL, "1951-09-09 00:00:00 0 32400 JST");
    check_local(1735689600LL, "2025-01-01 09:00:00 0 32400 JST");
    check_local(2224713600LL, "2040-07-01 09:00:00 0 32400 JST");
    check_mktime(1949, 7, 1, 10, 0, 0, -1, -647049600LL, "1949-07-01 10:00:00 1 36000 JDT");

    /* America/New_York: LMT until 1883, war time, and the US rules */
    set_tz(":America/New_York");
    check_names("EST", "EDT", 18000, 1);
    /* EST is 3:58 behind LMT */
    check_local_rt(-2717650801LL, "1883-11-18 12:03:57 0 -17762 LMT", 0);
    check_local_rt(-2717650800LL, "1883-11-18 12:00:00 0 -18000 EST", 0);
    check_local(-839030400LL, "1943-05-31 20:00:00 1 -14400 EWT");
    check_local(-767923200LL, "1945-08-31 20:00:00 1 -14400 EPT");
    check_local(0LL, "1969-12-31 19:00:00 0 -18000 EST");
    check_local(1741503599LL, "2025-03-09 01:59:59 0 -18000 EST");
    check_local(1741503600LL, "2025-03-09 03:00:00 1 -14400 EDT");
    check_local(1762063199LL, "2025-11-02 01:59:59 1 -14400 EDT");
    check_local(1762063200LL, "2025-11-02 01:00:00 0 -18000 EST");
    check_local(2224713600LL, "2040-06-30 20:00:00 1 -14400 EDT");
    check_local(2871158400LL, "2060-12-24 19:00:00 0 -18000 EST");
    check_mktime(2025, 3, 9, 2, 30, 0, -1, 1741505400LL, "2025-03-09 03:30:00 1 -14400 EDT");
#ifndef TEST_GLIBC /* glibc picks DST */
    check_mktime(2025, 11, 2, 1, 30, 0, -1, 1762065000LL, "2025-11-02 01:30:00 0 -18000 EST");
#endif
    check_mktime(2025, 11, 2, 1, 30, 0, 1, 1762061400LL, "2025-11-02 01:30:00 1 -14400 EDT");
    check_mktime(2025, 11, 2, 1, 30, 0, 0, 1762065000LL, "2025-11-02 01:30:00 0 -18000 EST");
    check_mktime(2025, 1, 1, 12, 0, 0, 1, 1735747200LL, "2025-01-01 11:00:00 0 -18000 EST");
    /* mktime normalizes the fields */
    check_mktime(2025, 2, 29 + 31, 25, 0, 0, -1, 1743570000LL, "2025-04-02 01:00:00 1 -14400 EDT");
    t = (time_t)-839030400LL;
    check(localtime_r(&t, &tm) == &tm, "localtime_r EWT");
    check(strftime(buf, sizeof(buf), "%Z %z", &tm) > 0, "strftime");
    check(strcmp(buf, "EWT -0400") == 0, "strftime %Z %z");

    /* Europe/London: British Standard Time, an hour ahead all year */
    set_tz("Europe/London");
    check_names("GMT", "BST", 0, 1);
    check_local(-3881520000LL, "1846-12-31 23:58:45 0 -75 LMT");
    check_local(-902102400LL, "1941-06-01 02:00:00 1 7200 BDST");
    check_local(0LL, "1970-01-01 01:00:00 0 3600 BST");
    check_local(1743296399LL, "2025-03-30 00:59:59 0 0 GMT");
    check_local(1743296400LL, "2025-03-30 02:00:00 1 3600 BST");
    check_local(1761440399LL, "2025-10-26 01:59:59 1 3600 BST");
    check_local(1761440400LL, "2025-10-26 01:00:00 0 0 GMT");
    check_local(2540246400LL, "2050-07-01 01:00:00 1 3600 BST");
    check_mktime(1970, 6, 1, 12, 0, 0, -1, 13086000LL, "1970-06-01 12:00:00 0 3600 BST");

    /* Australia/Lord_Howe: half an hour of DST */
    set_tz("Australia/Lord_Howe");
    check_names("+1030", "+11", -37800, 1);
    check_local(-2524521600LL, "1890-01-01 10:36:20 0 38180 LMT");
    check_local(1743865199LL, "2025-04-06 01:59:59 1 39600 +11");
    check_local(1743865200LL, "2025-04-06 01:30:00 0 37800 +1030");
    check_local(1759591799LL, "2025-10-05 01:59:59 0 37800 +1030");
    check_local(1759591800LL, "2025-10-05 02:30:00 1 39600 +11");
    check_local(2366841600LL, "2045-01-01 11:00:00 1 39600 +11");
    check_mktime(2025, 10, 5, 2, 15, 0, -1, 1759592700LL, "2025-10-05 02:45:00 1 39600 +11");
#ifndef TEST_GLIBC /* glibc picks DST */
    check_mktime(2025, 4, 6, 1, 45, 0, -1, 1743866100LL, "2025-04-06 01:45:00 0 37800 +1030");
#endif
    check_mktime(2025, 4, 6, 1, 45, 0, 1, 1743864300LL, "2025-04-06 01:45:00 1 39600 +11");

    /* America/Sao_Paulo: DST until 2019, west of UTC */
    set_tz("America/Sao_Paulo");
    check_names("-03", "-02", 10800, 1);
    check_local(-2208988800LL, "1899-12-31 20:53:32 0 -11188 LMT");
    check_local(1541300399LL, "2018-11-03 23:59:59 0 -10800 -03");
    check_local(1541300400LL, "2018-11-04 01:00:00 1 -7200 -02");
    check_local(1550368799LL, "2019-02-16 23:59:59 1 -7200 -02");
    check_local(1550368800LL, "2019-02-16 23:00:00 0 -10800 -03");
    check_local(1735689600LL, "2024-12-31 21:00:00 0 -10800 -03");
    check_local(2366841600LL, "2044-12-31 21:00:00 0 -10800 -03");
    check_mktime(2018, 11, 4, 0, 30, 0, -1, 1541302200LL, "2018-11-04 01:30:00 1 -7200 -02");
    check_mktime(2025, 6, 1, 12, 0, 0, -1, 1748790000LL, "2025-06-01 12:00:00 0 -10800 -03");

    /* With TZ unset, /etc/localtime, as TZ=":/etc/localtime" gives it */
    fd = open("/etc/localtime", O_RDONLY);
    if (fd >= 0) {
        close(fd);
        for (i = 0; i < sizeof(times) / sizeof(times[0]); i++) {
            t = (time_t)times[i];
            set_tz(":/etc/localtime");
            check(localtime_r(&t, &tm) == &tm, "localtime_r");
            strcpy(a, show(&tm));
            check(unsetenv("TZ") == 0, "unsetenv TZ");
            tzset();
            check(localtime_r(&t, &tm) == &tm, "localtime_r");
            if (strcmp(a, show(&tm)) != 0) {
                printf("TZ unset at %lld: got \"%s\", want \"%s\"\n", times[i], show(&tm), a);
                exit(1);
            }
        }
    }

    /* An empty TZ is UTC, not /etc/localtime */
    set_tz("");
    check(timezone == 0 && daylight == 0, "empty TZ");
    t = 0;
    check(localtime_r(&t, &tm) == &tm && tm.tm_hour == 0 && tm.tm_gmtoff == 0, "empty TZ is UTC");

    printf("tzif zone tests passed\n");
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
