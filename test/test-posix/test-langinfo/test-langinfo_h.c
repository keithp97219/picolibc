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
/* glibc declares ALTMON_1..ALTMON_12 only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <langinfo.h>

#include <stdio.h>
#include <stdlib.h>

/*
 * <langinfo.h> defines nl_item, locale_t, every item constant with a
 * distinct value, and the prototypes
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const nl_item items[] = {
    CODESET,
    D_T_FMT,
    D_FMT,
    T_FMT,
    T_FMT_AMPM,
    AM_STR,
    PM_STR,
    DAY_1,
    DAY_2,
    DAY_3,
    DAY_4,
    DAY_5,
    DAY_6,
    DAY_7,
    ABDAY_1,
    ABDAY_2,
    ABDAY_3,
    ABDAY_4,
    ABDAY_5,
    ABDAY_6,
    ABDAY_7,
    MON_1,
    MON_2,
    MON_3,
    MON_4,
    MON_5,
    MON_6,
    MON_7,
    MON_8,
    MON_9,
    MON_10,
    MON_11,
    MON_12,
    ABMON_1,
    ABMON_2,
    ABMON_3,
    ABMON_4,
    ABMON_5,
    ABMON_6,
    ABMON_7,
    ABMON_8,
    ABMON_9,
    ABMON_10,
    ABMON_11,
    ABMON_12,
    ERA,
    ERA_D_FMT,
    ERA_D_T_FMT,
    ERA_T_FMT,
    ALT_DIGITS,
    RADIXCHAR,
    THOUSEP,
    YESEXPR,
    NOEXPR,
    CRNCYSTR,
    ALTMON_1,
    ALTMON_2,
    ALTMON_3,
    ALTMON_4,
    ALTMON_5,
    ALTMON_6,
    ALTMON_7,
    ALTMON_8,
    ALTMON_9,
    ALTMON_10,
    ALTMON_11,
    ALTMON_12,
#ifndef __GLIBC__
    /* glibc 2.43 has no ABALTMON_ items */
    ABALTMON_1,
    ABALTMON_2,
    ABALTMON_3,
    ABALTMON_4,
    ABALTMON_5,
    ABALTMON_6,
    ABALTMON_7,
    ABALTMON_8,
    ABALTMON_9,
    ABALTMON_10,
    ABALTMON_11,
    ABALTMON_12,
#endif
};

#define NITEMS (sizeof(items) / sizeof(items[0]))

int
main(void)
{
    char  *(*p_nl_langinfo)(nl_item) = nl_langinfo;
    char  *(*p_nl_langinfo_l)(nl_item, locale_t) = nl_langinfo_l;
    size_t i, j;

    check(p_nl_langinfo != NULL && p_nl_langinfo_l != NULL, "prototypes");
    for (i = 0; i < NITEMS; i++)
        for (j = i + 1; j < NITEMS; j++)
            if (items[i] == items[j]) {
                printf("items %zu and %zu are both %ld\n", i, j, (long)items[i]);
                exit(1);
            }

    printf("success\n");
    return 0;
}
