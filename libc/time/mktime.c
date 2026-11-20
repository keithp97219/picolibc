/*
Copyright (c) 1994 Cygnus Support.
All rights reserved.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
and/or other materials related to such
distribution and use acknowledge that the software was developed
at Cygnus Support, Inc.  Cygnus Support, Inc. may not be used to
endorse or promote products derived from this software without
specific prior written permission.
THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * mktime.c
 * Original Author:	G. Haley
 *
 * Converts the broken-down time, expressed as local time, in the structure
 * pointed to by tim_p into a calendar time value. The original values of the
 * tm_wday and tm_yday fields of the structure are ignored, and the original
 * values of the other fields have no restrictions. On successful completion
 * the fields of the structure are set to represent the specified calendar
 * time. Returns the specified calendar time. If the calendar time can not be
 * represented, returns the value (time_t) -1.
 *
 * Modifications:	Fixed tm_isdst usage - 27 August 2008 Craig Howland.
 * 			Added timegm - 15 May 2021 R. Diez.
 */

/*
FUNCTION
<<mktime>>, <<timegm>>---convert time to arithmetic representation

INDEX
        mktime
INDEX
        timegm

SYNOPSIS
        #include <time.h>
        time_t mktime(struct tm *<[timp]>);
        time_t timegm(struct tm *<[timp]>);

DESCRIPTION
<<mktime>> assumes the time at <[timp]> is a local time, and converts
its representation from the traditional representation defined by
<<struct tm>> into a representation suitable for arithmetic.

<<localtime>> is the inverse of <<mktime>>.

<<timegm>> is similar to <<mktime>>, but assumes that the time at
<[timp]> is Coordinated Universal Time (UTC).

<<timegm>> could be emulated by setting the TZ environment variable to UTC,
calling <<mktime>> and restoring the value of TZ. However, other concurrent
threads could be affected by the temporary change to TZ.

<<timegm>> is the inverse of <<gmtime>>.

<<timegm>> is available if _BSD_SOURCE || _SVID_SOURCE || _DEFAULT_SOURCE.

RETURNS
If the contents of the structure at <[timp]> do not form a valid
calendar time representation, the result is <<-1>>.  Otherwise, the
result is the time, converted to a <<time_t>> value.

PORTABILITY
ANSI C requires <<mktime>>.

<<timegm>> is a nonstandard GNU extension that is also present on the BSDs.

<<mktime>> and <<timegm>> require no supporting OS subroutines.
*/

#include "local.h"
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : __month_lengths[0][x])

static const int16_t _DAYS_BEFORE_MONTH[12]
    = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

#define _DAYS_IN_YEAR(year) (365 + isleap2(year, YEAR_BASE))

static void
set_tm_wday(time_t days, struct tm *tim_p)
{
    if ((tim_p->tm_wday = (days + 4) % 7) < 0)
        tim_p->tm_wday += 7;
}

static bool
validate_structure(struct tm *tim_p)
{
    div_t res;
    int   days_in_feb = 28;
    int   year_off = 0;

    /* calculate time & date to account for out of range values */
    if (tim_p->tm_sec < 0 || tim_p->tm_sec > 59) {
        res = div(tim_p->tm_sec, 60);
        tim_p->tm_min += res.quot;
        if ((tim_p->tm_sec = res.rem) < 0) {
            tim_p->tm_sec += 60;
            --tim_p->tm_min;
        }
    }

    if (tim_p->tm_min < 0 || tim_p->tm_min > 59) {
        res = div(tim_p->tm_min, 60);
        tim_p->tm_hour += res.quot;
        if ((tim_p->tm_min = res.rem) < 0) {
            tim_p->tm_min += 60;
            --tim_p->tm_hour;
        }
    }

    if (tim_p->tm_hour < 0 || tim_p->tm_hour > 23) {
        res = div(tim_p->tm_hour, 24);
        tim_p->tm_mday += res.quot;
        if ((tim_p->tm_hour = res.rem) < 0) {
            tim_p->tm_hour += 24;
            --tim_p->tm_mday;
        }
    }

    if (tim_p->tm_mon < 0 || tim_p->tm_mon > 11) {
        res = div(tim_p->tm_mon, 12);
        year_off += res.quot;
        if ((tim_p->tm_mon = res.rem) < 0) {
            tim_p->tm_mon += 12;
            year_off--;
        }
    }

    if (isleap2(tim_p->tm_year, year_off + YEAR_BASE))
        days_in_feb = 29;

    if (tim_p->tm_mday <= 0) {
        while (tim_p->tm_mday <= 0) {
            if (--tim_p->tm_mon == -1) {
                year_off--;
                tim_p->tm_mon = 11;
                days_in_feb = (isleap2(tim_p->tm_year, year_off + YEAR_BASE) ? 29 : 28);
            }
            tim_p->tm_mday += _DAYS_IN_MONTH(tim_p->tm_mon);
        }
    } else {
        while (tim_p->tm_mday > _DAYS_IN_MONTH(tim_p->tm_mon)) {
            tim_p->tm_mday -= _DAYS_IN_MONTH(tim_p->tm_mon);
            if (++tim_p->tm_mon == 12) {
                year_off++;
                tim_p->tm_mon = 0;
                days_in_feb = (isleap2(tim_p->tm_year, year_off + YEAR_BASE) ? 29 : 28);
            }
        }
    }

    if (__picolibc_add_overflow(tim_p->tm_year, year_off, &tim_p->tm_year))
        return false;

    return true;
}

