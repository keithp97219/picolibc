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
 * The <wctype.h> _l functions: classes and mappings from wctype_l and
 * wctrans_l, (wctype_t)0 and (wctrans_t)0, the POSIX locale's classes,
 * and the functions without _l following the thread's locale
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define B(x) ((x) != 0)

static const char * const names[] = { "alnum", "alpha", "blank", "cntrl", "digit", "graph",
                                      "lower", "print", "punct", "space", "upper", "xdigit" };
#define NNAMES (sizeof(names) / sizeof(names[0]))

/* The <ctype.h> class of a portable character, in the same order as names */
static int
byte_class(int ch, size_t n)
{
    switch (n) {
    case 0:
        return B(isalnum(ch));
    case 1:
        return B(isalpha(ch));
    case 2:
        return B(isblank(ch));
    case 3:
        return B(iscntrl(ch));
    case 4:
        return B(isdigit(ch));
    case 5:
        return B(isgraph(ch));
    case 6:
        return B(islower(ch));
    case 7:
        return B(isprint(ch));
    case 8:
        return B(ispunct(ch));
    case 9:
        return B(isspace(ch));
    case 10:
        return B(isupper(ch));
    default:
        return B(isxdigit(ch));
    }
}

/* The isw*_l function for names[n] */
static int
wide_class_l(wint_t wc, size_t n, locale_t loc)
{
    switch (n) {
    case 0:
        return B(iswalnum_l(wc, loc));
    case 1:
        return B(iswalpha_l(wc, loc));
    case 2:
        return B(iswblank_l(wc, loc));
    case 3:
        return B(iswcntrl_l(wc, loc));
    case 4:
        return B(iswdigit_l(wc, loc));
    case 5:
        return B(iswgraph_l(wc, loc));
    case 6:
        return B(iswlower_l(wc, loc));
    case 7:
        return B(iswprint_l(wc, loc));
    case 8:
        return B(iswpunct_l(wc, loc));
    case 9:
        return B(iswspace_l(wc, loc));
    case 10:
        return B(iswupper_l(wc, loc));
    default:
        return B(iswxdigit_l(wc, loc));
    }
}

/* Wide characters outside the portable set to look at */
static const wint_t others[] = { 0x80, 0x9f, 0xa0, 0xc9, 0xe9, 0xff, 0x100, 0x3b1, 0x4e00, 0xff21 };
#define NOTHERS (sizeof(others) / sizeof(others[0]))

static void
check_posix(locale_t loc)
{
    wctype_t  types[NNAMES];
    wctrans_t lower, upper;
    wint_t    wc;
    size_t    n, i;

    for (n = 0; n < NNAMES; n++) {
        types[n] = wctype_l(names[n], loc);
        check(types[n] != 0, "wctype_l of a standard class");
        check(wctype(names[n]) != 0, "wctype of a standard class");
    }
    check(wctype_l("no-such-class", loc) == 0, "wctype_l of an unknown class");
    check(wctype("no-such-class") == 0, "wctype of an unknown class");

    lower = wctrans_l("tolower", loc);
    upper = wctrans_l("toupper", loc);
    check(lower != 0 && upper != 0 && wctrans("tolower") != 0, "wctrans_l");
    errno = 0;
    check(wctrans_l("no-such-mapping", loc) == 0, "wctrans_l of an unknown mapping");

    /* The portable characters have their <ctype.h> classes */
    for (wc = 0; wc < 0x80; wc++) {
        for (n = 0; n < NNAMES; n++) {
            int want = byte_class((int)wc, n);

            if (wide_class_l(wc, n, loc) != want || B(iswctype_l(wc, types[n], loc)) != want
                || B(iswctype(wc, types[n])) != want) {
                printf("wide char %#lx class %s\n", (unsigned long)wc, names[n]);
                exit(1);
            }
        }
        check(iswctype_l(wc, 0, loc) == 0 && iswctype(wc, 0) == 0, "(wctype_t)0");
        check(towlower_l(wc, loc) == (wint_t)tolower((int)wc)
                  && towupper_l(wc, loc) == (wint_t)toupper((int)wc),
              "towlower_l and towupper_l");
        check(towctrans_l(wc, lower, loc) == (wint_t)tolower((int)wc)
                  && towctrans_l(wc, upper, loc) == (wint_t)toupper((int)wc)
                  && towctrans(wc, upper) == (wint_t)toupper((int)wc),
              "towctrans_l");
        check(towctrans_l(wc, 0, loc) == wc && towctrans(wc, 0) == wc, "(wctrans_t)0");
    }

    /* Others may only be cntrl or punct, and don't change case */
    for (i = 0; i < NOTHERS; i++) {
        wc = others[i];
        for (n = 0; n < NNAMES; n++) {
            int got = wide_class_l(wc, n, loc);

            if (n == 3 || n == 8)
                continue;
            if ((n == 5 || n == 7) ? got != wide_class_l(wc, 8, loc) : got) {
                printf("wide char %#lx is %s in the POSIX locale\n", (unsigned long)wc, names[n]);
                exit(1);
            }
        }
        check(towlower_l(wc, loc) == wc && towupper_l(wc, loc) == wc, "no case outside ASCII");
    }

    /* WEOF is in no class and maps to itself */
    for (n = 0; n < NNAMES; n++)
        check(!wide_class_l(WEOF, n, loc) && !iswctype_l(WEOF, types[n], loc), "WEOF class");
    check(towlower_l(WEOF, loc) == WEOF && towupper_l(WEOF, loc) == WEOF, "WEOF case");
}

