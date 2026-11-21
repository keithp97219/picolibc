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
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * The restartable conversions mbrtowc, mbrlen, wcrtomb, mbsinit, btowc
 * and wctob: return values, null arguments, the conversion state,
 * EILSEQ, errno on success and the internal mbstate_t objects. The
 * UTF-8 checks run where a C.UTF-8 locale exists.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* errno keeps an unrelated value across calls that succeed */
#define SENTINEL EDOM

static void
reset(mbstate_t *ps)
{
    memset(ps, 0, sizeof(*ps));
}

static void
c_locale(void)
{
    mbstate_t ps;
    wchar_t   wc;
    char      buf[MB_LEN_MAX > 8 ? MB_LEN_MAX : 8];
    int       ch;

    reset(&ps);
    check(mbsinit(&ps) != 0, "a zeroed mbstate_t is the initial state");
    check(mbsinit(NULL) != 0, "mbsinit of a null pointer");

    /* Single characters, the null character and a null pwc */
    errno = SENTINEL;
    wc = 0;
    check(mbrtowc(&wc, "A", 1, &ps) == 1 && wc == L'A', "mbrtowc of A");
    wc = 1;
    check(mbrtowc(&wc, "", 1, &ps) == 0 && wc == L'\0', "mbrtowc of the null byte");
    check(mbsinit(&ps) != 0, "the initial state after the null character");
    check(mbrtowc(NULL, "B", 1, &ps) == 1, "mbrtowc with a null pwc");
    check(errno == SENTINEL, "mbrtowc leaves errno alone on success");

    /* n of 0 inspects nothing: an incomplete character */
    check(mbrtowc(&wc, "A", 0, &ps) == (size_t)-2, "mbrtowc with n 0");

    /* A null s is mbrtowc(NULL, "", 1, ps); pwc and n are ignored */
    wc = L'x';
    check(mbrtowc(&wc, NULL, 0, &ps) == 0, "mbrtowc with a null s");
    check(wc == L'x', "mbrtowc with a null s stores nothing");

    /* mbrlen counts the bytes */
    check(mbrlen("A", 1, &ps) == 1, "mbrlen of A");
    check(mbrlen("", 1, &ps) == 0, "mbrlen of the null byte");
    check(mbrlen("A", 0, &ps) == (size_t)-2, "mbrlen with n 0");
    check(errno == SENTINEL, "mbrlen leaves errno alone on success");

    /* wcrtomb stores the bytes, a null byte for L'\0' */
    memset(buf, 'x', sizeof(buf));
    check(wcrtomb(buf, L'A', &ps) == 1 && buf[0] == 'A', "wcrtomb of A");
    check(wcrtomb(buf, L'\0', &ps) == 1 && buf[0] == '\0', "wcrtomb of L'\\0'");
    check(mbsinit(&ps) != 0, "the initial state after L'\\0'");
    /* A null s is wcrtomb(buf, L'\0', ps) */
    check(wcrtomb(NULL, L'A', &ps) == 1, "wcrtomb with a null s");
    check(errno == SENTINEL, "wcrtomb leaves errno alone on success");

    /* Wide characters outside the 256 of the POSIX locale are encoding errors */
#if WCHAR_MAX > 0x10000
    errno = 0;
    check(wcrtomb(buf, (wchar_t)0x10000, &ps) == (size_t)-1 && errno == EILSEQ,
          "wcrtomb of a character the locale lacks");
    reset(&ps);
#endif

    /* The internal states, with a null ps */
    errno = SENTINEL;
    check(mbrtowc(&wc, "A", 1, NULL) == 1 && wc == L'A', "mbrtowc with a null ps");
    check(mbrlen("A", 1, NULL) == 1, "mbrlen with a null ps");
    check(wcrtomb(buf, L'A', NULL) == 1 && buf[0] == 'A', "wcrtomb with a null ps");
    check(errno == SENTINEL, "errno with a null ps");

    /* btowc and wctob: the portable characters are single bytes */
    check(btowc(EOF) == WEOF, "btowc(EOF)");
    check(wctob(WEOF) == EOF, "wctob(WEOF)");
    for (ch = 0; ch < 0x80; ch++) {
        check(btowc(ch) == (wint_t)ch, "btowc of an ASCII byte");
        check(wctob((wint_t)ch) == ch, "wctob of an ASCII character");
    }
#if WCHAR_MAX > 0x10000
    check(wctob((wint_t)0x10000) == EOF, "wctob of a character the locale lacks");
#endif
    check(errno == SENTINEL, "btowc and wctob leave errno alone");

    /*
     * XBD 6.2 and the CX text of btowc and mbrtowc: the POSIX locale has
     * 256 single-byte characters, so btowc(0x80) is not WEOF and no byte
     * gives EILSEQ. picolibc and glibc both treat bytes 0x80-0xff as
     * invalid in the C locale; that is not checked here.
     */
}

