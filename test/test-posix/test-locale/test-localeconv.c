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
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* localeconv reports the POSIX locale's values (XBD 7.3.3.1 and 7.3.4.1) */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define empty(s) ((s) != NULL && *(s) == '\0')

static void
check_posix(const char *where)
{
    struct lconv *lc = localeconv();

    printf("%s\n", where);
    check(lc != NULL, "localeconv");

    /* LC_NUMERIC */
    check(lc->decimal_point != NULL && strcmp(lc->decimal_point, ".") == 0, "decimal_point");
    check(empty(lc->thousands_sep), "thousands_sep");
    check(empty(lc->grouping), "grouping");

    /* LC_MONETARY: nothing is available */
    check(empty(lc->int_curr_symbol), "int_curr_symbol");
    check(empty(lc->currency_symbol), "currency_symbol");
    check(empty(lc->mon_decimal_point), "mon_decimal_point");
    check(empty(lc->mon_thousands_sep), "mon_thousands_sep");
    check(empty(lc->mon_grouping), "mon_grouping");
    check(empty(lc->positive_sign), "positive_sign");
    check(empty(lc->negative_sign), "negative_sign");
    check(lc->int_frac_digits == CHAR_MAX, "int_frac_digits");
    check(lc->frac_digits == CHAR_MAX, "frac_digits");
    check(lc->p_cs_precedes == CHAR_MAX, "p_cs_precedes");
    check(lc->p_sep_by_space == CHAR_MAX, "p_sep_by_space");
    check(lc->n_cs_precedes == CHAR_MAX, "n_cs_precedes");
    check(lc->n_sep_by_space == CHAR_MAX, "n_sep_by_space");
    check(lc->p_sign_posn == CHAR_MAX, "p_sign_posn");
    check(lc->n_sign_posn == CHAR_MAX, "n_sign_posn");
    check(lc->int_p_cs_precedes == CHAR_MAX, "int_p_cs_precedes");
    check(lc->int_p_sep_by_space == CHAR_MAX, "int_p_sep_by_space");
    check(lc->int_n_cs_precedes == CHAR_MAX, "int_n_cs_precedes");
    check(lc->int_n_sep_by_space == CHAR_MAX, "int_n_sep_by_space");
    check(lc->int_p_sign_posn == CHAR_MAX, "int_p_sign_posn");
    check(lc->int_n_sign_posn == CHAR_MAX, "int_n_sign_posn");
}

int
main(void)
{
    locale_t l;

    /* At startup, after setlocale and with a thread locale */
    check_posix("startup");
    check(setlocale(LC_ALL, "POSIX") != NULL, "setlocale POSIX");
    check_posix("POSIX");
    check(setlocale(LC_ALL, "C") != NULL, "setlocale C");
    check_posix("C");
    l = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(l != (locale_t)0, "newlocale");
    check(uselocale(l) == LC_GLOBAL_LOCALE, "uselocale");
    check_posix("thread locale");
    check(uselocale(LC_GLOBAL_LOCALE) == l, "uselocale");
    freelocale(l);

    printf("success\n");
    return 0;
}
