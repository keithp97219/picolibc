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
 * tzset, localtime_r, mktime and strftime with time zones read from
 * TZif files (RFC 8536 and RFC 9636), written here to a temporary
 * directory:
 *
 *  v1:   version 1, 32-bit data, no footer: types AAA (+1, std),
 *        AAD (+2, dst) and AAB (+1:30, std); AAD from -1000000000
 *        (1938), AAA from -990000000, AAB from 1000000000 (2001) on.
 *  v2:   version 2, with a different version 1 block (one type, BAD,
 *        +5) to be skipped. Types LMT (-4:56:02), XST (-5), XDT (-4,
 *        dst), with transitions before 1901 (-3000000000), in 1938,
 *        in 2049 after 2038 and a last one at 2100-01-01, then the
 *        footer XST5XDT,M3.2.0,M11.1.0. 2052 has no DST, as the
 *        transitions say, though the footer rule would.
 *  v3:   version 3, a footer with rule times before midnight,
 *        <-02>2<-01>,M3.5.0/-1,M10.5.0/0, after transitions to -03
 *        (1916) and -02 (2023) that have no DST.
 *  v4:   version 4, with leap seconds (ignored) and standard/wall and
 *        UT/local indicators, types LMT, +0530 and +0545 and a footer
 *        east of UTC without DST, <+0545>-5:45.
 *  west: version 2, -03 with one period of -02 DST in 2018-2019, and
 *        a footer west of UTC without DST, <-03>3.
 */

static const unsigned char tzif_v1[] = {
    0x54, 0x5a, 0x69, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0c, 0xc4,
    0x65, 0x36, 0x00, 0xc4, 0xfd, 0xcc, 0x80, 0x3b, 0x9a, 0xca, 0x00, 0x01, 0x00, 0x02, 0x00,
    0x00, 0x0e, 0x10, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x01, 0x04, 0x00, 0x00, 0x15, 0x18,
    0x00, 0x08, 0x41, 0x41, 0x41, 0x00, 0x41, 0x41, 0x44, 0x00, 0x41, 0x41, 0x42, 0x00,
};

static const unsigned char tzif_v2[] = {
    0x54, 0x5a, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x46, 0x50,
    0x00, 0x00, 0x42, 0x41, 0x44, 0x00, 0x54, 0x5a, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x0c, 0xff, 0xff, 0xff, 0xff, 0x4d, 0x2f, 0xa2, 0x00, 0xff, 0xff, 0xff, 0xff, 0xc4, 0x65,
    0x36, 0x00, 0xff, 0xff, 0xff, 0xff, 0xc4, 0xfd, 0xcc, 0x80, 0x00, 0x00, 0x00, 0x00, 0x95, 0x02,
    0xf9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0x9b, 0x8f, 0x80, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x86,
    0x57, 0x00, 0x01, 0x02, 0x01, 0x02, 0x01, 0x01, 0xff, 0xff, 0xba, 0x9e, 0x00, 0x00, 0xff, 0xff,
    0xb9, 0xb0, 0x00, 0x04, 0xff, 0xff, 0xc7, 0xc0, 0x01, 0x08, 0x4c, 0x4d, 0x54, 0x00, 0x58, 0x53,
    0x54, 0x00, 0x58, 0x44, 0x54, 0x00, 0x0a, 0x58, 0x53, 0x54, 0x35, 0x58, 0x44, 0x54, 0x2c, 0x4d,
    0x33, 0x2e, 0x32, 0x2e, 0x30, 0x2c, 0x4d, 0x31, 0x31, 0x2e, 0x31, 0x2e, 0x30, 0x0a,
};

