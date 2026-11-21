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
#include <wctype.h>

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * <wctype.h> without the _l forms: the types and prototypes, the
 * classes and case mappings of the POSIX locale, wctype and iswctype,
 * wctrans and towctrans, and WEOF. The UTF-8 checks run where a
 * C.UTF-8 locale exists.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static wint_t    wint __attribute__((unused));
static wctrans_t trans __attribute__((unused));
static wctype_t  type __attribute__((unused));
static locale_t  locale __attribute__((unused));
_Static_assert(_Generic(WEOF, wint_t: 1, default: 0), "WEOF is a wint_t");

#define proto(name, type) _Static_assert(_Generic((name), type: 1, default: 0), #name)
proto(iswalnum, int (*)(wint_t));
proto(iswalpha, int (*)(wint_t));
proto(iswblank, int (*)(wint_t));
proto(iswcntrl, int (*)(wint_t));
proto(iswctype, int (*)(wint_t, wctype_t));
proto(iswdigit, int (*)(wint_t));
proto(iswgraph, int (*)(wint_t));
proto(iswlower, int (*)(wint_t));
proto(iswprint, int (*)(wint_t));
proto(iswpunct, int (*)(wint_t));
proto(iswspace, int (*)(wint_t));
proto(iswupper, int (*)(wint_t));
proto(iswxdigit, int (*)(wint_t));
proto(towctrans, wint_t (*)(wint_t, wctrans_t));
proto(towlower, wint_t (*)(wint_t));
proto(towupper, wint_t (*)(wint_t));
proto(wctrans, wctrans_t (*)(const char *));
proto(wctype, wctype_t (*)(const char *));

static const struct {
    const char *name;
    int         (*wfunc)(wint_t);
    int         (*func)(int);
} classes[] = {
    { "alnum",  iswalnum,  isalnum  },
    { "alpha",  iswalpha,  isalpha  },
    { "blank",  iswblank,  isblank  },
    { "cntrl",  iswcntrl,  iscntrl  },
    { "digit",  iswdigit,  isdigit  },
    { "graph",  iswgraph,  isgraph  },
    { "lower",  iswlower,  islower  },
    { "print",  iswprint,  isprint  },
    { "punct",  iswpunct,  ispunct  },
    { "space",  iswspace,  isspace  },
    { "upper",  iswupper,  isupper  },
    { "xdigit", iswxdigit, isxdigit },
};

#define NCLASSES (sizeof(classes) / sizeof(classes[0]))

/* Through volatile objects, so the compiler can't fold the calls across setlocale */
static volatile wint_t e_acute = 0xe9;
static volatile wint_t arabic_one = 0x661;

static void
common(void)
{
    wctrans_t lower = wctrans("tolower"), upper = wctrans("toupper");
    unsigned  i;
    wint_t    c;

    check(lower != 0 && upper != 0 && lower != upper, "wctrans names");
    check(wctrans("bogus") == 0, "wctrans of an unknown name");
    check(wctype("bogus") == 0, "wctype of an unknown name");
    /* Those may set errno; nothing after them should */
    errno = EDOM;

    for (i = 0; i < NCLASSES; i++) {
        wctype_t t = wctype(classes[i].name);

        check(t != 0, "wctype of a standard class");
        /* WEOF has no class */
        check(classes[i].wfunc(WEOF) == 0, "WEOF has no class");
        check(iswctype(WEOF, t) == 0, "iswctype(WEOF)");
        for (c = 0; c < 0x80; c++) {
            /* The portable characters have the same classes as in <ctype.h> */
            check(!classes[i].wfunc(c) == !classes[i].func((int)c),
                  "class of a portable character");
            check(!iswctype(c, t) == !classes[i].wfunc(c), "iswctype matches the function");
            check(iswctype(c, 0) == 0, "iswctype with charclass 0");
        }
    }

    for (c = 0; c < 0x80; c++) {
        check(towlower(c) == (wint_t)tolower((int)c), "towlower of a portable character");
        check(towupper(c) == (wint_t)toupper((int)c), "towupper of a portable character");
        check(towctrans(c, lower) == towlower(c), "towctrans tolower");
        check(towctrans(c, upper) == towupper(c), "towctrans toupper");
        check(towctrans(c, 0) == c, "towctrans with desc 0");
    }
    check(towlower(WEOF) == WEOF && towupper(WEOF) == WEOF, "the mappings keep WEOF");

    /* Only the ten digits are digit, and only the 22 hex digits xdigit */
    check(!iswdigit(arabic_one) && !iswxdigit(arabic_one), "digit and xdigit are ASCII only");
}

int
main(void)
{
    unsigned i;

    errno = EDOM;
    common();

    /*
     * Outside the portable characters, the POSIX locale may classify
     * characters only as cntrl or punct (XBD 6.2)
     */
    for (i = 0; i < NCLASSES; i++)
        if (classes[i].wfunc != iswcntrl && classes[i].wfunc != iswpunct)
            check(classes[i].wfunc(e_acute) == 0, "U+00E9 has no class in the POSIX locale");
    check(towupper(e_acute) == e_acute && towlower(0xc9) == 0xc9,
          "no case mapping outside the portable characters");
    check(errno == EDOM, "errno after the C locale checks");

    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        common();
        check(iswalpha(e_acute) && iswlower(e_acute) && !iswupper(e_acute), "U+00E9 is lower");
        check(towupper(e_acute) == 0xc9 && towlower(0xc9) == e_acute, "U+00E9 case");
        check(towctrans(e_acute, wctrans("toupper")) == 0xc9, "towctrans in UTF-8");
        check(iswctype(e_acute, wctype("alpha")) != 0, "iswctype in UTF-8");
        check(iswspace(0x3000) && !iswblank(L'\n'), "iswspace of U+3000");
        check(iswpunct(0x00bf) && iswprint(0x00bf) && !iswalnum(0x00bf), "U+00BF");
        check(errno == EDOM, "errno after the UTF-8 checks");
        check(setlocale(LC_CTYPE, "C") != NULL, "back to the C locale");
    } else {
        printf("no C.UTF-8 locale, skipping the UTF-8 checks\n");
    }

    printf("success\n");
    return 0;
}
