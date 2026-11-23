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

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * uselocale((locale_t)0) only asks; newlocale's errors, mask and
 * base; "" takes the locale from LC_ALL, LC_* and LANG
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* A name no C library has: an unknown codeset */
#define BOGUS "xx_XX.NO-SUCH-CODESET"

/*
 * Point environ at an array of our own, so the test doesn't depend on
 * setenv and unsetenv
 */
extern char **environ;

static char   env_strings[3][64];
static char  *env[4];

static void
set_env(const char *lc_all, const char *lc_ctype, const char *lang)
{
    int n = 0;

    if (lc_all) {
        snprintf(env_strings[n], sizeof(env_strings[n]), "LC_ALL=%s", lc_all);
        env[n] = env_strings[n];
        n++;
    }
    if (lc_ctype) {
        snprintf(env_strings[n], sizeof(env_strings[n]), "LC_CTYPE=%s", lc_ctype);
        env[n] = env_strings[n];
        n++;
    }
    if (lang) {
        snprintf(env_strings[n], sizeof(env_strings[n]), "LANG=%s", lang);
        env[n] = env_strings[n];
        n++;
    }
    env[n] = NULL;
    environ = env;
}

int
main(void)
{
    locale_t l, m;

    /* uselocale((locale_t)0) leaves the thread's locale installed */
    l = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(l != (locale_t)0, "newlocale C");
    check(uselocale(l) == LC_GLOBAL_LOCALE, "install");
    check(uselocale((locale_t)0) == l, "query the installed locale");
    check(uselocale((locale_t)0) == l, "querying leaves it installed");
    check(uselocale(LC_GLOBAL_LOCALE) == l, "uninstall");
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "uninstalled");
    freelocale(l);

    /* A locale that doesn't exist is ENOENT */
    errno = 0;
    check(newlocale(LC_ALL_MASK, BOGUS, (locale_t)0) == (locale_t)0 && errno == ENOENT,
          "unknown locale is ENOENT");

    /* "" takes the locale from the environment */
    set_env(NULL, NULL, NULL);
    check(setlocale(LC_ALL, "") != NULL, "setlocale \"\" without variables");
    set_env(NULL, NULL, BOGUS);
    check(setlocale(LC_ALL, "") == NULL, "setlocale \"\" with LANG unknown");
    errno = 0;
    check(newlocale(LC_ALL_MASK, "", (locale_t)0) == (locale_t)0 && errno == ENOENT,
          "newlocale \"\" with LANG unknown");
    set_env("C", NULL, BOGUS);
    check(setlocale(LC_ALL, "") != NULL, "LC_ALL over LANG");
    set_env("", NULL, BOGUS);
    check(setlocale(LC_ALL, "") == NULL, "empty LC_ALL ignored");
    set_env(NULL, NULL, NULL);

#ifdef __PICOLIBC__
    /* With multibyte support, picolibc's C.UTF-8 differs from C in LC_CTYPE */
    m = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (m != (locale_t)0) {
        locale_t r;

        check(strcmp(nl_langinfo_l(CODESET, m), "UTF-8") == 0, "LC_CTYPE_MASK sets LC_CTYPE");

        /* Categories outside the mask come from the POSIX locale ... */
        r = newlocale(LC_COLLATE_MASK, "C.UTF-8", (locale_t)0);
        check(r != (locale_t)0 && strcmp(nl_langinfo_l(CODESET, r), "ASCII") == 0,
              "other masks leave LC_CTYPE");
        freelocale(r);

        /* ... or from base */
        r = newlocale(LC_NUMERIC_MASK, "C", m);
        check(r != (locale_t)0 && strcmp(nl_langinfo_l(CODESET, r), "UTF-8") == 0,
              "base keeps LC_CTYPE");
        freelocale(r);

        /* LC_CTYPE from the environment */
        set_env(NULL, "C.UTF-8", "C");
        check(setlocale(LC_ALL, "") != NULL, "setlocale LC_CTYPE=C.UTF-8");
        check(strcmp(nl_langinfo(CODESET), "UTF-8") == 0, "LC_CTYPE from the environment");
        r = newlocale(LC_ALL_MASK, "", (locale_t)0);
        check(r != (locale_t)0 && strcmp(nl_langinfo_l(CODESET, r), "UTF-8") == 0,
              "newlocale LC_CTYPE from the environment");
        freelocale(r);
        check(setlocale(LC_ALL, "C") != NULL, "back to C");
        set_env(NULL, NULL, NULL);
    } else {
        printf("no C.UTF-8 locale, skipping its checks\n");
    }
#else
    (void)m;
#endif

    printf("success\n");
    return 0;
}