static bool
mktime_utc(struct tm *tim_p, time_t *tim_ret, time_t *days_p)
{
    time_t tim = 0;
    time_t days = 0;
    int    year;
    time_t daysecs;

    /* validate structure */
    if (!validate_structure(tim_p))
        goto overflow;

    /* compute hours, minutes, seconds */
    tim += tim_p->tm_sec + (tim_p->tm_min * SECSPERMIN) + (tim_p->tm_hour * SECSPERHOUR);

    /* compute days in year */
    days += tim_p->tm_mday - 1;
    days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
    if (tim_p->tm_mon > 1 && isleap2(tim_p->tm_year, YEAR_BASE))
        days++;

    /* compute day of the year */
    tim_p->tm_yday = days;

    year = tim_p->tm_year;

#define DAYSPERHCEN 3652425

#define TIME_T_MAX  ((sizeof(time_t) < 8) ? INT32_MAX : INT64_MAX)
#define TIME_T_MIN  ((sizeof(time_t) < 8) ? INT32_MIN : INT64_MIN)

#define DAY_MAX     (TIME_T_MAX / SECSPERDAY)
#define DAY_MIN     (TIME_T_MIN / SECSPERDAY)
#define HCEN_MAX    (TIME_T_MAX / DAYSPERHCEN)
#define HCEN_MIN    (TIME_T_MIN / DAYSPERHCEN)

    if (sizeof(time_t) > 4) {
        long hundredcentury;

        /*
         * Split out the multiple of 10000 as those are an exact integer
         * number of days
         */
        if (year > 0)
            hundredcentury = year / 10000;
        else
            hundredcentury = (int)-(-(unsigned)year / 10000);

        year = year - hundredcentury * 10000;

#if __SIZEOF_INT__ > 4
        if (hundredcentury < HCEN_MIN || HCEN_MAX < hundredcentury)
            goto overflow;
#endif

        days += (time_t)hundredcentury * DAYSPERHCEN;
    }

    /* compute days in other years */
    if (year > 70) {
        long y;
        for (y = 70; y < year; y++)
            days += _DAYS_IN_YEAR(y);
    } else if (year < 70) {
        long y;
        for (y = 69; y > year; y--)
            days -= _DAYS_IN_YEAR(y);
        days -= _DAYS_IN_YEAR(y);
    }

    if (days < DAY_MIN || DAY_MAX < days)
        goto overflow;

    daysecs = days * SECSPERDAY;

    tim += daysecs;

    /* tim is >= 0 after validate_structure */
    if (daysecs > 0 && tim < 0)
        goto overflow;

    *days_p = days;
    *tim_ret = tim;
    return true;

overflow:
    errno = EOVERFLOW;
    return false;
}

/* Whether DST is in effect at t; call with the lock held */
static int
isdst_at(time_t t)
{
    struct tm utc;

    if (!gmtime_r(&t, &utc))
        return 0;
    return __tz_isdst(t, utc.tm_year + YEAR_BASE, utc.tm_yday);
}

time_t
mktime(struct tm *tim_p)
{
    time_t    days;
    time_t    tim;
    time_t    offset;
    struct tm res;
    int       isdst = 0;

    if (!mktime_utc(tim_p, &tim, &days))
        return (time_t)-1;

    tzset();

    __LIBC_LOCK();
    if (daylight) {
        if (tim_p->tm_isdst > 0)
            isdst = 1;
        else if (tim_p->tm_isdst < 0) {
            /*
             * Standard time, unless that is a DST time and the DST
             * reading is valid. In the gap at the start of DST,
             * neither is, and standard time is used.
             */
            time_t t;
            if (!__picolibc_add_overflow(tim, (time_t)__tzinfo.rule[0].offset, &t) && isdst_at(t)
                && !__picolibc_add_overflow(tim, (time_t)__tzinfo.rule[1].offset, &t)
                && isdst_at(t))
                isdst = 1;
        }
    }
    offset = (time_t)__tzinfo.rule[isdst].offset;
    __LIBC_UNLOCK();

    /* The members are set as localtime would set them */
    if (__picolibc_add_overflow(tim, offset, &tim) || !localtime_r(&tim, &res)) {
        errno = EOVERFLOW;
        return (time_t)-1;
    }
    *tim_p = res;
    return tim;
}

time_t
timegm(struct tm *tim_p)
{
    time_t days;
    time_t tim;

    if (!mktime_utc(tim_p, &tim, &days))
        return (time_t)-1;

    /* The time is always UTC, so there is no daylight saving time. */
    tim_p->tm_isdst = 0;

    set_tm_wday(days, tim_p);

    return tim;
}
