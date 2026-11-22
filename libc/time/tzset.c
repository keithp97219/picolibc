/*
FUNCTION
<<tzset>>---set timezone characteristics from <[TZ]> environment variable

INDEX
        tzset

SYNOPSIS
        #include <time.h>
        void tzset(void);

DESCRIPTION
<<tzset>> examines the <[TZ]> environment variable and sets up the three
external variables: <<_timezone>>, <<_daylight>>, and <<tzname>>.
The value of <<_timezone>> shall be the offset from the current time
to Universal Time.
The value of <<_daylight>> shall be 0 if there is no daylight savings
time for the current time zone, otherwise it will be non-zero.
The <<tzname>> array has two entries: the first is the designation of the
standard time period, the second is the designation of the alternate time
period.

The <[TZ]> environment variable is expected to be in the following POSIX
format (spaces inserted for clarity):

    <[std]> <[offset1]> [<[dst]> [<[offset2]>] [,<[start]> [/<[time1]>], <[end]> [/<[time2]>]]]

where:

<[std]> is the designation for the standard time period (minimum 3,
maximum <<TZNAME_MAX>> bytes) in one of two forms:

*- quoted within angle bracket '<' '>' characters: portable numeric
sign or alphanumeric characters in the current locale; the
quoting characters are not included in the designation

*- unquoted: portable alphabetic characters in the current locale

<[offset1]> is the value to add to local standard time to get Universal Time;
it has the format:

    [<[S]>]<[hh]>[:<[mm]>[:<[ss]>]]

    where:

    <[S]> is an optional numeric sign character; if negative '-', the
    time zone is East of the International Reference
    Meridian; else it is positive and West, and '+' may be used

    <[hh]> is the required numeric hour between 0 and 24

    <[mm]> is the optional numeric minute between 0 and 59

    <[ss]> is the optional numeric second between 0 and 59

<[dst]> is the designation of the alternate (daylight saving or
summer) time period, with the same limits and forms as
the standard time period designation

<[offset2]> is the value to add to local alternate time to get
Universal Time; it has the same format as <[offset1]>

<[start]> is the date in the year that alternate time starts;
the form may be one of:
(quotes "'" around characters below are used only to distinguish literals)

    <[n]>	zero based Julian day (0-365), counting February 29 Leap days

    'J'<[n]>	one based Julian day (1-365), not counting February 29 Leap
    days; in all years day 59 is February 28 and day 60 is March 1

    'M'<[m]>'.'<[w]>'.'<[d]>
    month <[m]> (1-12) week <[w]> (1-5) day <[d]> (0-6) where week 1 is
    the first week in month <[m]> with day <[d]>; week 5 is the last
    week in the month; day 0 is Sunday

<[time1]> is the optional local time that alternate time starts, in
the same format as <[offset1]> except that <[hh]> may be up to 167,
and defaults to 02:00:00

<[end]> is the date in the year that alternate time ends, in the same
forms as <[start]>

<[time2]> is the optional local time that alternate time ends, in
the same format as <[time1]>

Note that there is no white-space padding between fields. Also note that
if <[TZ]> is null, the default is Universal Time which has no daylight saving
time. If <[TZ]> is empty, the default EST5EDT is used.

When picolibc is built with the <<tzif>> option, <<tzset>> also reads
time zone files in the TZif format (RFC 8536 and RFC 9636). A <[TZ]>
of the form :<[name]>, or one that is not a rule string as above, names
the file <[name]> under the directory in the <[TZDIR]> environment
variable, by default /usr/share/zoneinfo; names holding a ".."
component are refused, and a name starting with '/' is used as is.
When <[TZ]> is null, the file /etc/localtime is read. When the file
cannot be read or is malformed, the time zone is what <[TZ]> gives
without TZif files.

RETURNS
There is no return value.

PORTABILITY
<<tzset>> is part of the POSIX standard.

Supporting OS subroutines required: None; with the <<tzif>> option,
<<open>>, <<read>> and <<close>>.
*/

#include "local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <stdbool.h>

#define TZNAME_MIN 3 /* POSIX min TZ abbr size local def */

/*
 * Parse the POSIX TZ string tzenv into names and rules. Without a
 * DST part, the second name and rule copy the first. When tzenv is
 * NULL or not a valid rule string, sets them to GMT and returns false.
 */
