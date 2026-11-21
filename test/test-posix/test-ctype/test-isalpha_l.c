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
/* The application's own macros, defined before the header */
#define c +
#define l +
#include <ctype.h>
#undef c
#undef l

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The <ctype.h> _l functions classify and convert by the locale they
 * are given, as macros and as functions; in the POSIX locale the
 * classes are those of XBD 7.3.1.1, with only cntrl and punct allowed
 * outside the portable characters; the functions without _l follow
 * the thread's locale
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define B(x) ((x) != 0)

/* The POSIX locale's classes for the portable characters (0..127) */
static int
posix_class(int ch, char cls)
{
    switch (cls) {
    case 'u':
        return 'A' <= ch && ch <= 'Z';
    case 'l':
        return 'a' <= ch && ch <= 'z';
    case 'a':
        return posix_class(ch, 'u') || posix_class(ch, 'l');
    case 'd':
        return '0' <= ch && ch <= '9';
    case 'n':
        return posix_class(ch, 'a') || posix_class(ch, 'd');
    case 'x':
        return posix_class(ch, 'd') || ('A' <= ch && ch <= 'F') || ('a' <= ch && ch <= 'f');
    case 's':
        return ch == ' ' || ('\t' <= ch && ch <= '\r');
    case 'b':
        return ch == ' ' || ch == '\t';
    case 'c':
        return (0 <= ch && ch < 0x20) || ch == 0x7f;
    case 'p':
        return 0x21 <= ch && ch <= 0x7e && !posix_class(ch, 'n');
    case 'g':
        return 0x21 <= ch && ch <= 0x7e;
    case 'r':
        return 0x20 <= ch && ch <= 0x7e;
    }
    return 0;
}

/* Classify ch in locale l using macros (m) or functions */
static int
class_l(int ch, char cls, locale_t loc, int m)
{
    switch (cls) {
    case 'u':
        return B(m ? isupper_l(ch, loc) : (isupper_l)(ch, loc));
    case 'l':
        return B(m ? islower_l(ch, loc) : (islower_l)(ch, loc));
    case 'a':
        return B(m ? isalpha_l(ch, loc) : (isalpha_l)(ch, loc));
    case 'd':
        return B(m ? isdigit_l(ch, loc) : (isdigit_l)(ch, loc));
    case 'n':
        return B(m ? isalnum_l(ch, loc) : (isalnum_l)(ch, loc));
    case 'x':
        return B(m ? isxdigit_l(ch, loc) : (isxdigit_l)(ch, loc));
    case 's':
        return B(m ? isspace_l(ch, loc) : (isspace_l)(ch, loc));
    case 'b':
        return B(m ? isblank_l(ch, loc) : (isblank_l)(ch, loc));
    case 'c':
        return B(m ? iscntrl_l(ch, loc) : (iscntrl_l)(ch, loc));
    case 'p':
        return B(m ? ispunct_l(ch, loc) : (ispunct_l)(ch, loc));
    case 'g':
        return B(m ? isgraph_l(ch, loc) : (isgraph_l)(ch, loc));
    case 'r':
        return B(m ? isprint_l(ch, loc) : (isprint_l)(ch, loc));
    }
    return 0;
}

/* The same without _l, from the current locale */
static int
class_cur(int ch, char cls, int m)
{
    switch (cls) {
    case 'u':
        return B(m ? isupper(ch) : (isupper)(ch));
    case 'l':
        return B(m ? islower(ch) : (islower)(ch));
    case 'a':
        return B(m ? isalpha(ch) : (isalpha)(ch));
    case 'd':
        return B(m ? isdigit(ch) : (isdigit)(ch));
    case 'n':
        return B(m ? isalnum(ch) : (isalnum)(ch));
    case 'x':
        return B(m ? isxdigit(ch) : (isxdigit)(ch));
    case 's':
        return B(m ? isspace(ch) : (isspace)(ch));
    case 'b':
        return B(m ? isblank(ch) : (isblank)(ch));
    case 'c':
        return B(m ? iscntrl(ch) : (iscntrl)(ch));
    case 'p':
        return B(m ? ispunct(ch) : (ispunct)(ch));
    case 'g':
        return B(m ? isgraph(ch) : (isgraph)(ch));
    case 'r':
        return B(m ? isprint(ch) : (isprint)(ch));
    }
    return 0;
}

static const char classes[] = "ulandxsbcpgr";

