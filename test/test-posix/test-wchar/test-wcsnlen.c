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
#include "../posix-groups.h"
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0 && defined(_POSIX_MEMORY_PROTECTION) \
    && _POSIX_MEMORY_PROTECTION > 0
#include <sys/mman.h>
#define HAVE_GUARD_PAGE
#endif

/*
 * The POSIX wide-string additions: wcsnlen (which never reads past
 * maxlen; checked against a guard page where there is one), wcpcpy,
 * wcpncpy, wcsdup, wcslcpy, wcslcat, wcscasecmp, wcsncasecmp, their _l
 * forms, wcscoll_l and wcsxfrm_l.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SENTINEL EDOM

/* Through volatile pointers, so the compiler uses the library's functions */
static size_t (* volatile p_wcsnlen)(const wchar_t *, size_t) = wcsnlen;

static void
check_wcsnlen(void)
{
    static const wchar_t abc[] = L"abc";
    wchar_t              nonull[4] = { L'a', L'b', L'c', L'd' };

    check(p_wcsnlen(abc, 10) == 3, "wcsnlen before maxlen");
    check(p_wcsnlen(abc, 3) == 3, "wcsnlen at the null");
    check(p_wcsnlen(abc, 2) == 2, "wcsnlen stops at maxlen");
    check(p_wcsnlen(abc, 0) == 0, "wcsnlen with maxlen 0");
    check(p_wcsnlen(nonull, 4) == 4, "wcsnlen of an array without a null");

#ifdef HAVE_GUARD_PAGE
    {
        /* The array ends at a page that can't be read */
        long     page = sysconf(_SC_PAGESIZE);
        char    *map;
        wchar_t *end;
        size_t   i;

        map = mmap(NULL, 2 * (size_t)page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                   0);
        check(map != MAP_FAILED, "mmap");
        check(mprotect(map + page, (size_t)page, PROT_NONE) == 0, "guard page");
        end = (wchar_t *)(map + page);
        for (i = 1; i <= 8; i++)
            end[-(long)i] = L'x';
        for (i = 0; i <= 8; i++)
            check(p_wcsnlen(end - i, i) == i, "wcsnlen reads only maxlen characters");
        check(munmap(map, 2 * (size_t)page) == 0, "munmap");
    }
#else
    printf("no mmap guard page, wcsnlen checked only for its results\n");
#endif
}

static void
check_copies(void)
{
    wchar_t  buf[8];
    wchar_t *p;

    errno = SENTINEL;

    /* wcpcpy returns the address of the null it copied */
    wmemset(buf, L'x', 8);
    p = wcpcpy(buf, L"abc");
    check(p == buf + 3 && wmemcmp(buf, L"abc", 4) == 0, "wcpcpy");
    check(wcpcpy(buf, L"") == buf && buf[0] == L'\0', "wcpcpy of an empty string");

    /* wcpncpy pads with nulls and returns the first; or &ws1[n] */
    wmemset(buf, L'x', 8);
    p = wcpncpy(buf, L"ab", 5);
    check(p == buf + 2, "wcpncpy returns the first null");
    check(buf[0] == L'a' && buf[1] == L'b' && buf[2] == 0 && buf[3] == 0 && buf[4] == 0,
          "wcpncpy pads to n");
    check(buf[5] == L'x', "wcpncpy writes only n");
    wmemset(buf, L'x', 8);
    p = wcpncpy(buf, L"abcdef", 3);
    check(p == buf + 3 && buf[2] == L'c' && buf[3] == L'x', "wcpncpy without a null");
    p = wcpncpy(buf, L"abc", 3);
    check(p == buf + 3, "wcpncpy with n the length");
    check(wcpncpy(buf, L"abc", 0) == buf, "wcpncpy with n 0");

    /* wcslcpy and wcslcat return the length they tried to make */
    wmemset(buf, L'x', 8);
    check(wcslcpy(buf, L"abcdef", 4) == 6, "wcslcpy truncating");
    check(wmemcmp(buf, L"abc", 4) == 0 && buf[4] == L'x', "wcslcpy wrote dstsize");
    check(wcslcpy(buf, L"ab", 8) == 2 && wcscmp(buf, L"ab") == 0, "wcslcpy");
    buf[0] = L'q';
    check(wcslcpy(buf, L"abc", 0) == 3 && buf[0] == L'q', "wcslcpy with dstsize 0");
    wcscpy(buf, L"ab");
    check(wcslcat(buf, L"cdefgh", 5) == 8, "wcslcat truncating");
    check(wcscmp(buf, L"abcd") == 0, "wcslcat filled dstsize - 1");
    wcscpy(buf, L"ab");
    check(wcslcat(buf, L"cd", 8) == 4 && wcscmp(buf, L"abcd") == 0, "wcslcat");
    /* No null when it would land at dst + dstsize */
    wcscpy(buf, L"abc");
    check(wcslcat(buf, L"de", 4) == 5 && wcscmp(buf, L"abc") == 0, "wcslcat with no room");
    check(errno == SENTINEL, "errno after the copies");

    /* wcsdup */
    p = wcsdup(L"hello");
    check(p != NULL && wcscmp(p, L"hello") == 0, "wcsdup");
    free(p);
    p = wcsdup(L"");
    check(p != NULL && p[0] == L'\0', "wcsdup of an empty string");
    free(p);
}

