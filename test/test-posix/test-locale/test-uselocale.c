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
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/*
 * uselocale installs, reports and removes the calling thread's
 * locale; (locale_t)0 only asks. Functions without a locale argument
 * use the thread's locale, or the global one when there is none
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/*
 * gcc treats the <ctype.h> and <wctype.h> functions as builtins whose
 * result depends only on the argument, so it reuses results across
 * uselocale. Read the characters from volatile objects to stop that.
 */
#ifdef __PICOLIBC__
static volatile wint_t e_acute = 0xe9;
static volatile wint_t E_acute = 0xc9;
#endif

int
main(void)
{
    locale_t l, m;

    /* No thread locale at first */
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "starts with the global locale");
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "the query changes nothing");

    l = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    m = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(l != (locale_t)0 && m != (locale_t)0, "newlocale");

    /* Installing returns what was there before */
    check(uselocale(l) == LC_GLOBAL_LOCALE, "install");
    check(uselocale((locale_t)0) == l, "query the installed locale");
    check(uselocale((locale_t)0) == l, "querying leaves it installed");
    check(uselocale(m) == l, "replace");
    check(uselocale((locale_t)0) == m, "replaced");

    /* LC_GLOBAL_LOCALE uninstalls it */
    check(uselocale(LC_GLOBAL_LOCALE) == m, "uninstall");
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "uninstalled");
    check(uselocale(LC_GLOBAL_LOCALE) == LC_GLOBAL_LOCALE, "uninstall again");

    /* A duplicate of the global locale can be installed */
    freelocale(m);
    m = duplocale(LC_GLOBAL_LOCALE);
    check(m != (locale_t)0, "duplocale");
    check(uselocale(m) == LC_GLOBAL_LOCALE && uselocale((locale_t)0) == m, "install a copy");
    check(uselocale(LC_GLOBAL_LOCALE) == m, "uninstall the copy");
    freelocale(m);

#ifdef __PICOLIBC__
    /*
     * picolibc's C.UTF-8 differs from C only in LC_CTYPE, so check
     * functions that depend on LC_CTYPE
     */
    m = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (m != (locale_t)0) {
        static const char utf8_e_acute[] = "\xc3\xa9";
        mbstate_t         state;
        wchar_t           wc;

        check(setlocale(LC_ALL, "C") != NULL, "global C");
        check(MB_CUR_MAX == 1 && !iswalpha(e_acute), "C");

        /* The thread's locale overrides the global one */
        check(uselocale(m) == LC_GLOBAL_LOCALE, "install C.UTF-8");
        check(MB_CUR_MAX > 1, "MB_CUR_MAX follows the thread's locale");
        memset(&state, 0, sizeof(state));
        check(mbrtowc(&wc, utf8_e_acute, 2, &state) == 2 && wc == (wchar_t)e_acute,
              "mbrtowc follows it");
        check(iswalpha(e_acute) && towupper(e_acute) == E_acute, "wctype follows it");
        check(strcmp(nl_langinfo(CODESET), "UTF-8") == 0, "nl_langinfo follows it");

        /* setlocale doesn't affect a thread with its own locale */
        check(uselocale(l) == m, "install C");
        check(setlocale(LC_CTYPE, "C.UTF-8") != NULL, "global C.UTF-8");
        check(MB_CUR_MAX == 1 && !iswalpha(e_acute), "the thread's C locale stays");
        check(strcmp(nl_langinfo(CODESET), "UTF-8") != 0, "nl_langinfo stays");

        /* Without a thread locale, the global one is used */
        check(uselocale(LC_GLOBAL_LOCALE) == l, "uninstall");
        check(MB_CUR_MAX > 1 && iswalpha(e_acute), "the global C.UTF-8 is used");
        check(setlocale(LC_ALL, "C") != NULL, "global C");
        check(MB_CUR_MAX == 1, "and follows setlocale");
        freelocale(m);
    } else {
        printf("no C.UTF-8 locale, skipping its checks\n");
    }
#endif
    freelocale(l);

    printf("success\n");
    return 0;
}
