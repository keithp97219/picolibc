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

#define _POSIX_C_SOURCE 200809L
#include <time.h>

/*
 * strftime fails with ERANGE when the result doesn't fit, and follows
 * POSIX.1-2024 for the + and 0 flags and the %C, %F, %G and %Y widths
 */

#include <errno.h>
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

/* glibc doesn't set errno when the result doesn't fit */
#ifndef __GLIBC__
#define IS_ERANGE(e) ((e) == ERANGE)
#else
#define IS_ERANGE(e) 1
#endif

static struct tm
date(int year, int mon, int mday, int wday, int yday)
{
    struct tm tm;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = mday;
    tm.tm_wday = wday;
    tm.tm_yday = yday;
    tm.tm_hour = 13;
    tm.tm_min = 5;
    tm.tm_sec = 9;
    return tm;
}

#ifndef __GLIBC__
static void
fmt(const struct tm *tm, const char *f, const char *want)
{
    char   buf[64];
    size_t n;

    n = strftime(buf, sizeof(buf), f, tm);
    if (n != strlen(want) || strcmp(buf, want) != 0) {
        printf("strftime(\"%s\"): got \"%s\" (%zu), want \"%s\"\n", f, buf, n, want);
        exit(1);
    }
}
#endif

int
main(void)
{
    struct tm tm;
    char      buf[64];

#ifndef __GLIBC__
    /* POSIX.1-2024 flags and widths (glibc ignores the + flag) */
    tm = date(1970, 1, 1, 4, 0);
    fmt(&tm, "%+4Y", "1970");
    tm = date(270, 1, 1, 0, 0);
    fmt(&tm, "%+4Y|%C%y|%+5Y|%+3C%y", "0270|0270|+0270|+0270");
    tm = date(17, 1, 1, 0, 0);
    fmt(&tm, "%C%y", "0017");
    tm = date(12345, 1, 1, 0, 0);
    fmt(&tm, "%Y|%+4Y|%05Y|%+5Y|%+3C%y|%06Y|%04C%y",
        "12345|+12345|12345|+12345|+12345|012345|012345");
    tm = date(2025, 3, 4, 2, 62);
    fmt(&tm, "%+12F|%012F|%10F|%+13F", "+02025-03-04|002025-03-04|2025-03-04|+002025-03-04");
    fmt(&tm, "%+6G|%06G", "+02025|002025");
    /* %C is two digits for years 0 to 9999 (glibc gives one) */
    tm = date(999, 1, 1, 0, 0);
    fmt(&tm, "%C", "09");
#endif

    /* ERANGE when the result and its NUL don't fit */
    tm = date(2025, 3, 4, 2, 62);
    errno = 0;
    check(strftime(buf, 10, "%Y-%m-%d", &tm) == 0 && IS_ERANGE(errno), "ERANGE");
    errno = 0;
    check(strftime(buf, 10, "abcdefghij", &tm) == 0 && IS_ERANGE(errno), "ERANGE, ordinary chars");
    check(strftime(buf, 11, "%Y-%m-%d", &tm) == 10 && strcmp(buf, "2025-03-04") == 0, "just fits");
    memset(buf, 'x', sizeof(buf));
    errno = 0;
    check(strftime(buf, 0, "abc", &tm) == 0 && IS_ERANGE(errno), "maxsize 0");
    check(buf[0] == 'x' && buf[1] == 'x', "nothing written with maxsize 0");

    printf("success\n");
    return 0;
}
