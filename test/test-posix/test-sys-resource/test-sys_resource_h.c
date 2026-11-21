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
/* PRIO_*, RUSAGE_*, struct rusage and three of the functions are XSI */
#define _XOPEN_SOURCE 800
#include <sys/resource.h>

#include <stdio.h>

/* <sys/resource.h> defines its types, constants, structures and prototypes */

#if !defined(RLIM_INFINITY) || !defined(RLIM_SAVED_MAX) || !defined(RLIM_SAVED_CUR)
#error "RLIM_ constant missing"
#endif

/* rlim_t is unsigned */
_Static_assert((rlim_t)-1 > 0, "rlim_t unsigned");

#ifndef __GLIBC__
/*
 * The RLIM_ values are usable in #if; glibc's 64-bit RLIM_INFINITY is
 * a cast, which isn't
 */
#if RLIM_INFINITY == 0 || RLIM_SAVED_MAX == 0 || RLIM_SAVED_CUR == 0
#error "RLIM_ constant zero"
#endif
#endif

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)
#define check_expr(e, t) _Generic((e), t: 1, default: 0)

int
main(void)
{
    int           ok = 1;
    struct rlimit rl;
    struct rusage ru;
    id_t          id = 0;
    int           r;

    ok &= check_type(getrlimit, int (*)(int, struct rlimit *));
    ok &= check_type(setrlimit, int (*)(int, const struct rlimit *));
    ok &= check_type(getpriority, int (*)(int, id_t));
    ok &= check_type(setpriority, int (*)(int, id_t, int));
    ok &= check_type(getrusage, int (*)(int, struct rusage *));
    if (!ok) {
        printf("wrong prototype\n");
        return 1;
    }

    /* The structure members and their types */
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    ok &= check_expr(rl.rlim_cur, rlim_t) & check_expr(rl.rlim_max, rlim_t);
    ru.ru_utime.tv_sec = 0;
    ru.ru_stime.tv_usec = 0;
    ok &= check_expr(ru.ru_utime, struct timeval) & check_expr(ru.ru_stime, struct timeval);
    (void)rl;
    (void)ru;
    (void)id;
    if (!ok) {
        printf("wrong member type\n");
        return 1;
    }

    /* RLIM_INFINITY is a value of rlim_t */
    rl.rlim_cur = RLIM_INFINITY;
    if (rl.rlim_cur != RLIM_INFINITY) {
        printf("RLIM_INFINITY isn't an rlim_t value\n");
        return 1;
    }

    /* The resources, priorities and usage selectors are distinct (duplicate cases don't compile) */
    for (r = 0; r < 1; r++) {
        switch (r) {
        case RLIMIT_CORE:
        case RLIMIT_CPU:
        case RLIMIT_DATA:
        case RLIMIT_FSIZE:
        case RLIMIT_NOFILE:
        case RLIMIT_STACK:
        case RLIMIT_AS:
            break;
        }
        switch (r) {
        case PRIO_PROCESS:
        case PRIO_PGRP:
        case PRIO_USER:
            break;
        }
        switch (r) {
        case RUSAGE_SELF:
        case RUSAGE_CHILDREN:
            break;
        }
    }

    printf("success\n");
    return 0;
}