/*
 * gcc treats the <ctype.h> and <wctype.h> functions as builtins whose
 * result depends only on the argument, so it reuses results across
 * uselocale. Read the characters from volatile objects to stop that.
 */
static volatile wint_t e_acute = 0xe9;
static volatile wint_t E_acute = 0xc9;
static volatile wint_t small_alpha = 0x3b1;
static volatile wint_t cjk = 0x4e00;
static volatile wint_t ALPHA = 0x391;

int
main(void)
{
    int       (*p_iswctype_l)(wint_t, wctype_t, locale_t) = iswctype_l;
    wint_t    (*p_towctrans_l)(wint_t, wctrans_t, locale_t) = towctrans_l;
    wctrans_t (*p_wctrans_l)(const char *, locale_t) = wctrans_l;
    wctype_t  (*p_wctype_l)(const char *, locale_t) = wctype_l;
    locale_t  posix, utf8;

    check(p_iswctype_l && p_towctrans_l && p_wctrans_l && p_wctype_l, "prototypes");
    check(WEOF == WEOF && sizeof(wint_t) > 0, "WEOF and wint_t");

    posix = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(posix != (locale_t)0, "newlocale");
    check_posix(posix);
    check(uselocale(posix) == LC_GLOBAL_LOCALE, "uselocale");
    check_posix(posix);
    check(uselocale(LC_GLOBAL_LOCALE) == posix, "uselocale");

    utf8 = (locale_t)0;
#ifdef __PICOLIBC__
    /* picolibc built with multibyte support has C.UTF-8 */
    utf8 = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
#endif
    if (utf8 != (locale_t)0) {
        wctype_t  alpha = wctype_l("alpha", utf8);
        wctrans_t upper = wctrans_l("toupper", utf8);

        /* The _l functions follow their argument */
        check(iswalpha_l(e_acute, utf8) && iswlower_l(e_acute, utf8) && iswupper_l(E_acute, utf8),
              "C.UTF-8 letters");
        check(iswalpha_l(small_alpha, utf8) && iswctype_l(cjk, alpha, utf8), "more letters");
        check(towupper_l(e_acute, utf8) == E_acute && towlower_l(E_acute, utf8) == e_acute,
              "C.UTF-8 case");
        check(towctrans_l(small_alpha, upper, utf8) == ALPHA, "towctrans_l");
        check(!iswalpha_l(e_acute, posix) && !iswalpha(e_acute), "not in the POSIX locale");

        /* The functions without _l follow the thread's locale */
        check(uselocale(utf8) == LC_GLOBAL_LOCALE, "uselocale");
        check(iswalpha(e_acute) && towupper(e_acute) == E_acute, "thread C.UTF-8");
        check(iswctype(small_alpha, wctype("alpha"))
                  && towctrans(small_alpha, wctrans("toupper")) == ALPHA,
              "iswctype and towctrans");
        check(!iswalpha_l(e_acute, posix), "the _l functions still follow their argument");
        check(uselocale(LC_GLOBAL_LOCALE) == utf8, "uselocale");
        check(!iswalpha(e_acute) && towupper(e_acute) == e_acute, "back to the global locale");
        freelocale(utf8);
    } else {
        printf("no C.UTF-8 locale, skipping its checks\n");
    }

    freelocale(posix);
    printf("success\n");
    return 0;
}
