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

#define _XOPEN_SOURCE 800
#include <time.h>

/* strptime: conversions, widths, flags and the members they set */

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

static struct tm tm;

/* Parse, check that rest is what's left, and return the result */
static void
parse(const char *buf, const char *f, const char *rest)
{
    char *end;

    memset(&tm, 0, sizeof(tm));
    end = strptime(buf, f, &tm);
    if (rest == NULL) {
        if (end != NULL) {
            printf("strptime(\"%s\", \"%s\") should fail\n", buf, f);
            exit(1);
        }
        return;
    }
    if (end == NULL || strcmp(end, rest) != 0) {
        printf("strptime(\"%s\", \"%s\"): left \"%s\", want \"%s\"\n", buf, f, end ? end : "(null)",
               rest);
        exit(1);
    }
    /* No interface sets tm_isdst negative */
    check(tm.tm_isdst >= 0, f);
}

int
main(void)
{
    check(setenv("TZ", "EST5EDT", 1) == 0, "setenv");
    tzset();

    /* The return value points past the last character parsed */
    parse("2025-03-04 rest", "%Y-%m-%d", " rest");
    check(tm.tm_year == 125 && tm.tm_mon == 2 && tm.tm_mday == 4, "%Y-%m-%d");

    /* Fixed-width conversions need no separators */
    parse("20250102", "%Y%m%d", "");
    check(tm.tm_year == 125 && tm.tm_mon == 0 && tm.tm_mday == 2, "%Y%m%d");
    parse("235960", "%H%M%S", "");
    check(tm.tm_hour == 23 && tm.tm_min == 59 && tm.tm_sec == 60, "%H%M%S");
    parse("12345", "%Y", "5");
    check(tm.tm_year == 1234 - 1900, "%Y reads four digits");

#ifdef __PICOLIBC__
    /* Field widths and the 0 and + flags (glibc ignores them) */
    parse("12345", "%5Y", "");
    check(tm.tm_year == 12345 - 1900, "%5Y");
    parse("+0270", "%+5Y", "");
    check(tm.tm_year == 270 - 1900, "%+5Y");
    parse("02025", "%05Y", "");
    check(tm.tm_year == 2025 - 1900, "%05Y");
    parse("+002025-03-04", "%F", "");
    check(tm.tm_year == 125 && tm.tm_mon == 2 && tm.tm_mday == 4, "%F with a wide year");
    parse("2025-03-04", "%8F", NULL);
#endif

    /* Names: full or abbreviated, any case */
    parse("Monday", "%a", "");
    check(tm.tm_wday == 1, "%a full name");
    parse("tue", "%A", "");
    check(tm.tm_wday == 2, "%A abbreviated name");
    parse("FEBRUARY", "%b", "");
    check(tm.tm_mon == 1, "%b full name");
    parse("mar", "%B", "");
    check(tm.tm_mon == 2, "%B abbreviated");
    parse("Apr", "%h", "");
    check(tm.tm_mon == 3, "%h");
    parse("Funday", "%a", NULL);

    /* Centuries and two-digit years */
    parse("69", "%y", "");
    check(tm.tm_year == 69, "%y 69 is 1969");
    parse("68", "%y", "");
    check(tm.tm_year == 168, "%y 68 is 2068");
    parse("00", "%y", "");
    check(tm.tm_year == 100, "%y 00 is 2000");
    parse("20 25", "%C %y", "");
    check(tm.tm_year == 125, "%C %y");
    parse("25 19", "%y %C", "");
    check(tm.tm_year == 25, "%y %C");
    parse("19", "%C", "");
    check(tm.tm_year == 0, "%C alone gives year 00");

    /* Numbers: ranges and leading zeros */
    parse("7", "%d", "");
    check(tm.tm_mday == 7, "%d without a leading zero");
    parse("32", "%d", NULL);
    parse("0", "%d", NULL);
    parse("13", "%m", NULL);
    parse("24", "%H", NULL);
#ifdef __PICOLIBC__ /* glibc accepts these */
    parse("60", "%M", NULL);
    parse("61", "%S", NULL);
#endif
    parse("13", "%I", NULL);
    parse("8", "%u", NULL);
    parse("7", "%w", NULL);
    parse("367", "%j", NULL);
    parse("060", "%j", "");
    check(tm.tm_yday == 59, "%j sets tm_yday to the day minus 1");
    parse("7", "%u", "");
    check(tm.tm_wday == 0, "%u is modulo 7");
    parse("3", "%w", "");
    check(tm.tm_wday == 3, "%w");

    /* %I with %p */
    parse("12:30 AM", "%I:%M %p", "");
    check(tm.tm_hour == 0 && tm.tm_min == 30, "12 AM");
    parse("12:30 pm", "%I:%M %p", "");
    check(tm.tm_hour == 12, "12 PM");
    parse("01:02:03 PM", "%r", "");
    check(tm.tm_hour == 13 && tm.tm_min == 2 && tm.tm_sec == 3, "%r");

    /* Composite conversions */
    parse("01/02/99", "%D", "");
    check(tm.tm_year == 99 && tm.tm_mon == 0 && tm.tm_mday == 2, "%D");
    parse("13:05", "%R", "");
    check(tm.tm_hour == 13 && tm.tm_min == 5, "%R");
    parse("13:05:09", "%T", "");
    check(tm.tm_hour == 13 && tm.tm_min == 5 && tm.tm_sec == 9, "%T");
    parse("Sat Jan  2 13:05:09 1999", "%c", "");
    check(tm.tm_year == 99 && tm.tm_mday == 2 && tm.tm_wday == 6 && tm.tm_hour == 13, "%c");
    parse("01/02/99", "%x", "");
    check(tm.tm_mon == 0 && tm.tm_mday == 2, "%x");
    parse("13:05:09", "%X", "");
    check(tm.tm_sec == 9, "%X");

    /* White space, %n, %t, ordinary characters and %% */
    parse("  2025 \t\n 03", "%Y %m", "");
    check(tm.tm_mon == 2, "white space");
    parse("2025\n03", "%Y%n%m", "");
    parse("2025\t03", "%Y%t%m", "");
    parse("100%", "%Y%%", "");
    parse("2025x03", "%Y-%m", NULL);

    /* Conversions whose effect is unspecified still parse */
    parse("1998 53 6", "%G %V %u", "");
    parse("98 53", "%g %V", "");
    parse("17 23", "%U %W", "");
    parse("-0430 x", "%z", " x");
    parse("1700000000", "%s", "");

    /* %Z matches tzname and sets tm_isdst */
#ifdef __PICOLIBC__
    parse("EDT", "%Z", ""); /* glibc doesn't set tm_isdst */
    check(tm.tm_isdst == 1, "%Z dst name");
#endif
    parse("EST", "%Z", "");
    check(tm.tm_isdst == 0, "%Z std name");

    /* E and O modifiers fall back to the plain conversions */
    parse("1999-01-02 13", "%EY-%Om-%Od %OH", "");
    check(tm.tm_year == 99 && tm.tm_mon == 0 && tm.tm_mday == 2 && tm.tm_hour == 13, "E and O");

    printf("success\n");
    return 0;
}
