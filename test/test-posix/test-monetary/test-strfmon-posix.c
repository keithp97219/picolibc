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
#include <monetary.h>

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * strfmon in the POSIX locale, where every monetary value is
 * unavailable: the flags, widths and precisions, the sign rules, and
 * E2BIG at the exact buffer size. Also <monetary.h>'s types and
 * prototypes.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define is_type(e, t) _Generic((e), t: 1, default: 0)

static const struct {
    const char *format;
    double      value;
    const char *expect;
} cases[] = {
    /* Plain characters and %% */
    { "a%%b",      0.0,    "a%b"      },
    { "[%.2n]",    1.5,    "[1.50]"   },
    { "%.2i",      1.5,    "1.50"     },
    /* Without '+' or '(' the sign is '-' and precedes the value */
    { "%.2n",      -1.5,   "-1.50"    },
    { "%.2i",      -1.5,   "-1.50"    },
    /* '(' encloses negative values */
    { "%(.2n",     -1.5,   "(1.50)"   },
    { "%(.2n",     1.5,    "1.50"     },
    /* '!' and '^' change nothing without a currency symbol or grouping */
    { "%!.2n",     -1.5,   "-1.50"    },
    { "%^.2n",     1234.5, "1234.50"  },
    /* Field width, right- or left-justified */
    { "%8.2n",     -1.5,   "   -1.50" },
    { "%-8.2n",    -1.5,   "-1.50   " },
    { "%-8.2n",    1.5,    "1.50    " },
    /* Left precision pads with the fill character and aligns signs */
    { "%#3.2n",    12.5,   "  12.50"  },
    { "%#3.2n",    -12.5,  "- 12.50"  },
    { "%=*#3.2n",  12.5,   " *12.50"  },
    { "%=*#3.2n",  -12.5,  "-*12.50"  },
    { "%=0#3.2n",  2.5,    " 002.50"  },
    /* The fill is ignored without a left precision */
    { "%=*8.2n",   2.5,    "    2.50" },
    /* Right precision rounds; 0 drops the radix character */
    { "%.1n",      0.26,   "0.3"      },
    { "%.0n",      2.75,   "3"        },
    { "%.3n",      2.5,    "2.500"    },
#ifndef __GLIBC__
    /* glibc doesn't pad positive values to match the parentheses */
    { "%(#3.2n",   12.5,   "  12.50 " },
    { "%(#3.2n",   -12.5,  "( 12.50)" },
    { "%(=*#3.2n", 12.5,   " *12.50 " },
#endif
};

#define NCASES (sizeof(cases) / sizeof(cases[0]))

int
main(void)
{
    ssize_t (*p_strfmon)(char * restrict, size_t, const char * restrict, ...) = strfmon;
    ssize_t (*p_strfmon_l)(char * restrict, size_t, locale_t, const char * restrict, ...)
        = strfmon_l;
    char     buf[64];
    size_t   i;
    ssize_t  n;
    locale_t l;

#if defined(__PICOLIBC__) && !defined(__IO_FLOAT_EXACT)
    printf("skipping on imprecise float stdio target\n");
    return 77;
#endif

    check(p_strfmon != NULL && p_strfmon_l != NULL, "prototypes");
    check(is_type(strfmon(buf, sizeof(buf), "x"), ssize_t), "returns ssize_t");
    check(is_type(sizeof(buf), size_t), "size_t");

    l = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(l != (locale_t)0, "newlocale");

    for (i = 0; i < NCASES; i++) {
        memset(buf, 'x', sizeof(buf));
        n = strfmon(buf, sizeof(buf), cases[i].format, cases[i].value);
        if (n != (ssize_t)strlen(cases[i].expect) || strcmp(buf, cases[i].expect) != 0) {
            printf("strfmon \"%s\" %g: got %zd \"%s\", expected \"%s\"\n", cases[i].format,
                   cases[i].value, n, n >= 0 ? buf : "", cases[i].expect);
            exit(1);
        }
        n = strfmon_l(buf, sizeof(buf), l, cases[i].format, cases[i].value);
        if (n != (ssize_t)strlen(cases[i].expect) || strcmp(buf, cases[i].expect) != 0) {
            printf("strfmon_l \"%s\" %g: got %zd\n", cases[i].format, cases[i].value, n);
            exit(1);
        }
    }

    /* Several conversions, and extra arguments are ignored */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-extra-args"
    n = strfmon(buf, sizeof(buf), "%.2n %.1n", 1.0, -2.0, 3.0);
#pragma GCC diagnostic pop
    check(n == 9 && strcmp(buf, "1.00 -2.0") == 0, "two conversions");

#ifndef __GLIBC__
    /* glibc formats '+' even though both sign strings are empty */
    errno = 0;
    check(strfmon(buf, sizeof(buf), "%+.2n", 1.5) == -1 && errno == EINVAL, "'+' is EINVAL");
    errno = 0;
    check(strfmon_l(buf, sizeof(buf), l, "%+.2n", -1.5) == -1 && errno == EINVAL,
          "strfmon_l '+' is EINVAL");
#endif

    /* The terminating null byte must fit too */
    check(strfmon(buf, 5, "%.2n", 1.5) == 4 && strcmp(buf, "1.50") == 0, "exact size");
    errno = 0;
    check(strfmon(buf, 4, "%.2n", 1.5) == -1 && errno == E2BIG, "one byte short");
    errno = 0;
    check(strfmon(buf, 3, "abc") == -1 && errno == E2BIG, "plain text too long");
    check(strfmon(buf, 4, "abc") == 3 && strcmp(buf, "abc") == 0, "plain text fits");
    errno = 0;
    check(strfmon(buf, 6, "%8.2n", 1.5) == -1 && errno == E2BIG, "field width too long");
    errno = 0;
    check(strfmon_l(buf, 4, l, "%.2n", 1.5) == -1 && errno == E2BIG, "strfmon_l E2BIG");

    /* The thread's locale is used */
    check(uselocale(l) == LC_GLOBAL_LOCALE, "uselocale");
    check(strfmon(buf, sizeof(buf), "%(.2n", -1.5) == 6 && strcmp(buf, "(1.50)") == 0,
          "with a thread locale");
    check(uselocale(LC_GLOBAL_LOCALE) == l, "uselocale");
    freelocale(l);

    printf("success\n");
    return 0;
}