static const unsigned char tzif_v3[] = {
    0x54, 0x5a, 0x69, 0x66, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0xff, 0xff, 0xcf, 0x80,
    0x00, 0x00, 0x4c, 0x4d, 0x54, 0x00, 0x54, 0x5a, 0x69, 0x66, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x00, 0x10, 0xff, 0xff, 0xff, 0xff, 0x9b, 0x80, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x3d,
    0xae, 0x90, 0x01, 0x02, 0xff, 0xff, 0xcf, 0x80, 0x00, 0x00, 0xff, 0xff, 0xd5, 0xd0, 0x00, 0x04,
    0xff, 0xff, 0xe3, 0xe0, 0x00, 0x08, 0xff, 0xff, 0xf1, 0xf0, 0x01, 0x0c, 0x4c, 0x4d, 0x54, 0x00,
    0x2d, 0x30, 0x33, 0x00, 0x2d, 0x30, 0x32, 0x00, 0x2d, 0x30, 0x31, 0x00, 0x0a, 0x3c, 0x2d, 0x30,
    0x32, 0x3e, 0x32, 0x3c, 0x2d, 0x30, 0x31, 0x3e, 0x2c, 0x4d, 0x33, 0x2e, 0x35, 0x2e, 0x30, 0x2f,
    0x2d, 0x31, 0x2c, 0x4d, 0x31, 0x30, 0x2e, 0x35, 0x2e, 0x30, 0x2f, 0x30, 0x0a,
};

static const unsigned char tzif_v4[] = {
    0x54, 0x5a, 0x69, 0x66, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x4f, 0xfc, 0x00, 0x00, 0x4c, 0x4d, 0x54, 0x00, 0x54, 0x5a, 0x69, 0x66, 0x34, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0xff, 0xff, 0xff, 0xff, 0xa1, 0xf2, 0xcd,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x18, 0x30, 0xa8, 0x01, 0x02, 0x00, 0x00, 0x4f, 0xfc,
    0x00, 0x00, 0x00, 0x00, 0x4d, 0x58, 0x00, 0x04, 0x00, 0x00, 0x50, 0xdc, 0x00, 0x0a, 0x4c,
    0x4d, 0x54, 0x00, 0x2b, 0x30, 0x35, 0x33, 0x30, 0x00, 0x2b, 0x30, 0x35, 0x34, 0x35, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x71, 0x3f, 0xb3, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x74, 0x3a, 0xa3, 0x81, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0a, 0x3c, 0x2b, 0x30, 0x35, 0x34, 0x35, 0x3e, 0x2d, 0x35, 0x3a, 0x34, 0x35, 0x0a,
};

static const unsigned char tzif_west[] = {
    0x54, 0x5a, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0xff,
    0xff, 0xd4, 0x4c, 0x00, 0x00, 0x4c, 0x4d, 0x54, 0x00, 0x54, 0x5a, 0x69, 0x66, 0x32, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0c, 0xff, 0xff, 0xff, 0xff, 0x96, 0xaa, 0x72,
    0xb4, 0x00, 0x00, 0x00, 0x00, 0x5b, 0xde, 0x60, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x68,
    0xc0, 0x20, 0x01, 0x02, 0x01, 0xff, 0xff, 0xd4, 0x4c, 0x00, 0x00, 0xff, 0xff, 0xd5, 0xd0,
    0x00, 0x04, 0xff, 0xff, 0xe3, 0xe0, 0x01, 0x08, 0x4c, 0x4d, 0x54, 0x00, 0x2d, 0x30, 0x33,
    0x00, 0x2d, 0x30, 0x32, 0x00, 0x0a, 0x3c, 0x2d, 0x30, 0x33, 0x3e, 0x33, 0x0a,
};

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[64];

/* Write a file in the temporary directory, returning its name */
static const char *
write_file(const char *name, const unsigned char *data, size_t len)
{
    static char path[128];
    int         fd;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    check(fd >= 0, "create TZif file");
    check(write(fd, data, len) == (ssize_t)len, "write TZif file");
    check(close(fd) == 0, "close TZif file");
    return path;
}

static void
set_tz(const char *tz)
{
    check(setenv("TZ", tz, 1) == 0, "setenv TZ");
    tzset();
}

/* Use the file holding data as the time zone, by TZ=":path" */
static void
use_file(const char *name, const unsigned char *data, size_t len)
{
    char tz[160];

    snprintf(tz, sizeof(tz), ":%s", write_file(name, data, len));
    set_tz(tz);
}

#define USE(name) use_file(#name, tzif_##name, sizeof(tzif_##name))

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

