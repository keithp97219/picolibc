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
/* glibc declares wcslcat and wcslcpy only with _DEFAULT_SOURCE */
#ifdef TEST_GLIBC
#define _DEFAULT_SOURCE
#endif

/*
 * Prototypes in the header must not use names an application may
 * define as macros
 */
#define a    )
#define c    )
#define h    )
#define n    )
#define s    )
#define wc   )
#define free )

#include <wchar.h>

#undef a
#undef c
#undef h
#undef n
#undef s
#undef wc
#undef free

#include <stdio.h>
#include <stdlib.h>

/* <wchar.h>: the types, macros, prototypes and namespace of POSIX.1-2024 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Types; struct tm is declared, possibly incomplete */
static FILE      *file_ptr __attribute__((unused));
static locale_t   locale __attribute__((unused));
static mbstate_t  state __attribute__((unused));
static size_t     size __attribute__((unused));
static va_list   *ap __attribute__((unused));
static wchar_t    wchar __attribute__((unused));
static wint_t     wint __attribute__((unused));
static struct tm *tm_ptr __attribute__((unused));

_Static_assert((size_t)-1 > 0, "size_t is unsigned");
_Static_assert(sizeof(wint_t) >= sizeof(wchar_t), "wint_t holds every wchar_t");
_Static_assert(sizeof(wint_t) <= sizeof(long), "wint_t no wider than long");

/* Macros */
#if !defined(WCHAR_MAX) || !defined(WCHAR_MIN) || !defined(WEOF) || !defined(NULL)
#error missing <wchar.h> macro
#endif
_Static_assert(_Generic(WEOF, wint_t: 1, default: 0), "WEOF is a wint_t");
_Static_assert(WCHAR_MAX >= 0x7f && WCHAR_MIN <= 0, "WCHAR_MIN and WCHAR_MAX");
_Static_assert((wchar_t)WCHAR_MAX == WCHAR_MAX && (wchar_t)WCHAR_MIN == WCHAR_MIN,
               "WCHAR limits fit wchar_t");
#if WCHAR_MAX < 0x7f || WCHAR_MIN > 0
#error WCHAR limits unusable in #if
#endif

/* Every function, with its type; _Generic doesn't evaluate, so nothing is linked */
#define proto(name, type) _Static_assert(_Generic((name), type: 1, default: 0), #name)

