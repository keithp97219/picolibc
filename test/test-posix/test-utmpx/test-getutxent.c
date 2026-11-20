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

#define _POSIX_C_SOURCE 202405L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utmpx.h>

/* The user accounting database reads to its end and finds no strangers */

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
    struct utmpx  key, *entry;
    unsigned long count = 0;

    setutxent();
    while ((entry = getutxent()) != NULL)
        check(++count < 1000000, "the database ends");

    /* Nothing names a line or id that doesn't exist */
    memset(&key, 0, sizeof(key));
    strcpy(key.ut_line, "picolibc-no-line");
    setutxent();
    check(getutxline(&key) == NULL, "getutxline");
    key.ut_type = USER_PROCESS;
    memcpy(key.ut_id, "\x7f\x7e\x7d\x7c", sizeof(key.ut_id));
    setutxent();
    check(getutxid(&key) == NULL, "getutxid");
    endutxent();

#ifdef __PICOLIBC__
    /* picolibc keeps no database */
    check(count == 0, "empty");
    errno = 0;
    check(pututxline(&key) == NULL && errno == ENOSYS, "pututxline");
#endif
    printf("success\n");
    return 0;
}
