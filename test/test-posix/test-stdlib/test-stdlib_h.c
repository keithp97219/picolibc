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
/* The rand48 family, random, a64l, grantpt and the others are XSI */
#define _XOPEN_SOURCE 800
/*
 * glibc declares mkostemp, qsort_r, reallocarray, secure_getenv and
 * ptsname_r only with _GNU_SOURCE
 */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <stdlib.h>

#include <stdio.h>

/*
 * <stdlib.h>: the macros, types and prototypes POSIX.1-2024 lists,
 * including the <sys/wait.h> and <fcntl.h> symbols, with the XSI
 * interfaces visible.
 */

#if !defined(EXIT_FAILURE) || !defined(EXIT_SUCCESS) || !defined(RAND_MAX) || !defined(MB_CUR_MAX) \
    || !defined(NULL)
#error "a <stdlib.h> macro is missing"
#endif

/* EXIT_FAILURE is between 1 and 125; EXIT_SUCCESS is 0; both in #if */
#if EXIT_FAILURE < 1 || EXIT_FAILURE > 125 || EXIT_SUCCESS != 0
#error "EXIT_FAILURE or EXIT_SUCCESS"
#endif

#if RAND_MAX < 32767
#error "RAND_MAX"
#endif

#if !defined(WEXITSTATUS) || !defined(WIFEXITED) || !defined(WIFSIGNALED) || !defined(WIFSTOPPED) \
    || !defined(WNOHANG) || !defined(WSTOPSIG) || !defined(WTERMSIG) || !defined(WUNTRACED)
#error "a <sys/wait.h> macro is missing"
#endif

#ifndef __GLIBC__
/* glibc's <stdlib.h> has neither WCOREDUMP nor the open flags */
#ifndef WCOREDUMP
#error "WCOREDUMP is missing"
#endif
#if !defined(O_APPEND) || !defined(O_CLOEXEC) || !defined(O_RDWR) || !defined(O_SYNC) \
    || !defined(O_NOCTTY) || !defined(O_DSYNC) || !defined(O_RSYNC)
#error "an open flag for mkostemp is missing"
#endif
#endif

#define check_type(f, t) _Generic(&(f), t: 1, default: 0)
#define check_expr(e, t) _Generic((e), t: 1, default: 0)

typedef int (*compar_t)(const void *, const void *);
typedef int (*compar_r_t)(const void *, const void *, void *);