static void
utf8_locale(void)
{
    mbstate_t ps;
    wchar_t   wc;
    char      buf[MB_LEN_MAX > 8 ? MB_LEN_MAX : 8];

    reset(&ps);
    errno = SENTINEL;

    /* A whole character */
    check(mbrtowc(&wc, "\xc3\xa9", 2, &ps) == 2 && wc == 0xe9, "mbrtowc of U+00E9");
    check(mbrtowc(&wc, "\xc3\xa9", MB_CUR_MAX, &ps) == 2, "mbrtowc with n MB_CUR_MAX");
    check(mbrlen("\xc3\xa9", 2, &ps) == 2, "mbrlen of U+00E9");

    /* Split: -2 keeps the state, the rest returns the bytes that complete it */
    check(mbrtowc(&wc, "\xc3", 1, &ps) == (size_t)-2, "first byte");
    check(mbsinit(&ps) == 0, "not the initial state mid-character");
    check(mbrtowc(&wc, "\xa9", 1, &ps) == 1 && wc == 0xe9, "second byte");
    check(mbsinit(&ps) != 0, "initial state after the character");
    check(mbrtowc(&wc, "\xe2\x82", 2, &ps) == (size_t)-2, "two of three bytes");
    check(mbrtowc(&wc, "\xac", 1, &ps) == 1 && wc == 0x20ac, "the third byte");
#if WCHAR_MAX > 0x10000
    check(mbrtowc(&wc, "\xf0", 1, &ps) == (size_t)-2, "one of four bytes");
    check(mbrtowc(&wc, "\x9f", 1, &ps) == (size_t)-2, "two of four bytes");
    check(mbrtowc(&wc, "\x98\x80", 2, &ps) == 2 && wc == 0x1f600, "the last two bytes");
#endif
    check(mbrlen("\xc3", 1, &ps) == (size_t)-2, "mbrlen of a first byte");
    check(mbrlen("\xa9", 1, &ps) == 1, "mbrlen of the second byte");
    check(errno == SENTINEL, "errno after the conversions");

    /* Encoding errors */
    errno = 0;
    check(mbrtowc(&wc, "\xff", 1, &ps) == (size_t)-1 && errno == EILSEQ, "mbrtowc of 0xff");
    reset(&ps);
    errno = 0;
    check(mbrtowc(&wc, "\xc3\x41", 2, &ps) == (size_t)-1 && errno == EILSEQ,
          "a missing continuation byte");
    reset(&ps);
    errno = 0;
    check(mbrtowc(&wc, "\xc0\x80", 2, &ps) == (size_t)-1 && errno == EILSEQ,
          "an overlong encoding");
    reset(&ps);
    errno = 0;
    check(mbrtowc(&wc, "\xed\xa0\x80", 3, &ps) == (size_t)-1 && errno == EILSEQ,
          "an encoded surrogate");
    reset(&ps);
    errno = 0;
    check(mbrlen("\x80", 1, &ps) == (size_t)-1 && errno == EILSEQ, "mbrlen of a lone 0x80");
    reset(&ps);

    /* wcrtomb */
    errno = SENTINEL;
    check(wcrtomb(buf, 0xe9, &ps) == 2 && memcmp(buf, "\xc3\xa9", 2) == 0, "wcrtomb of U+00E9");
    check(wcrtomb(buf, 0x20ac, &ps) == 3 && memcmp(buf, "\xe2\x82\xac", 3) == 0,
          "wcrtomb of U+20AC");
    check(errno == SENTINEL, "errno after wcrtomb");
    errno = 0;
    check(wcrtomb(buf, (wchar_t)0xd800, &ps) == (size_t)-1 && errno == EILSEQ,
          "wcrtomb of a surrogate");
    reset(&ps);

    /* btowc and wctob only accept single-byte characters */
    check(btowc(0xc3) == WEOF, "btowc of a first byte");
    check(btowc('A') == L'A', "btowc of A");
    check(wctob(0xe9) == EOF, "wctob of a two-byte character");
    check(wctob(L'A') == 'A', "wctob of A");

    /*
     * Each function has its own internal state, and none calls
     * another's: a character mbrtowc began with a null ps isn't
     * continued by mbrlen with a null ps
     */
    check(mbrtowc(&wc, "\xc3", 1, NULL) == (size_t)-2, "mbrtowc's internal state");
    errno = 0;
    check(mbrlen("\xa9", 1, NULL) == (size_t)-1 && errno == EILSEQ, "mbrlen's own state");
    check(mbrtowc(&wc, "\xa9", 1, NULL) == 1 && wc == 0xe9, "mbrtowc's state kept");
    check(mbrlen("\xc3", 1, NULL) == (size_t)-2, "mbrlen's internal state");
    errno = 0;
    check(mbrtowc(&wc, "\xa9", 1, NULL) == (size_t)-1 && errno == EILSEQ, "mbrtowc's own state");
    check(mbrlen("\xa9", 1, NULL) == 1, "mbrlen's state kept");
}

int
main(void)
{
    c_locale();

    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        utf8_locale();
        check(setlocale(LC_CTYPE, "C") != NULL, "back to the C locale");
        c_locale();
    } else {
        printf("no C.UTF-8 locale, skipping the UTF-8 checks\n");
    }

    printf("success\n");
    return 0;
}
