/* Copyright (c) 2002 Jeff Johnston <jjohnstn@redhat.com> */
/* local header used by libc/time routines */
#define _GNU_SOURCE
#include <time.h>
#include <sys/lock.h>
#include <stdint.h>
#include <sys/syslimits.h>

#define SECSPERMIN                     60L
#define MINSPERHOUR                    60L
#define HOURSPERDAY                    24L
#define SECSPERHOUR                    (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY                     (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK                    7
#define MONSPERYEAR                    12

#define YEAR_BASE                      1900
#define EPOCH_YEAR                     1970
#define EPOCH_WDAY                     4
#define EPOCH_YEARS_SINCE_LEAP         2
#define EPOCH_YEARS_SINCE_CENTURY      70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

typedef struct {
    char    ch;
    int8_t  m; /* Month of year if ch=M */
    int8_t  n; /* Week of month if ch=M */
    int16_t d; /* Day of week if ch=M, day of year if ch=J or ch=D */
    int32_t s; /* Time of day in seconds */
    time_t  change;
    int32_t offset; /* Seconds west of UTC, as in timezone */
} tzrule_t;

typedef struct {
    int      north;
    int      year;
    tzrule_t rule[2];
} tzinfo_t;

static inline int
isleap(int y)
{
    // This routine must return exactly 0 or 1, because the result is used to index on
    // __month_lengths[]. The order of checks below is the fastest for a random year.
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

static inline int
isleap2(int y, int y_off)
{
    // range reduce to avoid integer overflow
    if (y_off)
        y = y % 400 + y_off;
    return isleap(y);
}

static inline clockid_t
timebase_to_clockid(int base)
{
    switch (base) {
    case TIME_UTC:
        return CLOCK_REALTIME;
    case TIME_MONOTONIC:
        return CLOCK_MONOTONIC;
    case TIME_ACTIVE:
        return CLOCK_PROCESS_CPUTIME_ID;
    case TIME_THREAD_ACTIVE:
        return CLOCK_THREAD_CPUTIME_ID;
    default:
        return (clockid_t)-1;
    }
}

int   __tzcalc_limits(int __year);
int   __tz_isdst(time_t __t, int __year, int __yday);
char *__strptime_fields(const char *buf, const char *format, struct tm *timeptr, locale_t locale,
                        int *fields);

/* The fields __strptime_fields reports */
#define __STRPTIME_MDAY  (1 << 0)
#define __STRPTIME_MON   (1 << 1)
#define __STRPTIME_YEAR  (1 << 2)
#define __STRPTIME_WDAY  (1 << 3)
#define __STRPTIME_YDAY  (1 << 4)
#define __STRPTIME_HOUR  (1 << 9)
#define __STRPTIME_MIN   (1 << 10)
#define __STRPTIME_SEC   (1 << 11)
#define __STRPTIME_ISDST (1 << 12)

extern const uint8_t __month_lengths[2][MONSPERYEAR];
extern char          __tzname_std[TZNAME_MAX + 2];
extern char          __tzname_dst[TZNAME_MAX + 2];
extern tzinfo_t      __tzinfo;

#ifdef __TZIF
#include <stdbool.h>

/* A time type from a TZif file */
struct __tzif_type {
    int32_t utoff;   /* Seconds east of UTC */
    uint8_t isdst;   /* Whether it's daylight saving time */
    uint8_t abbrind; /* Index of its abbreviation in abbr */
};

/*
 * A time zone loaded from a TZif file, in one allocation: this
 * structure, then the transition times, time types, the type index of
 * each transition and the TZ value it was loaded for. The
 * abbreviations are kept apart, for the life of the process, so that
 * tm_zone stays valid after TZ changes.
 */
struct __tzif {
    int64_t            *trans;          /* Transition times, ascending */
    struct __tzif_type *types;          /* Time types */
    uint8_t            *idx;            /* Type index of each transition */
    const char         *abbr;           /* Abbreviations, never freed */
    char               *name;           /* TZ value it was loaded for */
    uint32_t            timecnt;        /* Number of transitions */
    uint16_t            typecnt;        /* Number of time types */
    uint8_t             footer;         /* Whether __tzinfo holds a footer rule */
    uint8_t             deflt;          /* Whether it is /etc/localtime, for an unset TZ */
    const char         *footer_name[2]; /* The footer's names, never freed */
};

extern struct __tzif *__tzif_zone;

bool __tzparse(const char *tzenv, char new_tzname[2][TZNAME_MAX + 2], tzrule_t new_tzrule[2]);
void __tzset_install(char new_tzname[2][TZNAME_MAX + 2], tzrule_t new_tzrule[2],
                     struct __tzif *zone);
bool __tzif_load(const char *tzenv);
void __tzif_set_names(const struct __tzif *zone);
const struct __tzif_type *__tzif_type_at(const struct __tzif *zone, time_t t);
#endif
