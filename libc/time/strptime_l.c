/*
 * Copyright (c) 1999 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of KTH nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include "locale_private.h"
#include "local.h"

static const int _DAYS_BEFORE_MONTH[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

#define SET_MDAY      (1 << 0)
#define SET_MON       (1 << 1)
#define SET_YEAR      (1 << 2)
#define SET_WDAY      (1 << 3)
#define SET_YDAY      (1 << 4)
#define SET_WEEK_SUN  (1 << 5)
#define SET_WEEK_MON  (1 << 6)
#define SET_WEEK_MON4 (1 << 7)
#define SET_AMPM      (1 << 8)
#define SET_HOUR      (1 << 9)
#define SET_MIN       (1 << 10)
#define SET_SEC       (1 << 11)
#define SET_ISDST     (1 << 12)
#define SET_YMD       (SET_YEAR | SET_MON | SET_MDAY)
#define SET_WEEK      (SET_WEEK_SUN | SET_WEEK_MON | SET_WEEK_MON4)

/*
 * tm_year is relative this year
 */

#define TM_YEAR_BASE 1900

/*
 * Return TRUE iff `year' was a leap year.
 * Needed for strptime.
 */
static int
is_leap_year(int year)
{
    return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

/* Needed for strptime. */
static int
match_string(const char * __restrict *buf, const char * const *strs, int nstrs, locale_t locale)
{
    int i = 0;

    for (i = 0; i < nstrs; i++) {
        size_t len = strlen(strs[i]);

        if (strncasecmp_l(*buf, strs[i], len, locale) == 0) {
            *buf += len;
            return i;
        }
    }
    return -1;
}

/* Needed for strptime. */
static int
first_day(int year)
{
    int ret = 4;

    while (--year >= 1970)
        ret = (ret + 365 + is_leap_year(year)) % 7;
    return ret;
}

/*
 * Set `timeptr' given `wnum' (week number [0, 53])
 * Needed for strptime
 */

static void
set_week_number_sun(struct tm *timeptr, int wnum)
{
    int fday = first_day(timeptr->tm_year + TM_YEAR_BASE);

    timeptr->tm_yday = wnum * 7 + timeptr->tm_wday - fday;
    if (timeptr->tm_yday < 0) {
        timeptr->tm_wday = fday;
        timeptr->tm_yday = 0;
    }
}

/*
 * Set `timeptr' given `wnum' (week number [0, 53])
 * Needed for strptime
 */

static void
set_week_number_mon(struct tm *timeptr, int wnum)
{
    int fday = (first_day(timeptr->tm_year + TM_YEAR_BASE) + 6) % 7;

    timeptr->tm_yday = wnum * 7 + (timeptr->tm_wday + 6) % 7 - fday;
    if (timeptr->tm_yday < 0) {
        timeptr->tm_wday = (fday + 1) % 7;
        timeptr->tm_yday = 0;
    }
}

/*
 * Set `timeptr' given `wnum' (week number [0, 53])
 * Needed for strptime
 */
static void
set_week_number_mon4(struct tm *timeptr, int wnum)
{
    int fday = first_day(timeptr->tm_year + TM_YEAR_BASE);

    if (fday >= 4)
        wnum++;

    timeptr->tm_yday = (wnum - 1) * 7 + timeptr->tm_wday - fday;
    if (timeptr->tm_yday < 0) {
        timeptr->tm_wday = fday;
        timeptr->tm_yday = 0;
    }
}

/* Parsing state shared by the recursive calls for %c, %D and so on */
struct strptime_state {
    int ymd;
    int wnum;
    int ampm;
    int century; /* from %C, or INT_MIN */
    int yy;      /* from %y, or -1 */
};

/*
 * Convert a decimal number of at most width bytes (the sign, when
 * allowed, counts) and check that it's within [min, max]. Leading
 * white space is skipped.
 */
static const char *
get_number(const char *buf, int width, bool sign, int min, int max, int *val, locale_t locale)
{
    const char *start;
    long long   v = 0;
    bool        neg = false;

    while (isspace_l((unsigned char)*buf, locale))
        ++buf;
    start = buf;
    if (sign && (*buf == '+' || *buf == '-')) {
        neg = *buf == '-';
        ++buf;
    }
    if (!isdigit_l((unsigned char)*buf, locale))
        return NULL;
    while (isdigit_l((unsigned char)*buf, locale) && buf - start < width) {
        v = v * 10 + (*buf++ - '0');
        if (v > (long long)INT_MAX + 1)
            return NULL;
    }
    if (neg)
        v = -v;
    if (v < min || v > max)
        return NULL;
    *val = (int)v;
    return buf;
}

/* Match either the full or the abbreviated name */
static int
match_name(const char * __restrict *buf, const char * const *full, const char * const *abbr,
           int nstrs, locale_t locale)
{
    int ret = match_string(buf, full, nstrs, locale);

    if (ret < 0)
        ret = match_string(buf, abbr, nstrs, locale);
    return ret;
}

#define NUMBER(w, sign, min, max)                                                 \
    do {                                                                          \
        buf = get_number(buf, width ? width : (w), sign, min, max, &ret, locale); \
        if (buf == NULL)                                                          \
            return NULL;                                                          \
    } while (0)

static char *
__strptime(const char *buf, const char *format, struct tm *timeptr, locale_t locale,
           struct strptime_state *st)
{
    char c;

    for (; (c = *format) != '\0'; ++format) {
        char *s;
        int   ret;
        int   width = 0;

        if (isspace_l((unsigned char)c, locale)) {
            while (isspace_l((unsigned char)*buf, locale))
                ++buf;
        } else if (c == '%' && format[1] != '\0') {
            c = *++format;
            /* The 0 and + flags are ignored */
            if (c == '0' || c == '+')
                c = *++format;
            /* A field width limits the bytes converted */
            while (c >= '0' && c <= '9') {
                if (width < 10000)
                    width = width * 10 + (c - '0');
                c = *++format;
            }
            if (c == 'E' || c == 'O')
                c = *++format;
            switch (c) {
            case 'A':
            case 'a':
                ret = match_name(&buf, TIME_WEEKDAY, TIME_WDAY, TIME_WDAY_NUM, locale);
                if (ret < 0)
                    return NULL;
                timeptr->tm_wday = ret;
                st->ymd |= SET_WDAY;
                break;
            case 'B':
            case 'b':
            case 'h':
                ret = match_name(&buf, TIME_MONTH, TIME_MON, TIME_MON_NUM, locale);
                if (ret < 0)
                    return NULL;
                timeptr->tm_mon = ret;
                st->ymd |= SET_MON;
                break;
            case 'C':
                NUMBER(2, true, INT_MIN / 100, INT_MAX / 100);
                st->century = ret;
                st->ymd |= SET_YEAR;
                break;
            case 'c': /* %a %b %e %H:%M:%S %Y */
                s = __strptime(buf, TIME_C_FMT, timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'D': /* %m/%d/%y */
                s = __strptime(buf, "%m/%d/%y", timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'd':
            case 'e':
                NUMBER(2, false, 1, 31);
                timeptr->tm_mday = ret;
                st->ymd |= SET_MDAY;
                break;
            case 'F': {
                /* %Y-%m-%d, with any year width, all within the field width */
                const char *end = width ? buf + width : NULL;
                int         left;

#define LEFT(w) (end ? ((left = (int)(end - buf)) < (w) ? left : (w)) : (w))
                buf = get_number(buf, LEFT(INT_MAX), true, INT_MIN + TM_YEAR_BASE, INT_MAX, &ret,
                                 locale);
                if (buf == NULL || (end && buf >= end) || *buf != '-')
                    return NULL;
                timeptr->tm_year = ret - TM_YEAR_BASE;
                ++buf;
                buf = get_number(buf, LEFT(2), false, 1, 12, &ret, locale);
                if (buf == NULL || (end && buf >= end) || *buf != '-')
                    return NULL;
                timeptr->tm_mon = ret - 1;
                ++buf;
                buf = get_number(buf, LEFT(2), false, 1, 31, &ret, locale);
                if (buf == NULL)
                    return NULL;
                timeptr->tm_mday = ret;
#undef LEFT
                st->century = INT_MIN;
                st->yy = -1;
                st->ymd |= SET_YMD;
                break;
            }
            case 'G': /* The week-based year; its effect is unspecified */
                NUMBER(4, true, INT_MIN, INT_MAX);
                break;
            case 'g':
                NUMBER(2, true, -99, 99);
                break;
            case 'H':
            case 'k': /* hour with leading space - GNU extension */
                NUMBER(2, false, 0, 23);
                timeptr->tm_hour = ret;
                st->ymd |= SET_HOUR;
                break;
            case 'I':
            case 'l': /* hour with leading space - GNU extension */
                NUMBER(2, false, 1, 12);
                if (ret == 12)
                    ret = 0;
                timeptr->tm_hour = ret;
                st->ymd |= SET_HOUR;
                break;
            case 'j':
                NUMBER(3, false, 1, 366);
                timeptr->tm_yday = ret - 1;
                st->ymd |= SET_YDAY;
                break;
            case 'm':
                NUMBER(2, false, 1, 12);
                timeptr->tm_mon = ret - 1;
                st->ymd |= SET_MON;
                break;
            case 'M':
                NUMBER(2, false, 0, 59);
                timeptr->tm_min = ret;
                st->ymd |= SET_MIN;
                break;
            case 'p':
                st->ampm = match_string(&buf, TIME_AM_PM, TIME_AM_PM_NUM, locale);
                if (st->ampm < 0)
                    return NULL;
                st->ymd |= SET_AMPM;
                break;
            case 'r': /* %I:%M:%S %p */
                s = __strptime(buf, TIME_AMPM_FMT, timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'R': /* %H:%M */
                s = __strptime(buf, "%H:%M", timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 's': /* seconds since Unix epoch */
            {
                long long sec;
                time_t    t;
                int       save_errno;
                save_errno = errno;
                errno = 0;
                sec = strtoll_l(buf, &s, 10, locale);
                t = sec;
                if (s == buf || errno != 0 || t != sec || localtime_r(&t, timeptr) != timeptr)
                    return NULL;
                errno = save_errno;
                buf = s;
                st->century = INT_MIN;
                st->yy = -1;
                st->ymd |= SET_YDAY | SET_WDAY | SET_YMD | SET_HOUR | SET_MIN | SET_SEC | SET_ISDST;
                break;
            }
            case 'S':
                NUMBER(2, false, 0, 60);
                timeptr->tm_sec = ret;
                st->ymd |= SET_SEC;
                break;
            case 'n':
            case 't':
                while (isspace_l((unsigned char)*buf, locale))
                    ++buf;
                break;
            case 'T': /* %H:%M:%S */
                s = __strptime(buf, "%H:%M:%S", timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'u':
                NUMBER(1, false, 1, 7);
                timeptr->tm_wday = ret % 7;
                st->ymd |= SET_WDAY;
                break;
            case 'w':
                NUMBER(1, false, 0, 6);
                timeptr->tm_wday = ret;
                st->ymd |= SET_WDAY;
                break;
            case 'U':
                NUMBER(2, false, 0, 53);
                st->wnum = ret;
                st->ymd |= SET_WEEK_SUN;
                break;
            case 'V':
                NUMBER(2, false, 1, 53);
                st->wnum = ret;
                st->ymd |= SET_WEEK_MON4;
                break;
            case 'W':
                NUMBER(2, false, 0, 53);
                st->wnum = ret;
                st->ymd |= SET_WEEK_MON;
                break;
            case 'x':
                s = __strptime(buf, TIME_X_FMT, timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'X':
                s = __strptime(buf, TIME_UX_FMT, timeptr, locale, st);
                if (s == NULL)
                    return NULL;
                buf = s;
                break;
            case 'y':
                NUMBER(2, true, -99, 99);
                st->yy = ret < 0 ? -ret : ret;
                st->ymd |= SET_YEAR;
                break;
            case 'Y':
                NUMBER(4, true, INT_MIN + TM_YEAR_BASE, INT_MAX);
                timeptr->tm_year = ret - TM_YEAR_BASE;
                st->century = INT_MIN;
                st->yy = -1;
                st->ymd |= SET_YEAR;
                break;
            case 'z': {
                /* +hh, +hhmm or +hh:mm */
                int sign, hh, mm = 0;

                while (isspace_l((unsigned char)*buf, locale))
                    ++buf;
                if (*buf != '+' && *buf != '-')
                    return NULL;
                sign = *buf++ == '-' ? -1 : 1;
                if (!isdigit_l((unsigned char)buf[0], locale)
                    || !isdigit_l((unsigned char)buf[1], locale))
                    return NULL;
                hh = (buf[0] - '0') * 10 + (buf[1] - '0');
                buf += 2;
                if (*buf == ':' && isdigit_l((unsigned char)buf[1], locale))
                    ++buf;
                if (isdigit_l((unsigned char)buf[0], locale)
                    && isdigit_l((unsigned char)buf[1], locale)) {
                    mm = (buf[0] - '0') * 10 + (buf[1] - '0');
                    buf += 2;
                }
                if (hh > 24 || mm > 59)
                    return NULL;
                timeptr->tm_gmtoff = sign * (hh * 3600L + mm * 60L);
                break;
            }
            case 'Z': {
                /* A timezone name: letters, or the characters of a quoted name */
                const char *name;
                size_t      len;

                while (isspace_l((unsigned char)*buf, locale))
                    ++buf;
                name = buf;
                if (isalpha_l((unsigned char)*buf, locale)) {
                    while (isalpha_l((unsigned char)*buf, locale))
                        ++buf;
                } else {
                    while (isalnum_l((unsigned char)*buf, locale) || *buf == '+' || *buf == '-')
                        ++buf;
                }
                len = (size_t)(buf - name);
                if (len == 0)
                    return NULL;
                tzset();
                if (strlen(tzname[1]) == len && strncmp(name, tzname[1], len) == 0
                    && strcmp(tzname[0], tzname[1]) != 0) {
                    timeptr->tm_isdst = 1;
                    st->ymd |= SET_ISDST;
                } else if (strlen(tzname[0]) == len && strncmp(name, tzname[0], len) == 0) {
                    timeptr->tm_isdst = 0;
                    st->ymd |= SET_ISDST;
                }
                break;
            }
            case '\0':
                --format;
                __fallthrough;
            case '%':
                if (*buf == '%')
                    ++buf;
                else
                    return NULL;
                break;
            default:
                return NULL;
            }
        } else {
            if (*buf == c)
                ++buf;
            else
                return NULL;
        }
    }
    return (char *)buf;
}

/* Combine the parsed fields; returns the set of fields given */
char *
__strptime_fields(const char *buf, const char *format, struct tm *timeptr, locale_t locale,
                  int *fields)
{
    struct strptime_state st = { 0, 0, 0, INT_MIN, -1 };
    int                   ymd;

    memset(timeptr, 0, sizeof(*timeptr));
    buf = __strptime(buf, format, timeptr, locale, &st);
    if (buf == NULL)
        return NULL;
    ymd = st.ymd;

    /* %C gives the century, %y the year within it */
    if (st.century != INT_MIN) {
        long long year = (long long)st.century * 100 + (st.yy >= 0 ? st.yy : 0) - TM_YEAR_BASE;
        if (st.century < 0)
            year = (long long)st.century * 100 - (st.yy >= 0 ? st.yy : 0) - TM_YEAR_BASE;
        if (year < INT_MIN || year > INT_MAX)
            return NULL;
        timeptr->tm_year = (int)year;
    } else if (st.yy >= 0) {
        /* [69,99] are 1969 to 1999, [00,68] are 2000 to 2068 */
        timeptr->tm_year = st.yy < 69 ? st.yy + 100 : st.yy;
    }

    if ((ymd & SET_WEEK) != 0 && (ymd & (SET_YEAR | SET_WDAY)) == (SET_YEAR | SET_WDAY)) {
        /* tm_year and week number, compute tm_yday */
        if (ymd & SET_WEEK_SUN)
            set_week_number_sun(timeptr, st.wnum);
        else if (ymd & SET_WEEK_MON4)
            set_week_number_mon4(timeptr, st.wnum);
        else if (ymd & SET_WEEK_MON)
            set_week_number_mon(timeptr, st.wnum);
        ymd |= SET_YDAY;
    }
    if (ymd & SET_AMPM) {
        if (timeptr->tm_hour > 12)
            return NULL;
        else if (timeptr->tm_hour == 12)
            timeptr->tm_hour = st.ampm * 12;
        else
            timeptr->tm_hour += st.ampm * 12;
    }

    if ((ymd & SET_YMD) == SET_YMD) {
        /* all of tm_year, tm_mon and tm_mday, but... */

        if (!(ymd & SET_YDAY)) {
            /* ...not tm_yday, so fill it in */
            timeptr->tm_yday = _DAYS_BEFORE_MONTH[timeptr->tm_mon] + timeptr->tm_mday;
            if (!is_leap_year(timeptr->tm_year + TM_YEAR_BASE) || timeptr->tm_mon < 2) {
                timeptr->tm_yday--;
            }
            ymd |= SET_YDAY;
        }
    } else if ((ymd & (SET_YEAR | SET_YDAY)) == (SET_YEAR | SET_YDAY)) {
        /* both of tm_year and tm_yday, but... */

        if (!(ymd & SET_MON)) {
            /* ...not tm_mon, so fill it in, and/or... */
            if (timeptr->tm_yday < _DAYS_BEFORE_MONTH[1])
                timeptr->tm_mon = 0;
            else {
                int leap = is_leap_year(timeptr->tm_year + TM_YEAR_BASE);
                int i;
                for (i = 2; i < 12; ++i) {
                    if (timeptr->tm_yday < _DAYS_BEFORE_MONTH[i] + leap)
                        break;
                }
                timeptr->tm_mon = i - 1;
            }
            ymd |= SET_MON;
        }

        if (!(ymd & SET_MDAY)) {
            /* ...not tm_mday, so fill it in */
            timeptr->tm_mday = timeptr->tm_yday - _DAYS_BEFORE_MONTH[timeptr->tm_mon];
            if (!is_leap_year(timeptr->tm_year + TM_YEAR_BASE) || timeptr->tm_mon < 2) {
                timeptr->tm_mday++;
            }
            ymd |= SET_MDAY;
        }
    }

    if ((ymd & (SET_YEAR | SET_YDAY | SET_WDAY)) == (SET_YEAR | SET_YDAY)) {
        /* fill in tm_wday */
        int fday = first_day(timeptr->tm_year + TM_YEAR_BASE);
        timeptr->tm_wday = (fday + timeptr->tm_yday) % 7;
    }

    if (fields)
        *fields = ymd;
    return (char *)buf;
}

char *
strptime_l(const char *buf, const char *format, struct tm *timeptr, locale_t locale)
{
    return __strptime_fields(buf, format, timeptr, locale, NULL);
}