static void
check_local(long long t, const char *want)
{
    time_t      tt = (time_t)t;
    struct tm   tm;
    const char *got;

    check(localtime_r(&tt, &tm) == &tm, "localtime_r");
    got = show(&tm);
    if (strcmp(got, want) != 0) {
        printf("TZ=%s, localtime_r(%lld): got \"%s\", want \"%s\"\n", getenv("TZ"), t, got, want);
        exit(1);
    }
}

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

/* A time zone that didn't load leaves what TZ would give without TZif */
static void
check_fallback(const char *what)
{
    if (timezone != 0 || daylight != 0) {
        printf("%s: timezone %ld daylight %d, want UTC\n", what, timezone, daylight);
        exit(1);
    }
#ifdef __PICOLIBC__
    if (strcmp(tzname[0], "GMT") != 0) {
        printf("%s: tzname[0] %s, want GMT\n", what, tzname[0]);
        exit(1);
    }
    check_local(0, "1970-01-01 00:00:00 0 0 GMT");
#endif
}

static unsigned char bad[sizeof(tzif_v2)];

/*
 * Change the bytes of v2 at off and see that the file is rejected.
 * The version 2 header is at 54, after the version 1 header and its
 * one type and abbreviation, and the data at 98: 6 transitions from
 * 98, their types from 146, 3 time types from 152, 12 bytes of
 * abbreviations from 170 and the footer from 182.
 */
/* glibc is set when glibc rejects the file too */
static void
check_bad(const char *what, size_t off, const void *bytes, size_t n, size_t len, int glibc)
{
    memcpy(bad, tzif_v2, sizeof(bad));
    memcpy(bad + off, bytes, n);
    set_tz("EST5");
    use_file("bad", bad, len);
#ifdef TEST_GLIBC
    if (!glibc)
        return;
#endif
    (void)glibc;
    check_fallback(what);
}