proto(btowc, wint_t (*)(int));
proto(fgetwc, wint_t (*)(FILE *));
proto(fgetws, wchar_t *(*)(wchar_t *, int, FILE *));
proto(fputwc, wint_t (*)(wchar_t, FILE *));
proto(fputws, int (*)(const wchar_t *, FILE *));
proto(fwide, int (*)(FILE *, int));
proto(fwprintf, int (*)(FILE *, const wchar_t *, ...));
proto(fwscanf, int (*)(FILE *, const wchar_t *, ...));
proto(getwc, wint_t (*)(FILE *));
proto(getwchar, wint_t (*)(void));
proto(mbrlen, size_t (*)(const char *, size_t, mbstate_t *));
proto(mbrtowc, size_t (*)(wchar_t *, const char *, size_t, mbstate_t *));
proto(mbsinit, int (*)(const mbstate_t *));
proto(mbsnrtowcs, size_t (*)(wchar_t *, const char **, size_t, size_t, mbstate_t *));
proto(mbsrtowcs, size_t (*)(wchar_t *, const char **, size_t, mbstate_t *));
proto(open_wmemstream, FILE *(*)(wchar_t **, size_t *));
proto(putwc, wint_t (*)(wchar_t, FILE *));
proto(putwchar, wint_t (*)(wchar_t));
proto(swprintf, int (*)(wchar_t *, size_t, const wchar_t *, ...));
proto(swscanf, int (*)(const wchar_t *, const wchar_t *, ...));
proto(ungetwc, wint_t (*)(wint_t, FILE *));
proto(vfwprintf, int (*)(FILE *, const wchar_t *, va_list));
proto(vfwscanf, int (*)(FILE *, const wchar_t *, va_list));
proto(vswprintf, int (*)(wchar_t *, size_t, const wchar_t *, va_list));
proto(vswscanf, int (*)(const wchar_t *, const wchar_t *, va_list));
proto(vwprintf, int (*)(const wchar_t *, va_list));
proto(vwscanf, int (*)(const wchar_t *, va_list));
proto(wcpcpy, wchar_t *(*)(wchar_t *, const wchar_t *));
proto(wcpncpy, wchar_t *(*)(wchar_t *, const wchar_t *, size_t));
proto(wcrtomb, size_t (*)(char *, wchar_t, mbstate_t *));
proto(wcscasecmp, int (*)(const wchar_t *, const wchar_t *));
proto(wcscasecmp_l, int (*)(const wchar_t *, const wchar_t *, locale_t));
proto(wcscat, wchar_t *(*)(wchar_t *, const wchar_t *));
proto(wcscmp, int (*)(const wchar_t *, const wchar_t *));
proto(wcscoll, int (*)(const wchar_t *, const wchar_t *));
proto(wcscoll_l, int (*)(const wchar_t *, const wchar_t *, locale_t));
proto(wcscpy, wchar_t *(*)(wchar_t *, const wchar_t *));
proto(wcscspn, size_t (*)(const wchar_t *, const wchar_t *));
proto(wcsdup, wchar_t *(*)(const wchar_t *));
proto(wcsftime, size_t (*)(wchar_t *, size_t, const wchar_t *, const struct tm *));
proto(wcslcat, size_t (*)(wchar_t *, const wchar_t *, size_t));
proto(wcslcpy, size_t (*)(wchar_t *, const wchar_t *, size_t));
proto(wcslen, size_t (*)(const wchar_t *));
proto(wcsncasecmp, int (*)(const wchar_t *, const wchar_t *, size_t));
proto(wcsncasecmp_l, int (*)(const wchar_t *, const wchar_t *, size_t, locale_t));
proto(wcsncat, wchar_t *(*)(wchar_t *, const wchar_t *, size_t));
proto(wcsncmp, int (*)(const wchar_t *, const wchar_t *, size_t));
proto(wcsncpy, wchar_t *(*)(wchar_t *, const wchar_t *, size_t));
proto(wcsnlen, size_t (*)(const wchar_t *, size_t));
proto(wcsnrtombs, size_t (*)(char *, const wchar_t **, size_t, size_t, mbstate_t *));
proto(wcsrtombs, size_t (*)(char *, const wchar_t **, size_t, mbstate_t *));
proto(wcsspn, size_t (*)(const wchar_t *, const wchar_t *));
proto(wcstod, double (*)(const wchar_t *, wchar_t **));
proto(wcstof, float (*)(const wchar_t *, wchar_t **));
proto(wcstok, wchar_t *(*)(wchar_t *, const wchar_t *, wchar_t **));
proto(wcstol, long (*)(const wchar_t *, wchar_t **, int));
proto(wcstoll, long long (*)(const wchar_t *, wchar_t **, int));
proto(wcstoul, unsigned long (*)(const wchar_t *, wchar_t **, int));
proto(wcstoull, unsigned long long (*)(const wchar_t *, wchar_t **, int));
proto(wcsxfrm, size_t (*)(wchar_t *, const wchar_t *, size_t));
proto(wcsxfrm_l, size_t (*)(wchar_t *, const wchar_t *, size_t, locale_t));
proto(wctob, int (*)(wint_t));
proto(wmemcmp, int (*)(const wchar_t *, const wchar_t *, size_t));
proto(wmemcpy, wchar_t *(*)(wchar_t *, const wchar_t *, size_t));
proto(wmemmove, wchar_t *(*)(wchar_t *, const wchar_t *, size_t));
proto(wmemset, wchar_t *(*)(wchar_t *, wchar_t, size_t));
proto(wprintf, int (*)(const wchar_t *, ...));
proto(wscanf, int (*)(const wchar_t *, ...));
#ifdef __LDBL_MANT_DIG__
proto(wcstold, long double (*)(const wchar_t *, wchar_t **));
#endif

/*
 * The search functions may be macros keeping the argument's const; the
 * functions themselves take a const pointer and return a plain one
 */
proto((wcschr), wchar_t *(*)(const wchar_t *, wchar_t));
proto((wcspbrk), wchar_t *(*)(const wchar_t *, const wchar_t *));
proto((wcsrchr), wchar_t *(*)(const wchar_t *, wchar_t));
proto((wcsstr), wchar_t *(*)(const wchar_t *, const wchar_t *));
proto((wmemchr), wchar_t *(*)(const wchar_t *, wchar_t, size_t));

/*
 * Strict POSIX doesn't reserve these names for <wchar.h>: the XSI
 * column functions, GNU and BSD extensions, and free (reserved only by
 * <stdlib.h>). A file-scope object with the name conflicts with any
 * declaration the header made.
 */
#ifndef TEST_GLIBC
static int wcwidth __attribute__((unused));
static int wcswidth __attribute__((unused));
static int wmempcpy __attribute__((unused));
static int wcsftime_l __attribute__((unused));
static int fgetwc_unlocked __attribute__((unused));
static int fputwc_unlocked __attribute__((unused));
static int wcstol_l __attribute__((unused));
#endif

int
main(void)
{
    /* Each is a function even where a macro exists */
    wchar_t        buf[8] = L"abcab";
    const wchar_t *cp;
    mbstate_t      ps = { 0 };

    cp = (wcschr)(buf, L'b');
    check(cp == buf + 1, "wcschr");
    cp = (wcsrchr)(buf, L'b');
    check(cp == buf + 4, "wcsrchr");
    cp = (wcspbrk)(buf, L"cx");
    check(cp == buf + 2, "wcspbrk");
    cp = (wcsstr)(buf, L"ca");
    check(cp == buf + 2, "wcsstr");
    cp = (wmemchr)(buf, L'\0', 8);
    check(cp == buf + 5, "wmemchr finds the terminator");
    check((mbsinit)(&ps) != 0, "mbsinit");
    check((wcslen)(buf) == 5, "wcslen");
    check((btowc)('A') == L'A' && (wctob)(L'A') == 'A', "btowc, wctob");
    check((btowc)(EOF) == WEOF && (wctob)(WEOF) == EOF, "EOF and WEOF");

    printf("success\n");
    return 0;
}
