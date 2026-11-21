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
#define _XOPEN_SOURCE   800
/* The application's own macros, defined before the header */
#define id   +
#define line +
#include <utmpx.h>
#undef id
#undef line

#include <stdio.h>

/*
 * <utmpx.h> defines struct utmpx with the POSIX members, pid_t and
 * struct timeval, the eight ut_type values, and the prototypes POSIX
 * gives; its prototypes don't use the application's names
 */

/* The header alone defines these */
static pid_t          p __attribute__((unused));
static struct timeval tv __attribute__((unused));
static struct utmpx   ut __attribute__((unused));

/* struct timeval as <sys/time.h> describes it */
#include <sys/time.h>
_Static_assert(_Generic(tv.tv_sec, time_t: 1, default: 0), "tv_sec");
#ifndef __GLIBC__
/* glibc's tv_usec isn't suseconds_t on 32-bit targets with 64-bit time */
_Static_assert(_Generic(tv.tv_usec, suseconds_t: 1, default: 0), "tv_usec");
#endif

/* The strings are arrays of char */
_Static_assert(_Generic(&ut.ut_user, char (*)[sizeof(ut.ut_user)]: 1, default: 0), "ut_user");
_Static_assert(_Generic(&ut.ut_id, char (*)[sizeof(ut.ut_id)]: 1, default: 0), "ut_id");
_Static_assert(_Generic(&ut.ut_line, char (*)[sizeof(ut.ut_line)]: 1, default: 0), "ut_line");
_Static_assert(_Generic(ut.ut_pid, pid_t: 1, default: 0), "ut_pid");
_Static_assert(_Generic(ut.ut_type, short: 1, default: 0), "ut_type");
#ifndef __GLIBC__
/* glibc's ut_tv is a pair of int32_t on 32-bit targets, not a struct timeval */
_Static_assert(_Generic(ut.ut_tv, struct timeval: 1, default: 0), "ut_tv");
#endif

/* Eight distinct values that fit in ut_type */
static const short types[] = {
    EMPTY, BOOT_TIME, OLD_TIME, NEW_TIME, USER_PROCESS, INIT_PROCESS, LOGIN_PROCESS, DEAD_PROCESS,
};

#define NTYPES (sizeof(types) / sizeof(types[0]))

static void          (         *const p_endutxent)(void) = endutxent;
static struct utmpx *(* const p_getutxent)(void) = getutxent;
static struct utmpx *(* const p_getutxid)(const struct utmpx *) = getutxid;
static struct utmpx *(* const p_getutxline)(const struct utmpx *) = getutxline;
static struct utmpx *(* const p_pututxline)(const struct utmpx *) = pututxline;
static void          (         *const p_setutxent)(void) = setutxent;

int
main(void)
{
    size_t i, j;

    for (i = 0; i < NTYPES; i++)
        for (j = i + 1; j < NTYPES; j++)
            if (types[i] == types[j]) {
                printf("ut_type values %zu and %zu are both %d\n", i, j, types[i]);
                return 1;
            }
    (void)p_endutxent;
    (void)p_getutxent;
    (void)p_getutxid;
    (void)p_getutxline;
    (void)p_pututxline;
    (void)p_setutxent;
    printf("success\n");
    return 0;
}
