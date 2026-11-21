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

/*
 * strcoll, strcoll_l, strxfrm and strxfrm_l: in the POSIX locale
 * collation is byte order; strcmp of transformed strings agrees with
 * strcoll (and strcoll_l with the same locale, whatever the current
 * one); strxfrm's length, its limit of n bytes and a null s1 when n is
 * 0; errno unchanged.
 */

static int ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

#define MARK EDOM
#define XMAX 512

static int
sign(int v)
{
    return (v > 0) - (v < 0);
}

static const char * const strings[] = {
    "",          "a",
    "A",         "b",
    "ab",        "abc",
    "abd",       "aB",
    "B",         "z",
    "_",         "0",
    "9",         " ",
    "\x7f",      "\x80",
    "\xc3\xa9",  "\xc3\xa8",
    "e\xcc\x81", "\xff",
    "a\xff",     "\xe2\x82\xac",
    "zz",
};

#define N_STRINGS (sizeof(strings) / sizeof(strings[0]))

/* Transform s in locale (or the current one), checking strxfrm's own rules */
static size_t
xfrm(const char *s, char *out, locale_t locale)
{
    size_t len, n, r;
    char   buf[XMAX + 8];

    errno = MARK;
    len = locale ? strxfrm_l(NULL, s, 0, locale) : strxfrm(NULL, s, 0);
    if (errno != MARK)
        fail("strxfrm(\"%s\") changed errno", s);
    if (len >= XMAX) {
        fail("strxfrm(\"%s\") is too long, %zu", s, len);
        return 0;
    }
    /* No more than n bytes are written, whatever n is */
    for (n = 0; n <= len + 1; n++) {
        memset(buf, 'x', sizeof(buf));
        r = locale ? strxfrm_l(buf, s, n, locale) : strxfrm(buf, s, n);
        if (r != len)
            fail("strxfrm(\"%s\", %zu) is %zu, not %zu", s, n, r, len);
        if (buf[n] != 'x')
            fail("strxfrm(\"%s\", %zu) wrote past n", s, n);
        if (n > len && buf[len] != '\0')
            fail("strxfrm(\"%s\", %zu) isn't terminated", s, n);
    }
    r = locale ? strxfrm_l(out, s, len + 1, locale) : strxfrm(out, s, len + 1);
    (void)r;
    return len;
}

static void
check_locale(const char *name, locale_t locale, int byte_order)
{
    static char xf[N_STRINGS][XMAX];
    unsigned    i, j;

    for (i = 0; i < N_STRINGS; i++)
        xfrm(strings[i], xf[i], locale);
    for (i = 0; i < N_STRINGS; i++)
        for (j = 0; j < N_STRINGS; j++) {
            const char *a = strings[i], *b = strings[j];
            int         coll;

            errno = MARK;
            coll = sign(locale ? strcoll_l(a, b, locale) : strcoll(a, b));
            if (errno != MARK)
                fail("%s: strcoll(\"%s\", \"%s\") changed errno", name, a, b);
            if (sign(strcmp(xf[i], xf[j])) != coll)
                fail("%s: strcmp of the transformed \"%s\" and \"%s\" is %d, strcoll %d", name, a,
                     b, sign(strcmp(xf[i], xf[j])), coll);
            if (byte_order && coll != sign(strcmp(a, b)))
                fail("%s: strcoll(\"%s\", \"%s\") is %d, strcmp %d", name, a, b, coll,
                     sign(strcmp(a, b)));
            if ((i == j) != (coll == 0) && strcmp(a, b) != 0 && byte_order)
                fail("%s: \"%s\" and \"%s\" collate equal", name, a, b);
        }
}

int
main(void)
{
    locale_t posix, utf8;

    posix = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    if (posix == (locale_t)0) {
        printf("newlocale(\"POSIX\") failed\n");
        return 1;
    }

    /* The program starts in the POSIX locale */
    check_locale("strcoll in the initial locale", (locale_t)0, 1);
    check_locale("strcoll_l in POSIX", posix, 1);

    utf8 = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    if (utf8 != (locale_t)0) {
        locale_t old;

        /* Any order, as long as strxfrm_l and strcoll_l agree */
        check_locale("strcoll_l in C.UTF-8", utf8, 0);
        /* With another current locale, strcoll_l still uses its argument */
        old = uselocale(utf8);
        check_locale("strcoll_l in POSIX with C.UTF-8 current", posix, 1);
        check_locale("strcoll with C.UTF-8 current", (locale_t)0, 0);
        uselocale(old);
        freelocale(utf8);
    } else {
        printf("no C.UTF-8 locale; only the POSIX locale is checked\n");
    }
    freelocale(posix);

    if (ret == 0)
        printf("success\n");
    return ret;
}
