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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * mbsrtowcs, mbsnrtowcs, wcsrtombs and wcsnrtombs: the source pointer,
 * the length limits, a null destination, a null ps, EILSEQ and errno on
 * success. The UTF-8 checks run where a C.UTF-8 locale exists.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SENTINEL EDOM

static void
reset(mbstate_t *ps)
{
    memset(ps, 0, sizeof(*ps));
}

static void
c_locale(void)
{
    static const char    abc[] = "abc";
    static const wchar_t wabc[] = L"abc";
    const char          *src;
    const wchar_t       *wsrc;
    wchar_t              wbuf[8];
    char                 buf[8];
    mbstate_t            ps;

    reset(&ps);
    errno = SENTINEL;

    /* The whole string, its null included; *src becomes a null pointer */
    src = abc;
    wmemset(wbuf, L'x', 8);
    check(mbsrtowcs(wbuf, &src, 8, &ps) == 3, "mbsrtowcs count");
    check(src == NULL, "mbsrtowcs reached the null");
    check(wmemcmp(wbuf, L"abc", 4) == 0, "mbsrtowcs stored the null");
    check(mbsinit(&ps) != 0, "mbsrtowcs ends in the initial state");

    /* len stops it; *src is just past the last character converted */
    src = abc;
    wmemset(wbuf, L'x', 8);
    check(mbsrtowcs(wbuf, &src, 2, &ps) == 2 && src == abc + 2, "mbsrtowcs with len 2");
    check(wbuf[0] == L'a' && wbuf[1] == L'b' && wbuf[2] == L'x', "only len stored");

    /* A null dst counts and leaves *src alone */
    src = abc;
    check(mbsrtowcs(NULL, &src, 0, &ps) == 3 && src == abc, "mbsrtowcs with a null dst");

    /* mbsnrtowcs stops after nmc bytes; *src is past the last byte processed */
    src = abc;
    check(mbsnrtowcs(wbuf, &src, 2, 8, &ps) == 2 && src == abc + 2, "mbsnrtowcs nmc 2");
    src = abc;
    check(mbsnrtowcs(wbuf, &src, 4, 8, &ps) == 3 && src == NULL, "mbsnrtowcs nmc with the null");
    src = abc;
    check(mbsnrtowcs(NULL, &src, 2, 0, &ps) == 2 && src == abc, "mbsnrtowcs with a null dst");

    /* wcsrtombs */
    wsrc = wabc;
    memset(buf, 'x', sizeof(buf));
    check(wcsrtombs(buf, &wsrc, 8, &ps) == 3 && wsrc == NULL, "wcsrtombs count");
    check(memcmp(buf, "abc", 4) == 0, "wcsrtombs stored the null");
    wsrc = wabc;
    memset(buf, 'x', sizeof(buf));
    check(wcsrtombs(buf, &wsrc, 2, &ps) == 2 && wsrc == wabc + 2, "wcsrtombs with len 2");
    check(buf[2] == 'x', "wcsrtombs stores at most len bytes");
    wsrc = wabc;
    check(wcsrtombs(NULL, &wsrc, 0, &ps) == 3 && wsrc == wabc, "wcsrtombs with a null dst");

    /* wcsnrtombs converts at most nwc wide characters */
    wsrc = wabc;
    check(wcsnrtombs(buf, &wsrc, 2, 8, &ps) == 2 && wsrc == wabc + 2, "wcsnrtombs nwc 2");
    wsrc = wabc;
    check(wcsnrtombs(buf, &wsrc, 4, 8, &ps) == 3 && wsrc == NULL, "wcsnrtombs nwc with the null");
    check(errno == SENTINEL, "errno after the conversions");

    /* A null ps uses each function's internal state */
    src = abc;
    check(mbsrtowcs(wbuf, &src, 8, NULL) == 3, "mbsrtowcs with a null ps");
    src = abc;
    check(mbsnrtowcs(wbuf, &src, 8, 8, NULL) == 3, "mbsnrtowcs with a null ps");
    wsrc = wabc;
    check(wcsrtombs(buf, &wsrc, 8, NULL) == 3, "wcsrtombs with a null ps");
    wsrc = wabc;
    check(wcsnrtombs(buf, &wsrc, 8, 8, NULL) == 3, "wcsnrtombs with a null ps");
    check(errno == SENTINEL, "errno with a null ps");

#if WCHAR_MAX > 0x10000
    {
        /* A wide character the locale lacks is an encoding error */
        static const wchar_t bad[] = { L'a', (wchar_t)0x10000, L'b', L'\0' };

        wsrc = bad;
        errno = 0;
        check(wcsrtombs(buf, &wsrc, 8, &ps) == (size_t)-1 && errno == EILSEQ, "wcsrtombs EILSEQ");
        reset(&ps);
        wsrc = bad;
        errno = 0;
        check(wcsrtombs(buf, &wsrc, 8, NULL) == (size_t)-1 && errno == EILSEQ,
              "wcsrtombs EILSEQ with a null ps");
        wsrc = bad;
        errno = 0;
        check(wcsnrtombs(NULL, &wsrc, 8, 0, NULL) == (size_t)-1 && errno == EILSEQ,
              "wcsnrtombs EILSEQ with a null dst");
    }
#endif
}