static void
check_compares(void)
{
    locale_t c = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    wchar_t  buf[8];

    check(c != (locale_t)0, "newlocale(C)");
    errno = SENTINEL;

    /* In the POSIX locale, case-insensitive means compare as lowercase */
    check(wcscasecmp(L"Hello", L"hELLO") == 0, "wcscasecmp equal");
    check(wcscasecmp(L"abc", L"ABD") < 0, "wcscasecmp less");
    check(wcscasecmp(L"ABD", L"abc") > 0, "wcscasecmp greater");
    check(wcscasecmp(L"ab", L"ABC") < 0, "wcscasecmp prefix");
    /* '_' (0x5f) is between 'Z' and 'a': lowercase first */
    check(wcscasecmp(L"_", L"A") < 0, "wcscasecmp compares lowercase");
    check(wcscasecmp_l(L"Hello", L"hELLO", c) == 0, "wcscasecmp_l");
    check(wcscasecmp_l(L"_", L"a", c) < 0, "wcscasecmp_l lowercase");
    check(wcsncasecmp(L"abcX", L"ABCy", 3) == 0, "wcsncasecmp stops at n");
    check(wcsncasecmp(L"abcX", L"ABCy", 4) < 0, "wcsncasecmp at n");
    check(wcsncasecmp(L"a", L"b", 0) == 0, "wcsncasecmp with n 0");
    check(wcsncasecmp_l(L"abcX", L"ABCy", 3, c) == 0, "wcsncasecmp_l");

    /* The POSIX locale collates by code */
    check(wcscoll(L"a", L"b") < 0 && wcscoll(L"b", L"a") > 0, "wcscoll");
    check(wcscoll_l(L"a", L"b", c) < 0 && wcscoll_l(L"abc", L"abc", c) == 0, "wcscoll_l");
    check(wcsxfrm_l(buf, L"abc", 8, c) == 3, "wcsxfrm_l length");
    check(wcsxfrm_l(NULL, L"abcdef", 0, c) == 6, "wcsxfrm_l with n 0 and a null ws1");
    check(wcsxfrm(NULL, L"abcdef", 0) == 6, "wcsxfrm with n 0 and a null ws1");
    {
        /* The transformed strings compare as wcscoll compares the originals */
        wchar_t x1[8], x2[8];

        check(wcsxfrm_l(x1, L"ab", 8, c) < 8 && wcsxfrm_l(x2, L"ac", 8, c) < 8, "wcsxfrm_l fits");
        check(wcscmp(x1, x2) < 0, "wcsxfrm_l keeps the order");
    }
    check(errno == SENTINEL, "errno after the comparisons");
    freelocale(c);
}

int
main(void)
{
    check_wcsnlen();
    check_copies();
    check_compares();
    printf("success\n");
    return 0;
}