int
main(void)
{
    char        buf[64];
    char        tz[160];
    time_t      t;
    struct tm   tm;
    const char *path;

    strcpy(dir, "/tmp/tzif-XXXXXX");
    check(mkdtemp(dir) != NULL, "mkdtemp");

    /* Version 1: 32-bit times, and the last type applies after the last transition */
    USE(v1);
    check_names("AAB", "AAD", -5400, 1);
    check_local(-2000000000LL, "1906-08-16 21:26:40 0 3600 AAA");
    check_local(-1000000001LL, "1938-04-24 23:13:19 0 3600 AAA");
    check_local(-1000000000LL, "1938-04-25 00:13:20 1 7200 AAD");
    check_local(-995000000LL, "1938-06-21 21:06:40 1 7200 AAD");
    check_local(-990000000LL, "1938-08-18 17:00:00 0 3600 AAA");
    check_local(0LL, "1970-01-01 01:00:00 0 3600 AAA");
    check_local(999999999LL, "2001-09-09 02:46:39 0 3600 AAA");
    check_local(1000000000LL, "2001-09-09 03:16:40 0 5400 AAB");
    check_local(2000000000LL, "2033-05-18 05:03:20 0 5400 AAB");
    check_local(4000000000LL, "2096-10-02 08:36:40 0 5400 AAB");
    /* In the gap at the start of AAD, and after it */
    check_mktime(1938, 4, 24, 23, 30, 0, -1, -999999000LL, "1938-04-25 00:30:00 1 7200 AAD");
    check_mktime(1938, 4, 25, 0, 30, 0, -1, -999999000LL, "1938-04-25 00:30:00 1 7200 AAD");
    /* Before a gap when the offset grows by half an hour */
    check_mktime(2001, 9, 9, 2, 30, 0, -1, 999999000LL, "2001-09-09 02:30:00 0 3600 AAA");
    check_mktime(2001, 9, 9, 3, 0, 0, -1, 1000000800LL, "2001-09-09 03:30:00 0 5400 AAB");
    check_mktime(1970, 1, 1, 0, 0, 0, 0, -3600LL, "1970-01-01 00:00:00 0 3600 AAA");
    check_mktime(2096, 1, 1, 0, 0, 0, -1, 3976209000LL, "2096-01-01 00:00:00 0 5400 AAB");
    /* Repeated at the end of AAD: standard time, unless tm_isdst asks for DST */
    check_mktime(1938, 8, 18, 17, 30, 0, -1, -989998200LL, "1938-08-18 17:30:00 0 3600 AAA");
    check_mktime(1938, 8, 18, 17, 30, 0, 0, -989998200LL, "1938-08-18 17:30:00 0 3600 AAA");
    check_mktime(1938, 8, 18, 17, 30, 0, 1, -990001800LL, "1938-08-18 17:30:00 1 7200 AAD");

    /* Version 2: 64-bit times, and the footer after the last transition */
    USE(v2);
    check_names("XST", "XDT", 18000, 1);
    check_local(-4000000000LL, "1843-03-31 11:57:18 0 -17762 LMT");
    check_local(-3000000001LL, "1874-12-07 13:43:57 0 -17762 LMT");
    check_local(-3000000000LL, "1874-12-07 13:40:00 0 -18000 XST");
    check_local(-2000000000LL, "1906-08-16 15:26:40 0 -18000 XST");
    check_local(-995000000LL, "1938-06-21 15:06:40 1 -14400 XDT");
    check_local(0LL, "1969-12-31 19:00:00 0 -18000 XST");
    check_local(2505000000LL, "2049-05-18 21:20:00 1 -14400 XDT");
    check_local(2600000000LL, "2052-05-22 09:13:20 0 -18000 XST");
    check_local(4102444799LL, "2099-12-31 18:59:59 0 -18000 XST");
    check_local(4102444800LL, "2099-12-31 19:00:00 0 -18000 XST");
    check_local(4119000000LL, "2100-07-11 10:40:00 1 -14400 XDT");
    check_local(4120000000LL, "2100-07-23 00:26:40 1 -14400 XDT");
    check_mktime(1843, 3, 24, 0, 0, 0, -1, -4000647838LL, "1843-03-24 00:00:00 0 -17762 LMT");
    check_mktime(2049, 5, 1, 12, 0, 0, -1, 2503497600LL, "2049-05-01 12:00:00 1 -14400 XDT");
    check_mktime(2100, 7, 20, 12, 0, 0, -1, 4119782400LL, "2100-07-20 12:00:00 1 -14400 XDT");
    /* In the footer's years, a gap, a repeated hour and tm_isdst that doesn't fit */
    check_mktime(2100, 3, 14, 2, 30, 0, -1, 4108692600LL, "2100-03-14 03:30:00 1 -14400 XDT");
    check_mktime(2100, 3, 14, 2, 30, 0, 1, 4108689000LL, "2100-03-14 01:30:00 0 -18000 XST");
#ifndef TEST_GLIBC /* glibc picks DST here, standard time in v1 */
    check_mktime(2100, 11, 7, 1, 30, 0, -1, 4129252200LL, "2100-11-07 01:30:00 0 -18000 XST");
#endif
    check_mktime(2100, 11, 7, 1, 30, 0, 1, 4129248600LL, "2100-11-07 01:30:00 1 -14400 XDT");
    check_mktime(2100, 11, 7, 1, 30, 0, 0, 4129252200LL, "2100-11-07 01:30:00 0 -18000 XST");
    check_mktime(2100, 1, 15, 12, 0, 0, 1, 4103712000LL, "2100-01-15 11:00:00 0 -18000 XST");

    /* strftime takes the offset and name from tm */
    t = (time_t)-4000000000LL;
    check(localtime_r(&t, &tm) == &tm, "localtime_r LMT");
    check(strftime(buf, sizeof(buf), "%Z %z", &tm) > 0, "strftime");
    check(strcmp(buf, "LMT -0456") == 0, "strftime %Z %z from the file");
    t = (time_t)2505000000LL;
    check(localtime_r(&t, &tm) == &tm, "localtime_r XDT");
    check(strftime(buf, sizeof(buf), "%Z %z %s", &tm) > 0, "strftime");
    check(strcmp(buf, "XDT -0400 2505000000") == 0, "strftime %Z %z %s in DST");

    /* Version 3: rule times before midnight */
    USE(v3);
    check_names("-02", "-02", 7200, 0);
    check_local(-1686083585LL, "1916-07-27 23:59:59 0 -12416 LMT");
    check_local(-1686083584LL, "1916-07-28 00:26:56 0 -10800 -03");
    check_local(1698541199LL, "2023-10-28 21:59:59 0 -10800 -03");
    check_local(1698541200LL, "2023-10-28 23:00:00 0 -7200 -02");
    check_local(1711846799LL, "2024-03-30 22:59:59 0 -7200 -02");
    check_local(1711846800LL, "2024-03-31 00:00:00 1 -3600 -01");
    check_local(1719835200LL, "2024-07-01 11:00:00 1 -3600 -01");
    check_local(1729990799LL, "2024-10-26 23:59:59 1 -3600 -01");
    check_local(1729990800LL, "2024-10-26 23:00:00 0 -7200 -02");
    check_mktime(2024, 3, 30, 23, 30, 0, -1, 1711848600LL, "2024-03-31 00:30:00 1 -3600 -01");
    check_mktime(2024, 10, 26, 23, 30, 0, -1, 1729992600LL, "2024-10-26 23:30:00 0 -7200 -02");
    check_mktime(2024, 10, 26, 23, 30, 0, 1, 1729989000LL, "2024-10-26 23:30:00 1 -3600 -01");
    check_mktime(2024, 10, 26, 23, 30, 0, 0, 1729992600LL, "2024-10-26 23:30:00 0 -7200 -02");

    /* Version 4: leap seconds ignored, a footer east of UTC without DST */
    USE(v4);
    check_names("+0545", "+0545", -20700, 0);
    check_local(-1577923201LL, "1920-01-01 05:41:15 0 20476 LMT");
    check_local(-1577923200LL, "1920-01-01 05:30:00 0 19800 +0530");
    check_local(504901799LL, "1985-12-31 23:59:59 0 19800 +0530");
    check_local(504901800LL, "1986-01-01 00:15:00 0 20700 +0545");
    check_local(1800000000LL, "2027-01-15 13:45:00 0 20700 +0545");
#ifndef TEST_GLIBC /* glibc applies the leap seconds */
    check_local(2000000000LL, "2033-05-18 09:18:20 0 20700 +0545");
#endif
    check_mktime(1985, 12, 31, 23, 55, 0, -1, 504901500LL, "1985-12-31 23:55:00 0 19800 +0530");
    check_mktime(1986, 1, 1, 0, 10, 0, -1, 504902400LL, "1986-01-01 00:25:00 0 20700 +0545");
#ifndef TEST_GLIBC /* glibc makes up a DST offset an hour ahead */
    /* Without DST, tm_isdst > 0 changes nothing, as with rule strings */
    check_mktime(2020, 1, 1, 0, 0, 0, 1, 1577816100LL, "2020-01-01 00:00:00 0 20700 +0545");
#endif

    /* West of UTC, with one period of DST and a footer without it */
    USE(west);
    check_names("-03", "-02", 10800, 1);
    check_local(-1767214413LL, "1913-12-31 23:59:59 0 -11188 LMT");
    check_local(-1767214412LL, "1914-01-01 00:06:28 0 -10800 -03");
    check_local(1541300399LL, "2018-11-03 23:59:59 0 -10800 -03");
    check_local(1541300400LL, "2018-11-04 01:00:00 1 -7200 -02");
    check_local(1550368799LL, "2019-02-16 23:59:59 1 -7200 -02");
    check_local(1550368800LL, "2019-02-16 23:00:00 0 -10800 -03");
    check_local(1750000000LL, "2025-06-15 12:06:40 0 -10800 -03");
    check_local(3000000000LL, "2065-01-24 02:20:00 0 -10800 -03");
    check_mktime(2018, 11, 4, 0, 30, 0, -1, 1541302200LL, "2018-11-04 01:30:00 1 -7200 -02");
    check_mktime(2018, 11, 3, 23, 30, 0, -1, 1541298600LL, "2018-11-03 23:30:00 0 -10800 -03");
    check_mktime(2019, 2, 16, 23, 30, 0, -1, 1550370600LL, "2019-02-16 23:30:00 0 -10800 -03");
    check_mktime(2019, 2, 16, 23, 30, 0, 1, 1550367000LL, "2019-02-16 23:30:00 1 -7200 -02");
    check_mktime(2019, 2, 16, 23, 30, 0, 0, 1550370600LL, "2019-02-16 23:30:00 0 -10800 -03");
    check_mktime(2030, 6, 1, 0, 0, 0, -1, 1906513200LL, "2030-06-01 00:00:00 0 -10800 -03");

    /* tm_zone stays valid after TZ changes to another zone */
    {
        struct tm lmt, xdt;

        USE(v2);
        t = (time_t)-4000000000LL;
        check(localtime_r(&t, &lmt) == &lmt, "localtime_r LMT");
        t = (time_t)4120000000LL;
        check(localtime_r(&t, &xdt) == &xdt, "localtime_r XDT from the footer");
        USE(west);
        set_tz("EST5");
        check(strcmp(lmt.tm_zone, "LMT") == 0, "tm_zone from a file after TZ changes");
        check(strcmp(xdt.tm_zone, "XDT") == 0, "tm_zone from a footer after TZ changes");
    }

    /* A rule string after a file is used as before */
    set_tz("EST5EDT,M3.2.0,M11.1.0");
    check_names("EST", "EDT", 18000, 1);
    check_local(1751371200LL, "2025-07-01 08:00:00 1 -14400 EDT");

    /* TZ without ':': an absolute path, and a name under TZDIR */
    path = write_file("v2", tzif_v2, sizeof(tzif_v2));
    set_tz(path);
    check_names("XST", "XDT", 18000, 1);
    check(setenv("TZDIR", dir, 1) == 0, "setenv TZDIR");
    set_tz("v1");
    check_names("AAB", "AAD", -5400, 1);
    set_tz(":v2");
    check_names("XST", "XDT", 18000, 1);
#ifdef __PICOLIBC__ /* glibc allows ".." except in set-user-ID programs */
    snprintf(tz, sizeof(tz), "../%s/v1", strrchr(dir, '/') + 1);
    set_tz(tz);
    check_fallback("a name with ..");
#endif
    set_tz("no-such-zone");
    check_fallback("a missing file");
    check(unsetenv("TZDIR") == 0, "unsetenv TZDIR");

#ifdef __PICOLIBC__
    /* With TZ unchanged, the file isn't read again */
    set_tz(path);
    write_file("v2", tzif_v1, sizeof(tzif_v1));
    tzset();
    check_names("XST", "XDT", 18000, 1);
#endif

    /* Malformed files are rejected, leaving what TZ would give without TZif */
    use_file("empty", tzif_v2, 0);
    check_fallback("an empty file");
    check_bad("bad magic", 0, "TZiF", 4, sizeof(bad), 1);
    check_bad("bad version", 4, "5", 1, sizeof(bad), 0);
    check_bad("truncated", 0, "", 0, 120, 1);
    check_bad("bad type index", 146, "\3", 1, sizeof(bad), 1);
    check_bad("bad abbreviation index", 157, "\14", 1, sizeof(bad), 0);
    check_bad("too many transitions", 86, "\0\0\x01\0", 4, sizeof(bad), 1);
    check_bad("transitions past the end", 86, "\0\0\0\x20", 4, sizeof(bad), 1);
    check_bad("no time types", 90, "\0\0\0\0", 4, sizeof(bad), 1);
    check_bad("unsorted transitions", 106, tzif_v2 + 98, 8, sizeof(bad), 0);
    check_bad("offset of a day", 158, "\0\1\x51\x80", 4, sizeof(bad), 0);
    check_bad("no footer", 0, "", 0, sizeof(bad) - 24, 1);
    check_bad("unterminated footer", 0, "", 0, sizeof(bad) - 1, 0);

    write_file("v2", tzif_v2, sizeof(tzif_v2));
    snprintf(tz, sizeof(tz), "%s/bad", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/empty", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/v1", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/v2", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/v3", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/v4", dir);
    unlink(tz);
    snprintf(tz, sizeof(tz), "%s/west", dir);
    unlink(tz);
    rmdir(dir);
    printf("tzif tests passed\n");
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
