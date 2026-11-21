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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utmp.h>

/* <utmp.h> is <utmpx.h> under its traditional names */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    struct utmp   key, *entry;
    unsigned long count = 0;

    /* The traditional names reach the utmpx members */
    memset(&key, 0, sizeof(key));
    strcpy(key.ut_name, "picolibc");
    check(strcmp(key.ut_user, "picolibc") == 0, "ut_name");
    key.ut_time = 12345;
    check(key.ut_tv.tv_sec == 12345, "ut_time");
#ifdef __PICOLIBC__
    {
        /* This compiles without a cast only if struct utmp is struct utmpx */
        struct utmpx *x = &key;
        (void)x;
    }
#endif
    check(sizeof(key.ut_line) == UT_LINESIZE && sizeof(key.ut_user) == UT_NAMESIZE
              && sizeof(key.ut_host) == UT_HOSTSIZE,
          "sizes");

    check(utmpname(UTMP_FILE) == 0, "utmpname");
    setutent();
    while ((entry = getutent()) != NULL)
        check(++count < 1000000, "the database ends");

    strcpy(key.ut_line, "picolibc-no-line");
    setutent();
    check(getutline(&key) == NULL, "getutline");
    key.ut_type = USER_PROCESS;
    memcpy(key.ut_id, "\x7f\x7e\x7d\x7c", sizeof(key.ut_id));
    setutent();
    check(getutid(&key) == NULL, "getutid");
    endutent();

#ifdef __PICOLIBC__
    /* picolibc keeps no database */
    check(count == 0, "empty");
    errno = 0;
    check(pututline(&key) == NULL && errno == ENOSYS, "pututline");
    updwtmp(WTMP_FILE, &key);
#endif
    printf("success\n");
    return 0;
}