#ifndef __TZIF
static bool __tzparse(const char *tzenv, char new_tzname[2][TZNAME_MAX + 2],
                      tzrule_t new_tzrule[2]);
#endif

bool
__tzparse(const char *tzenv, char new_tzname[2][TZNAME_MAX + 2], tzrule_t new_tzrule[2])
{
    unsigned short hh, mm, ss, m, w, d;
    int            sign, n;
    int            i, ch;
    bool           dst_valid = false;
    bool           ok = true;

    if (tzenv == NULL)
        goto bail;

    /* ignore implementation-specific format specifier */
    if (*tzenv == ':')
        ++tzenv;

    /* allow POSIX angle bracket < > quoted signed alphanumeric tz abbr e.g. <MESZ+0330> */
    if (*tzenv == '<') {
        ++tzenv;

        /* quit if no items, too few or too many chars, or no close quote '>' */
        if (sscanf(tzenv, "%11[-+0-9A-Za-z]%n", new_tzname[0], &n) <= 0 || n < TZNAME_MIN
            || TZNAME_MAX < n || '>' != tzenv[n])
            goto bail;

        ++tzenv; /* bump for close quote '>' */
    } else {
        /* allow POSIX unquoted alphabetic tz abbr e.g. MESZ */
        if (sscanf(tzenv, "%11[A-Za-z]%n", new_tzname[0], &n) <= 0 || n < TZNAME_MIN
            || TZNAME_MAX < n)
            goto bail;
    }

    tzenv += n;

    sign = 1;
    if (*tzenv == '-') {
        sign = -1;
        ++tzenv;
    } else if (*tzenv == '+')
        ++tzenv;

    mm = 0;
    ss = 0;

    if (sscanf(tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) < 1)
        goto bail;

    new_tzrule[0].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

    tzenv += n;

    /* Set up dst values */

    /* allow POSIX angle bracket < > quoted signed alphanumeric tz abbr e.g. <MESZ+0330> */
    if (*tzenv == '<') {
        ++tzenv;

        /* quit if no items, too few or too many chars, or no close quote '>' */
        if (sscanf(tzenv, "%11[-+0-9A-Za-z]%n", new_tzname[1], &n) <= 0
            && tzenv[0] == '>') { /* No dst */
            goto done;
        } else if (n < TZNAME_MIN || TZNAME_MAX < n || '>' != tzenv[n]) { /* error */
            goto bail;
        }

        ++tzenv; /* bump for close quote '>' */
    } else {
        /* allow POSIX unquoted alphabetic tz abbr e.g. MESZ */
        if (sscanf(tzenv, "%11[A-Za-z]%n", new_tzname[1], &n) <= 0) { /* No dst */
            goto done;
        } else if (n < TZNAME_MIN || TZNAME_MAX < n) { /* error */
            goto bail;
        }
    }

    tzenv += n;

    /* otherwise we have a dst name, look for the offset */
    sign = 1;
    if (*tzenv == '-') {
        sign = -1;
        ++tzenv;
    } else if (*tzenv == '+')
        ++tzenv;

    hh = 0;
    mm = 0;
    ss = 0;

    n = 0;
    if (sscanf(tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) <= 0)
        new_tzrule[1].offset = new_tzrule[0].offset - 3600;
    else
        new_tzrule[1].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

    dst_valid = true;

    tzenv += n;

    for (i = 0; i < 2; ++i) {
        if (*tzenv == ',')
            ++tzenv;

        if (*tzenv == 'M') {
            if (sscanf(tzenv, "M%hu%n.%hu%n.%hu%n", &m, &n, &w, &n, &d, &n) != 3 || m < 1 || m > 12
                || w < 1 || w > 5 || d > 6) {
                /* error in time format */
                goto bail;
            }

            new_tzrule[i].ch = 'M';
            new_tzrule[i].m = m;
            new_tzrule[i].n = w;
            new_tzrule[i].d = d;

            tzenv += n;
        } else {
            char *end;
            if (*tzenv == 'J') {
                ch = 'J';
                ++tzenv;
            } else
                ch = 'D';

            d = strtoul(tzenv, &end, 10);

            /* if unspecified, default to US settings */
            /* From 1987-2006, US was M4.1.0,M10.5.0, but starting in 2007 is
             * M3.2.0,M11.1.0 (2nd Sunday March through 1st Sunday November)  */
            if (end == tzenv) {
                if (i == 0) {
                    new_tzrule[0].ch = 'M';
                    new_tzrule[0].m = 3;
                    new_tzrule[0].n = 2;
                    new_tzrule[0].d = 0;
                } else {
                    new_tzrule[1].ch = 'M';
                    new_tzrule[1].m = 11;
                    new_tzrule[1].n = 1;
                    new_tzrule[1].d = 0;
                }
            } else {
                new_tzrule[i].ch = ch;
                new_tzrule[i].d = d;
            }

            tzenv = end;
        }

        /* default time is 02:00:00 am */
        hh = 2;
        mm = 0;
        ss = 0;
        n = 0;
        sign = 1;

        if (*tzenv == '/') {
            /* POSIX.1-2024 allows a signed time, e.g. "M3.5.0/-1" */
            ++tzenv;
            if (*tzenv == '-') {
                sign = -1;
                ++tzenv;
            } else if (*tzenv == '+')
                ++tzenv;

            if (sscanf(tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) <= 0) {
                /* error in time format */
                goto bail;
            }
        }

        new_tzrule[i].s = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

        tzenv += n;
    }

done:
    if (!dst_valid) {
        if (false) {
        bail:
            ok = false;
            strcpy(new_tzname[0], "GMT");
            /* UTC; __tzcalc_limits sets change */
            new_tzrule[0].ch = 'J';
            new_tzrule[0].m = 0;
            new_tzrule[0].n = 0;
            new_tzrule[0].d = 0;
            new_tzrule[0].s = 0;
            new_tzrule[0].offset = 0;
        }
        strcpy(new_tzname[1], new_tzname[0]);
        new_tzrule[1] = new_tzrule[0];
    }
    return ok;
}

