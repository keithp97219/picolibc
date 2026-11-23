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

/* getdate and getdate_err are XSI interfaces */
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/*
 * getdate uses the first template matching all of the input, fills in
 * what the template didn't give from the current time, and rejects
 * invalid dates and template files that aren't regular files
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/*
 * Point environ at an array of our own holding TZ and DATEMSK, so the
 * test doesn't depend on setenv
 */
extern char **environ;

static char   datemsk[300];
static char  *env[] = { "TZ=UTC0", datemsk, NULL };

static void
set_datemsk(const char *path)
{
    snprintf(datemsk, sizeof(datemsk), "DATEMSK=%s", path);
}

static char dir[64];

static void
templates(const char *text)
{
    FILE *f = fopen("datemsk", "w");

    check(f != NULL && fputs(text, f) >= 0 && fclose(f) == 0, "write templates");
}

/* getdate, retried if the current time crossed a second, minute or day */
static struct tm *
get(const char *s, struct tm *now)
{
    struct tm *r;
    time_t     t0, t1;

    do {
        t0 = time(NULL);
        r = getdate(s);
        t1 = time(NULL);
    } while (t0 != t1);
    check(localtime_r(&t0, now) != NULL, "localtime_r");
    return r;
}

int
main(void)
{
    struct tm *r, now, want;
    char       path[256];

    snprintf(dir, sizeof(dir), "tmp-getdate-%ld", (long)getpid());
    check(mkdir(dir, 0700) == 0 && chdir(dir) == 0, "scratch directory");
    set_datemsk("");
    environ = env;
    tzset();

    /* 4: not a regular file */
    check(mkdir("subdir", 0700) == 0, "mkdir");
    check(getcwd(path, sizeof(path) - 8) != NULL, "getcwd");
    strcat(path, "/subdir");
    set_datemsk(path);
    check(getdate("Monday") == NULL && getdate_err == 4, "template is a directory");

    check(getcwd(path, sizeof(path) - 9) != NULL, "getcwd");
    strcat(path, "/datemsk");
    set_datemsk(path);

    /* The first template matching the whole input is used */
    templates("%Y\n%Y-%m-%d\n%m/%d/%Y %H:%M:%S\n%A\n%B\n%H:%M\n%B %d\n");
    r = get("2024-02-29", &now);
    check(r != NULL, "getdate full date");
    check(r->tm_year == 124 && r->tm_mon == 1 && r->tm_mday == 29 && r->tm_wday == 4
              && r->tm_yday == 59,
          "full date");
    /* No time given: the current time */
    check(r->tm_hour == now.tm_hour && r->tm_min == now.tm_min && r->tm_sec == now.tm_sec,
          "current time assumed");

    r = get("03/04/2025 05:06:07", &now);
    check(r != NULL && r->tm_year == 125 && r->tm_mon == 2 && r->tm_mday == 4 && r->tm_hour == 5
              && r->tm_min == 6 && r->tm_sec == 7,
          "date and time");

    /* 7: no template matches all of the input */
    check(getdate("03/04/2025 junk") == NULL && getdate_err == 7, "partial match");
    check(getdate("nonsense") == NULL && getdate_err == 7, "no match");

    /* 8: invalid input */
    check(getdate("02/31/2025 00:00:00") == NULL && getdate_err == 8, "February 31");
    check(getdate("2025-04-31") == NULL && getdate_err == 8, "April 31");

    /* Only a weekday: today or the next such day */
    r = get("wEdNeSdAy", &now);
    check(r != NULL && r->tm_wday == 3, "weekday");
    want = now;
    want.tm_mday += (3 - now.tm_wday + 7) % 7;
    want.tm_isdst = -1;
    check(mktime(&want) != (time_t)-1, "mktime");
    check(r->tm_year == want.tm_year && r->tm_mon == want.tm_mon && r->tm_mday == want.tm_mday,
          "first matching day from today");

    /* Only an hour: this hour or the next such hour */
    r = get("00:00", &now);
    check(r != NULL && r->tm_hour == 0 && r->tm_min == 0, "hour");
    want = now;
    if (now.tm_hour > 0)
        want.tm_mday++;
    want.tm_isdst = -1;
    check(mktime(&want) != (time_t)-1, "mktime");
    check(r->tm_mday == want.tm_mday && r->tm_mon == want.tm_mon, "first matching hour from now");

#ifndef __GLIBC__
    /* Only a month: this month or the next such month, first day (glibc keeps the day) */
    r = get("January", &now);
    check(r != NULL && r->tm_mon == 0 && r->tm_mday == 1, "month alone");
    check(r->tm_year == now.tm_year + (now.tm_mon > 0), "first matching month from now");
#endif
    r = get("December 25", &now);
    check(r != NULL && r->tm_mon == 11 && r->tm_mday == 25 && r->tm_year == now.tm_year,
          "month and day, this year");

    check(unlink("datemsk") == 0 && rmdir("subdir") == 0, "cleanup");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
}
