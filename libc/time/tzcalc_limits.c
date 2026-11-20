/* Copyright (c) 2002 Arthur David Olson */
/*
 * tzcalc_limits.c
 * Original Author: Adapted from tzcode maintained by Arthur David Olson.
 * Modifications:
 * - Changed to mktm_r and added __tzcalc_limits - 04/10/02, Jeff Johnston
 * - Fixed bug in mday computations - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Fixed bug in __tzcalc_limits - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Moved __tzcalc_limits() to separate file - 05/09/14, Freddie Chopin <freddie_chopin@op.pl>
 */

#include "local.h"
#include <limits.h>

/* Days from 1970-01-01 to January 1 of year, for any year */
static time_t
days_to_year(int year)
{
    time_t y = (time_t)year - 1;
    time_t d4 = (y >= 0 ? y : y - 3) / 4;
    time_t d100 = (y >= 0 ? y : y - 99) / 100;
    time_t d400 = (y >= 0 ? y : y - 399) / 400;

    /* 719162 is the number of days from 0001-01-01 to 1970-01-01 */
    return y * 365 + d4 - d100 + d400 - 719162;
}

int
__tzcalc_limits(int year)
{
    time_t days, year_days;
    int    i, j;

    __tzinfo.year = year;

    year_days = days_to_year(year);

    for (i = 0; i < 2; ++i) {
        if (__tzinfo.rule[i].ch == 'J') {
            /* The Julian day n (1 <= n <= 365). */
            days = year_days + __tzinfo.rule[i].d + (isleap(year) && __tzinfo.rule[i].d >= 60);
            /* Convert to yday */
            --days;
        } else if (__tzinfo.rule[i].ch == 'D')
            days = year_days + __tzinfo.rule[i].d;
        else {
            const int             yleap = isleap(year);
            int                   m_day, m_wday, wday_diff;
            const uint8_t * const ip = __month_lengths[yleap];

            days = year_days;

            for (j = 1; j < __tzinfo.rule[i].m; ++j)
                days += ip[j - 1];

            m_wday = (int)((EPOCH_WDAY + days) % DAYSPERWEEK);
            if (m_wday < 0)
                m_wday += DAYSPERWEEK;

            wday_diff = __tzinfo.rule[i].d - m_wday;
            if (wday_diff < 0)
                wday_diff += DAYSPERWEEK;
            m_day = (__tzinfo.rule[i].n - 1) * DAYSPERWEEK + wday_diff;

            while (m_day >= ip[j - 1])
                m_day -= DAYSPERWEEK;

            days += m_day;
        }

        /* store the change-over time in GMT form by adding offset */
        __tzinfo.rule[i].change = days * SECSPERDAY + __tzinfo.rule[i].s + __tzinfo.rule[i].offset;
    }

    __tzinfo.north = (__tzinfo.rule[0].change < __tzinfo.rule[1].change);

    return 1;
}

/*
 * Whether daylight saving time is in effect at t, whose UTC date is
 * day yday of year. A rule's transitions can fall in the next or the
 * previous year (EST5EDT,0/0,J365/25 ends DST on January 1), so near
 * the ends of the year the neighbouring years' rules are checked too.
 * Call with the lock held.
 */
int
__tz_isdst(time_t t, int year, int yday)
{
    int north, k;

    if (year != __tzinfo.year)
        __tzcalc_limits(year);
    north = __tzinfo.north;
    for (k = 0; k < 3; k++) {
        if (k == 1) {
            if (yday > 7 || year == INT_MIN)
                continue;
            __tzcalc_limits(year - 1);
        } else if (k == 2) {
            if (yday < 357 || year == INT_MAX)
                continue;
            __tzcalc_limits(year + 1);
        }
        if (__tzinfo.north) {
            if (t >= __tzinfo.rule[0].change && t < __tzinfo.rule[1].change)
                return 1;
        } else {
            if (t >= __tzinfo.rule[1].change && t < __tzinfo.rule[0].change)
                return 0;
        }
    }
    return !north;
}