int
main(void)
{
    int     ok = 1;
    div_t   d = { .quot = 1, .rem = 2 };
    ldiv_t  ld = { .quot = 1L, .rem = 2L };
    lldiv_t lld = { .quot = 1LL, .rem = 2LL };
    size_t  sz = 0;
    wchar_t wc = 0;

    ok &= check_expr(d.quot, int) && check_expr(d.rem, int);
    ok &= check_expr(ld.quot, long) && check_expr(ld.rem, long);
    ok &= check_expr(lld.quot, long long) && check_expr(lld.rem, long long);
    ok &= check_expr(MB_CUR_MAX, size_t);
    ok &= check_expr(sz, size_t) && sizeof(wc) > 0;
    if (!ok) {
        printf("wrong type\n");
        return 1;
    }
    if (MB_CUR_MAX != 1) {
        printf("MB_CUR_MAX is %u in the POSIX locale\n", (unsigned)MB_CUR_MAX);
        return 1;
    }
#ifndef __GLIBC__
    {
        int status = 0x0080 | 6;
        if (!WCOREDUMP(status) || WIFEXITED(status)) {
            printf("WCOREDUMP\n");
            return 1;
        }
    }
#endif

    ok &= check_type(_Exit, void (*)(int));
    ok &= check_type(a64l, long (*)(const char *));
    ok &= check_type(abort, void (*)(void));
    ok &= check_type(abs, int (*)(int));
    ok &= check_type(aligned_alloc, void *(*)(size_t, size_t));
    ok &= check_type(at_quick_exit, int (*)(void (*)(void)));
    ok &= check_type(atexit, int (*)(void (*)(void)));
    ok &= check_type(atof, double (*)(const char *));
    ok &= check_type(atoi, int (*)(const char *));
    ok &= check_type(atol, long (*)(const char *));
    ok &= check_type(atoll, long long (*)(const char *));
    ok &= check_type(calloc, void *(*)(size_t, size_t));
    ok &= check_type(div, div_t (*)(int, int));
    ok &= check_type(drand48, double (*)(void));
    ok &= check_type(erand48, double (*)(unsigned short[3]));
    ok &= check_type(exit, void (*)(int));
    ok &= check_type(free, void (*)(void *));
    ok &= check_type(getenv, char *(*)(const char *));
    ok &= check_type(getsubopt, int (*)(char **, char * const *, char **));
    ok &= check_type(grantpt, int (*)(int));
    ok &= check_type(initstate, char *(*)(unsigned, char *, size_t));
    ok &= check_type(jrand48, long (*)(unsigned short[3]));
    ok &= check_type(l64a, char *(*)(long));
    ok &= check_type(labs, long (*)(long));
    ok &= check_type(lcong48, void (*)(unsigned short[7]));
    ok &= check_type(ldiv, ldiv_t (*)(long, long));
    ok &= check_type(llabs, long long (*)(long long));
    ok &= check_type(lldiv, lldiv_t (*)(long long, long long));
    ok &= check_type(lrand48, long (*)(void));
    ok &= check_type(malloc, void *(*)(size_t));
    ok &= check_type(mblen, int (*)(const char *, size_t));
    ok &= check_type(mbstowcs, size_t (*)(wchar_t *, const char *, size_t));
    ok &= check_type(mbtowc, int (*)(wchar_t *, const char *, size_t));
    ok &= check_type(mkdtemp, char *(*)(char *));
    ok &= check_type(mkostemp, int (*)(char *, int));
    ok &= check_type(mkstemp, int (*)(char *));
    ok &= check_type(mrand48, long (*)(void));
    ok &= check_type(nrand48, long (*)(unsigned short[3]));
    ok &= check_type(posix_memalign, int (*)(void **, size_t, size_t));
    ok &= check_type(posix_openpt, int (*)(int));
    ok &= check_type(ptsname, char *(*)(int));
    ok &= check_type(ptsname_r, int (*)(int, char *, size_t));
    ok &= check_type(putenv, int (*)(char *));
    ok &= check_type(qsort, void (*)(void *, size_t, size_t, compar_t));
    ok &= check_type(quick_exit, void (*)(int));
    ok &= check_type(rand, int (*)(void));
    ok &= check_type(random, long (*)(void));
    ok &= check_type(realloc, void *(*)(void *, size_t));
    ok &= check_type(reallocarray, void *(*)(void *, size_t, size_t));
    ok &= check_type(realpath, char *(*)(const char *, char *));
    ok &= check_type(secure_getenv, char *(*)(const char *));
    ok &= check_type(seed48, unsigned short *(*)(unsigned short[3]));
    ok &= check_type(setenv, int (*)(const char *, const char *, int));
    ok &= check_type(setstate, char *(*)(char *));
    ok &= check_type(srand, void (*)(unsigned));
    ok &= check_type(srand48, void (*)(long));
    ok &= check_type(srandom, void (*)(unsigned));
    ok &= check_type(strtod, double (*)(const char *, char **));
    ok &= check_type(strtof, float (*)(const char *, char **));
    ok &= check_type(strtol, long (*)(const char *, char **, int));
    ok &= check_type(strtoll, long long (*)(const char *, char **, int));
    ok &= check_type(strtoul, unsigned long (*)(const char *, char **, int));
    ok &= check_type(strtoull, unsigned long long (*)(const char *, char **, int));
    ok &= check_type(system, int (*)(const char *));
    ok &= check_type(unlockpt, int (*)(int));
    ok &= check_type(unsetenv, int (*)(const char *));
    ok &= check_type(wcstombs, size_t (*)(char *, const wchar_t *, size_t));
    ok &= check_type(wctomb, int (*)(char *, wchar_t));
    /* picolibc's qsort_r may be a macro choosing the GNU or BSD order */
#undef qsort_r
    ok &= check_type(qsort_r, void (*)(void *, size_t, size_t, compar_r_t, void *));
    if (!ok) {
        printf("wrong prototype\n");
        return 1;
    }
    printf("success\n");
    return 0;
}
