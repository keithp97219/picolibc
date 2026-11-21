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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/*
 * strcasecmp, strncasecmp, strcasecmp_l and strncasecmp_l in the
 * POSIX locale: "as if the strings had been converted to lowercase and
 * then a byte comparison performed", errno unchanged. The _l versions
 * use the locale they are given, not the current one.
 */

static int ret;

#define fail(...)            \
    do {                     \
        printf(__VA_ARGS__); \
        printf("\n");        \
        ret = 1;             \
    } while (0)

#define MARK EDOM

static int
sign(int v)
{
    return (v > 0) - (v < 0);
}

/* The POSIX locale's lowercase */
static int
lower(unsigned char c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

static int
ref(const char *a, const char *b, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        int d = lower((unsigned char)a[i]) - lower((unsigned char)b[i]);

        if (d || !a[i])
            return sign(d);
    }
    return 0;
}

static locale_t posix;

static void
compare(const char *a, const char *b)
{
    size_t n, len = strlen(a) > strlen(b) ? strlen(a) : strlen(b);
    int    want = ref(a, b, SIZE_MAX);

    errno = MARK;
    if (sign(strcasecmp(a, b)) != want)
        fail("strcasecmp(\"%s\", \"%s\") is %d, not %d", a, b, strcasecmp(a, b), want);
    if (sign(strcasecmp_l(a, b, posix)) != want)
        fail("strcasecmp_l(\"%s\", \"%s\") is %d, not %d", a, b, strcasecmp_l(a, b, posix), want);
    for (n = 0; n <= len + 1; n++) {
        int wantn = ref(a, b, n);

        if (sign(strncasecmp(a, b, n)) != wantn)
            fail("strncasecmp(\"%s\", \"%s\", %zu) is %d, not %d", a, b, n, strncasecmp(a, b, n),
                 wantn);
        if (sign(strncasecmp_l(a, b, n, posix)) != wantn)
            fail("strncasecmp_l(\"%s\", \"%s\", %zu) is %d, not %d", a, b, n,
                 strncasecmp_l(a, b, n, posix), wantn);
    }
    if (errno != MARK)
        fail("comparing \"%s\" and \"%s\" changed errno", a, b);
}

static const char * const strings[] = {
    "",
    "a",
    "A",
    "b",
    "B",
    "z",
    "Z",
    "abc",
    "ABC",
    "aBc",
    "abd",
    "ABD",
    "ab",
    "abcd",
    /* Between the cases: '[' to '`' sort below lowercase, above uppercase */
    "[",
    "_",
    "`",
    "@",
    "{",
    "a_b",
    "A[B",
    "A_B",
    /* Bytes with the high bit set have no case here and sort above ASCII */
    "\x80",
    "\xc4",
    "\xe4",
    "\xff",
    "a\xc4",
    "A\xe4",
    "\x01",
    "zz\x7f",
};

#define N_STRINGS (sizeof(strings) / sizeof(strings[0]))

int
main(void)
{
    unsigned i, j;
    locale_t utf8;

    posix = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    if (posix == (locale_t)0) {
        printf("newlocale(\"POSIX\") failed\n");
        return 1;
    }
    for (i = 0; i < N_STRINGS; i++)
        for (j = 0; j < N_STRINGS; j++)
            compare(strings[i], strings[j]);

    /* Comparisons stop at n even without a null */
    {
        char a[3] = { 'a', 'B', 'c' };
        char b[3] = { 'A', 'b', 'C' };

        if (strncasecmp(a, b, 3) != 0 || strncasecmp_l(a, b, 3, posix) != 0)
            fail("strncasecmp read past n");
    }

    /*
     * The _l functions follow their locale argument, not the current
     * one: with another current locale they still give the POSIX
     * results
     */
    utf8 = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    if (utf8 != (locale_t)0) {
        locale_t old = uselocale(utf8);

        for (i = 0; i < N_STRINGS; i++)
            for (j = 0; j < N_STRINGS; j++) {
                int want = ref(strings[i], strings[j], SIZE_MAX);

                if (sign(strcasecmp_l(strings[i], strings[j], posix)) != want)
                    fail("strcasecmp_l(\"%s\", \"%s\") in C.UTF-8 isn't the POSIX result",
                         strings[i], strings[j]);
                if (sign(strncasecmp_l(strings[i], strings[j], 2, posix))
                    != ref(strings[i], strings[j], 2))
                    fail("strncasecmp_l(\"%s\", \"%s\") in C.UTF-8 isn't the POSIX result",
                         strings[i], strings[j]);
            }
        uselocale(old);
        freelocale(utf8);
    } else {
        printf("no C.UTF-8 locale; the current-locale check is skipped\n");
    }
    freelocale(posix);

    if (ret == 0)
        printf("success\n");
    return ret;
}
