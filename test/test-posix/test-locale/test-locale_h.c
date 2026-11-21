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
#define category +
#define locale   +
#define base     +
#define mask     +
#include <locale.h>
#undef category
#undef locale
#undef base
#undef mask

#include <stdio.h>
#include <stdlib.h>

/* <locale.h> defines its types, macros and prototypes by itself */

#ifndef NULL
#error NULL
#endif

/* The categories are macros with distinct values */
#if !defined(LC_ALL) || !defined(LC_COLLATE) || !defined(LC_CTYPE) || !defined(LC_MESSAGES) \
    || !defined(LC_MONETARY) || !defined(LC_NUMERIC) || !defined(LC_TIME)
#error LC_ category macros
#endif

#if !defined(LC_COLLATE_MASK) || !defined(LC_CTYPE_MASK) || !defined(LC_MESSAGES_MASK)   \
    || !defined(LC_MONETARY_MASK) || !defined(LC_NUMERIC_MASK) || !defined(LC_TIME_MASK) \
    || !defined(LC_ALL_MASK) || !defined(LC_GLOBAL_LOCALE)
#error LC_ mask macros
#endif

static const int categories[]
    = { LC_ALL, LC_COLLATE, LC_CTYPE, LC_MESSAGES, LC_MONETARY, LC_NUMERIC, LC_TIME };

static const int masks[] = { LC_COLLATE_MASK,  LC_CTYPE_MASK,   LC_MESSAGES_MASK,
                             LC_MONETARY_MASK, LC_NUMERIC_MASK, LC_TIME_MASK };

#define NCAT  (sizeof(categories) / sizeof(categories[0]))
#define NMASK (sizeof(masks) / sizeof(masks[0]))

/* The integer constant expressions can size arrays */
static char category_constant[LC_TIME + LC_ALL + 1] __attribute__((unused));
static char mask_constant[LC_ALL_MASK] __attribute__((unused));

/* struct lconv has its members with their types */
#define is_type(e, t) _Generic((e), t: 1, default: 0)

static struct lconv lc __attribute__((unused));

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    size_t        i, j;

    /* Prototypes */
    char         *(*p_setlocale)(int, const char *) = setlocale;
    struct lconv *(*p_localeconv)(void) = localeconv;
    locale_t      (*p_newlocale)(int, const char *, locale_t) = newlocale;
    locale_t      (*p_duplocale)(locale_t) = duplocale;
    void          (*p_freelocale)(locale_t) = freelocale;
    locale_t      (*p_uselocale)(locale_t) = uselocale;
#ifndef __GLIBC__
    /* glibc 2.43 has no getlocalename_l */
    const char *(*p_getlocalename_l)(int, locale_t) = getlocalename_l;
    check(p_getlocalename_l != NULL, "getlocalename_l");
#endif

    check(p_setlocale && p_localeconv && p_newlocale && p_duplocale && p_freelocale && p_uselocale,
          "prototypes");

    for (i = 0; i < NCAT; i++)
        for (j = i + 1; j < NCAT; j++)
            check(categories[i] != categories[j], "distinct categories");

    /* Each mask is a distinct bit and LC_ALL_MASK has them all */
    for (i = 0; i < NMASK; i++) {
        check(masks[i] != 0 && (masks[i] & (masks[i] - 1)) == 0, "one bit per mask");
        check((LC_ALL_MASK & masks[i]) == masks[i], "LC_ALL_MASK has every mask");
        for (j = i + 1; j < NMASK; j++)
            check(masks[i] != masks[j], "distinct masks");
    }

    check(is_type(LC_GLOBAL_LOCALE, locale_t), "LC_GLOBAL_LOCALE is a locale_t");
    check(LC_GLOBAL_LOCALE != (locale_t)0, "LC_GLOBAL_LOCALE isn't (locale_t)0");

    check(is_type(lc.currency_symbol, char *), "currency_symbol");
    check(is_type(lc.decimal_point, char *), "decimal_point");
    check(is_type(lc.frac_digits, char), "frac_digits");
    check(is_type(lc.grouping, char *), "grouping");
    check(is_type(lc.int_curr_symbol, char *), "int_curr_symbol");
    check(is_type(lc.int_frac_digits, char), "int_frac_digits");
    check(is_type(lc.int_n_cs_precedes, char), "int_n_cs_precedes");
    check(is_type(lc.int_n_sep_by_space, char), "int_n_sep_by_space");
    check(is_type(lc.int_n_sign_posn, char), "int_n_sign_posn");
    check(is_type(lc.int_p_cs_precedes, char), "int_p_cs_precedes");
    check(is_type(lc.int_p_sep_by_space, char), "int_p_sep_by_space");
    check(is_type(lc.int_p_sign_posn, char), "int_p_sign_posn");
    check(is_type(lc.mon_decimal_point, char *), "mon_decimal_point");
    check(is_type(lc.mon_grouping, char *), "mon_grouping");
    check(is_type(lc.mon_thousands_sep, char *), "mon_thousands_sep");
    check(is_type(lc.negative_sign, char *), "negative_sign");
    check(is_type(lc.n_cs_precedes, char), "n_cs_precedes");
    check(is_type(lc.n_sep_by_space, char), "n_sep_by_space");
    check(is_type(lc.n_sign_posn, char), "n_sign_posn");
    check(is_type(lc.positive_sign, char *), "positive_sign");
    check(is_type(lc.p_cs_precedes, char), "p_cs_precedes");
    check(is_type(lc.p_sep_by_space, char), "p_sep_by_space");
    check(is_type(lc.p_sign_posn, char), "p_sign_posn");
    check(is_type(lc.thousands_sep, char *), "thousands_sep");

    printf("success\n");
    return 0;
}
