/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2025 Keith Packard
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

#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "local.h"
#include "locale_private.h"

/* The  DATEMSK environment variable is not defined, or its value is an empty string. */
#define GETDATE_DATEMSK 1

/* The template file specified by DATEMSK cannot be opened for reading. */
#define GETDATE_NOACCESS 2

/* Failed to get file status information. */
#define GETDATE_NOSTATUS 3

/* The template file is not a regular file. */
#define GETDATE_NOTREGULAR 4

/* An error was encountered while reading the template file. */
#define GETDATE_READERROR 5

/* Memory allocation failed (not enough memory available). */
#define GETDATE_NOMEMORY 6

/* There is no line in the file that matches the input. */
#define GETDATE_NOMATCH 7

/* Invalid input specification. */
#define GETDATE_INVALID_INPUT 8

/* Is the rest of string only white space? */
static bool
only_space(const char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    return *s == '\0';
}

#define DATE_FIELDS                                                                          \
    (__STRPTIME_MDAY | __STRPTIME_MON | __STRPTIME_YEAR | __STRPTIME_WDAY | __STRPTIME_YDAY)
#define TIME_FIELDS (__STRPTIME_HOUR | __STRPTIME_MIN | __STRPTIME_SEC)

/* Fill in what the template didn't give, as POSIX describes, and check the result */
static int
complete(struct tm *tm, int fields)
{
    struct tm now;
    time_t    t = time(NULL);
    int       wday;

    if (t == (time_t)-1 || !localtime_r(&t, &now))
        return GETDATE_INVALID_INPUT;

    /* Without a time, the current one */
    if (!(fields & TIME_FIELDS)) {
        tm->tm_hour = now.tm_hour;
        tm->tm_min = now.tm_min;
        tm->tm_sec = now.tm_sec;
    }

    if (!(fields & DATE_FIELDS)) {
        /* No date: the first matching hour, starting with this one */
        tm->tm_year = now.tm_year;
        tm->tm_mon = now.tm_mon;
        tm->tm_mday = now.tm_mday;
        if (tm->tm_hour < now.tm_hour)
            tm->tm_mday++;
    } else if ((fields & DATE_FIELDS) == __STRPTIME_WDAY) {
        /* Only a weekday: the first matching day, starting with today */
        wday = tm->tm_wday;
        tm->tm_year = now.tm_year;
        tm->tm_mon = now.tm_mon;
        tm->tm_mday = now.tm_mday + (wday - now.tm_wday + 7) % 7;
    } else if (!(fields & __STRPTIME_YEAR)) {
        /* No year: this year, or next if the month has passed */
        tm->tm_year = now.tm_year;
        if (fields & __STRPTIME_MON) {
            if (tm->tm_mon < now.tm_mon)
                tm->tm_year++;
            if (!(fields & __STRPTIME_MDAY))
                tm->tm_mday = 1;
        } else {
            tm->tm_mon = now.tm_mon;
            if (!(fields & __STRPTIME_MDAY))
                tm->tm_mday = now.tm_mday;
        }
    } else {
        if (!(fields & __STRPTIME_MON))
            tm->tm_mon = 0;
        if (!(fields & __STRPTIME_MDAY))
            tm->tm_mday = 1;
    }

    /* A day the month doesn't have is invalid (February 31) */
    if (fields & __STRPTIME_MDAY) {
        int year = tm->tm_year + 1900;
        int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        if (tm->tm_mday > __month_lengths[leap][tm->tm_mon])
            return GETDATE_INVALID_INPUT;
    }

    if (!(fields & __STRPTIME_ISDST))
        tm->tm_isdst = -1;
    /* mktime leaves tm_wday alone when the time can't be represented */
    tm->tm_wday = -1;
    if (mktime(tm) == (time_t)-1 && tm->tm_wday == -1)
        return GETDATE_INVALID_INPUT;
    return 0;
}

int
getdate_r(const char * restrict string, struct tm * restrict tm)
{
    char       *line = NULL;
    size_t      size = 0;
    const char *datemsk_name;
    FILE       *datemsk;
    int         ret = GETDATE_NOMATCH;
    int         fields;
    int         save_errno = errno;

    datemsk_name = getenv("DATEMSK");
    if (!datemsk_name || datemsk_name[0] == '\0')
        return GETDATE_DATEMSK;
    datemsk = fopen(datemsk_name, "r");
    if (!datemsk) {
        if (errno == ENOMEM)
            return GETDATE_NOMEMORY;
        return GETDATE_NOACCESS;
    }

    errno = 0;
    while (getline(&line, &size, datemsk) >= 0) {
        char *end;

        /* The first template matching all of the input is used */
        end = __strptime_fields(string, line, tm, __get_current_locale(), &fields);
        if (end && only_space(end)) {
            ret = complete(tm, fields);
            break;
        }
        errno = 0;
    }
    if (ret == GETDATE_NOMATCH && ferror(datemsk)) {
        /* Reading a directory fails; picolibc has no portable fstat */
        if (errno == EISDIR)
            ret = GETDATE_NOTREGULAR;
        else if (errno == ENOMEM)
            ret = GETDATE_NOMEMORY;
        else
            ret = GETDATE_READERROR;
    }
    free(line);
    fclose(datemsk);
    errno = save_errno;
    return ret;
}