static void
utf8_locale(void)
{
    static const char    mixed[] = "a\xc3\xa9z";
    static const char    bad[] = "a\xff\x62";
    static const wchar_t wmixed[] = { L'a', 0xe9, L'z', L'\0' };
    const char          *src;
    const wchar_t       *wsrc;
    wchar_t              wbuf[8];
    char                 buf[8];
    mbstate_t            ps;

    reset(&ps);
    errno = SENTINEL;

    src = mixed;
    check(mbsrtowcs(wbuf, &src, 8, &ps) == 3 && src == NULL, "mbsrtowcs of UTF-8");
    check(wbuf[0] == L'a' && wbuf[1] == 0xe9 && wbuf[2] == L'z' && wbuf[3] == 0, "the characters");
    src = mixed;
    check(mbsrtowcs(NULL, &src, 0, &ps) == 3, "mbsrtowcs counts characters");

    /* mbsnrtowcs stops inside a character; the next call completes it */
    src = mixed;
    check(mbsnrtowcs(wbuf, &src, 2, 8, &ps) == 1 && src == mixed + 2,
          "mbsnrtowcs stops mid-character");
    check(mbsinit(&ps) == 0, "the state holds the partial character");
    check(mbsnrtowcs(wbuf, &src, 8, 8, &ps) == 2 && src == NULL, "mbsnrtowcs completes it");
    check(wbuf[0] == 0xe9 && wbuf[1] == L'z', "the completed characters");

    wsrc = wmixed;
    check(wcsrtombs(buf, &wsrc, 8, &ps) == 4 && wsrc == NULL, "wcsrtombs of UTF-8");
    check(memcmp(buf, mixed, 5) == 0, "the bytes");
    wsrc = wmixed;
    check(wcsrtombs(NULL, &wsrc, 0, &ps) == 4, "wcsrtombs counts bytes");
    /* A character that doesn't fit in len isn't stored */
    wsrc = wmixed;
    check(wcsrtombs(buf, &wsrc, 2, &ps) == 1 && wsrc == wmixed + 1, "wcsrtombs stops before");
    check(errno == SENTINEL, "errno after the conversions");

    /* An invalid byte: EILSEQ, *src at the invalid byte */
    src = bad;
    errno = 0;
    check(mbsrtowcs(wbuf, &src, 8, &ps) == (size_t)-1 && errno == EILSEQ, "mbsrtowcs EILSEQ");
    check(src == bad + 1, "mbsrtowcs leaves src past the last character converted");
    reset(&ps);
    src = bad;
    errno = 0;
    check(mbsnrtowcs(NULL, &src, 8, 0, NULL) == (size_t)-1 && errno == EILSEQ,
          "mbsnrtowcs EILSEQ with a null ps");
    {
        static const wchar_t surrogate[] = { L'a', (wchar_t)0xd800, L'\0' };

        wsrc = surrogate;
        errno = 0;
        check(wcsrtombs(buf, &wsrc, 8, &ps) == (size_t)-1 && errno == EILSEQ, "wcsrtombs EILSEQ");
        check(wsrc == surrogate + 1, "wcsrtombs leaves src past the last character converted");
        reset(&ps);
    }

    /*
     * Each function has its own internal state: a character mbsnrtowcs
     * left unfinished isn't completed by mbsrtowcs
     */
    src = "\xc3";
    check(mbsnrtowcs(wbuf, &src, 1, 8, NULL) == 0, "mbsnrtowcs leaves a partial character");
    src = "\xa9";
    errno = 0;
    check(mbsrtowcs(wbuf, &src, 8, NULL) == (size_t)-1 && errno == EILSEQ, "mbsrtowcs's own state");
    src = "\xa9";
    check(mbsnrtowcs(wbuf, &src, 1, 8, NULL) == 1 && wbuf[0] == 0xe9, "mbsnrtowcs's state kept");
}

int
main(void)
{
    c_locale();

    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        utf8_locale();
        check(setlocale(LC_CTYPE, "C") != NULL, "back to the C locale");
    } else {
        printf("no C.UTF-8 locale, skipping the UTF-8 checks\n");
    }

    printf("success\n");
    return 0;
}
