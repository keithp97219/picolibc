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
#include <stdlib.h>

#include <stdio.h>

/*
 * In strict POSIX mode, without XSI, <stdlib.h> declares the base
 * interfaces, but neither the XSI ones nor names POSIX doesn't reserve
 * for it: interfaces removed from POSIX and BSD or GNU extensions.
 * Defining an object with a declared function's name doesn't compile.
 */

#if !defined(EXIT_FAILURE) || !defined(WIFEXITED) || !defined(MB_CUR_MAX)
#error "a <stdlib.h> macro is missing"
#endif

#ifdef alloca
#error "alloca isn't POSIX"
#endif

/* The XSI interfaces */
#ifndef __GLIBC__
/* glibc declares rand_r (removed in POSIX.1-2024) for any POSIX version */
static int rand_r __attribute__((unused));
#endif
static int a64l __attribute__((unused));
static int l64a __attribute__((unused));
static int drand48 __attribute__((unused));
static int erand48 __attribute__((unused));
static int jrand48 __attribute__((unused));
static int lcong48 __attribute__((unused));
static int lrand48 __attribute__((unused));
static int mrand48 __attribute__((unused));
static int nrand48 __attribute__((unused));
static int seed48 __attribute__((unused));
static int srand48 __attribute__((unused));
static int random __attribute__((unused));
static int srandom __attribute__((unused));
static int initstate __attribute__((unused));
static int setstate __attribute__((unused));
static int grantpt __attribute__((unused));
static int posix_openpt __attribute__((unused));
static int ptsname __attribute__((unused));
static int ptsname_r __attribute__((unused));
static int unlockpt __attribute__((unused));
static int putenv __attribute__((unused));
static int setkey __attribute__((unused));

/* Interfaces removed from POSIX, and extensions */
static int valloc __attribute__((unused));
static int ecvt __attribute__((unused));
static int fcvt __attribute__((unused));
static int gcvt __attribute__((unused));
static int mktemp __attribute__((unused));
static int mkstemps __attribute__((unused));
static int suboptarg __attribute__((unused));
static int reallocf __attribute__((unused));
static int arc4random __attribute__((unused));
static int itoa __attribute__((unused));
static int on_exit __attribute__((unused));
static int atoff __attribute__((unused));
static int strtod_l __attribute__((unused));

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)

int
main(void)
{
    int ok = 1;

    /* The base interfaces are there */
    ok &= check_type(getenv, char *(*)(const char *));
    ok &= check_type(setenv, int (*)(const char *, const char *, int));
    ok &= check_type(unsetenv, int (*)(const char *));
    ok &= check_type(mkstemp, int (*)(char *));
    ok &= check_type(mkdtemp, char *(*)(char *));
    ok &= check_type(getsubopt, int (*)(char **, char * const *, char **));
    ok &= check_type(posix_memalign, int (*)(void **, size_t, size_t));
    ok &= check_type(system, int (*)(const char *));
#ifndef __GLIBC__
    /* glibc declares these POSIX.1-2024 interfaces only with _GNU_SOURCE */
    ok &= check_type(mkostemp, int (*)(char *, int));
    ok &= check_type(reallocarray, void *(*)(void *, size_t, size_t));
    ok &= check_type(realpath, char *(*)(const char *, char *));
    ok &= check_type(secure_getenv, char *(*)(const char *));
#undef qsort_r
    ok &= check_type(
        qsort_r,
        void (*)(void *, size_t, size_t, int (*)(const void *, const void *, void *), void *));
#endif
    if (!ok) {
        printf("wrong prototype\n");
        return 1;
    }
    printf("success\n");
    return 0;
}