/* Check every character and EOF against the POSIX locale */
static void
check_posix(locale_t loc, const char *where)
{
    int         ch, m;
    const char *cls;

    printf("%s\n", where);
    for (ch = EOF; ch <= 255; ch++) {
        for (m = 0; m <= 1; m++) {
            for (cls = classes; *cls; cls++) {
                int got = class_l(ch, *cls, loc, m);
                int cur = class_cur(ch, *cls, m);
                int want;

                if (ch == EOF)
                    want = 0;
                else if (ch < 0x80)
                    want = posix_class(ch, *cls);
                else if (*cls == 'c' || *cls == 'p')
                    /* Allowed to be cntrl or punct, so just agree */
                    want = got;
                else if (*cls == 'g' || *cls == 'r')
                    want = class_l(ch, 'p', loc, m);
                else
                    want = 0;
                if (got != want || cur != want) {
                    printf("char %d class %c: _l %d, current %d, want %d (%s)\n", ch, *cls, got,
                           cur, want, m ? "macro" : "function");
                    exit(1);
                }
            }
            /* Only the portable letters change case */
            {
                int up = ('a' <= ch && ch <= 'z') ? ch - 'a' + 'A' : ch;
                int down = ('A' <= ch && ch <= 'Z') ? ch - 'A' + 'a' : ch;

                if ((m ? toupper_l(ch, loc) : (toupper_l)(ch, loc)) != up
                    || (m ? tolower_l(ch, loc) : (tolower_l)(ch, loc)) != down
                    || (m ? toupper(ch) : (toupper)(ch)) != up
                    || (m ? tolower(ch) : (tolower)(ch)) != down) {
                    printf("char %d: case conversion\n", ch);
                    exit(1);
                }
            }
        }
    }
}

/*
 * gcc treats the <ctype.h> and <wctype.h> functions as builtins whose
 * result depends only on the argument, so it reuses results across
 * uselocale. Read the characters from volatile objects to stop that.
 */
static volatile int e_acute = 0xe9;
static volatile int E_acute = 0xc9;

int
main(void)
{
    int      (*p_isalnum_l)(int, locale_t) = isalnum_l;
    int      (*p_toupper_l)(int, locale_t) = toupper_l;
    locale_t posix, iso;

    check(p_isalnum_l != NULL && p_toupper_l != NULL, "prototypes");

    posix = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(posix != (locale_t)0, "newlocale");
    check_posix(posix, "POSIX locale object");
    check(uselocale(posix) == LC_GLOBAL_LOCALE, "uselocale");
    check_posix(posix, "as the thread's locale");
    check(uselocale(LC_GLOBAL_LOCALE) == posix, "uselocale");

    iso = (locale_t)0;
#ifdef __PICOLIBC__
    /* picolibc built with extended charsets has ISO-8859-1 */
    iso = newlocale(LC_CTYPE_MASK, "C.ISO-8859-1", (locale_t)0);
#endif
    if (iso != (locale_t)0) {
        /* The _l functions follow their argument, not the current locale */
        check(isalpha_l(e_acute, iso) && islower_l(e_acute, iso) && isupper_l(E_acute, iso),
              "ISO-8859-1 letters");
        check((isalpha_l)(e_acute, iso) && (isupper_l)(E_acute, iso), "ISO-8859-1 functions");
        check(toupper_l(e_acute, iso) == E_acute && tolower_l(E_acute, iso) == e_acute,
              "ISO-8859-1 case");
        check(!isalpha_l(e_acute, posix) && toupper_l(e_acute, posix) == e_acute,
              "POSIX locale object");
        check(!isalpha(e_acute) && toupper(e_acute) == e_acute, "global POSIX locale");

        /* The functions without _l follow the thread's locale */
        check(uselocale(iso) == LC_GLOBAL_LOCALE, "uselocale");
        check(isalpha(e_acute) && (isalpha)(e_acute) && toupper(e_acute) == E_acute,
              "thread ISO-8859-1");
        check(!isalpha_l(e_acute, posix), "the _l functions still follow their argument");
        check(uselocale(LC_GLOBAL_LOCALE) == iso, "uselocale");
        check(!isalpha(e_acute), "back to the global locale");
        freelocale(iso);
    } else {
        printf("no ISO-8859-1 locale, skipping its checks\n");
    }

    freelocale(posix);
    printf("success\n");
    return 0;
}