/*
 * Make names and rules the current time zone. With __TZIF, zone is
 * the TZif zone they came from, or NULL, and replaces the current one.
 */
#ifdef __TZIF
void
__tzset_install(char new_tzname[2][TZNAME_MAX + 2], tzrule_t new_tzrule[2], struct __tzif *zone)
#else
static void
__tzset_install(char new_tzname[2][TZNAME_MAX + 2], tzrule_t new_tzrule[2])
#endif
{
#ifdef __TZIF
    struct __tzif *old;
#endif

    __LIBC_LOCK();
    {
        /* Copy in the new values */
        strcpy(__tzname_std, new_tzname[0]);
        strcpy(__tzname_dst, new_tzname[1]);
        __tzinfo.rule[0] = new_tzrule[0];
        __tzinfo.rule[1] = new_tzrule[1];

        /* Compute the derived values */
        __tzcalc_limits(__tzinfo.year);
        timezone = new_tzrule[0].offset;
        daylight = new_tzrule[0].offset != new_tzrule[1].offset;
#ifdef __TZIF
        old = __tzif_zone;
        __tzif_zone = zone;
        if (zone)
            __tzif_set_names(zone);
#endif
    }
    __LIBC_UNLOCK();
#ifdef __TZIF
    if (old != zone)
        free(old);
#endif
}

void
tzset(void)
{
    char    *tzenv;
    char     new_tzname[2][TZNAME_MAX + 2];
    tzrule_t new_tzrule[2] = {};

    tzenv = getenv("TZ");

#ifdef __TZIF
    /*
     * An unset TZ, ":name" and anything that isn't a rule string name
     * TZif files. When that fails, the time zone is what it would be
     * without them.
     */
    if (tzenv == NULL || *tzenv == ':') {
        if (__tzif_load(tzenv))
            return;
        __tzparse(tzenv, new_tzname, new_tzrule);
    } else if (!__tzparse(tzenv, new_tzname, new_tzrule) && __tzif_load(tzenv))
        return;
#else
    __tzparse(tzenv, new_tzname, new_tzrule);
#endif
#ifdef __TZIF
    __tzset_install(new_tzname, new_tzrule, NULL);
#else
    __tzset_install(new_tzname, new_tzrule);
#endif
}
