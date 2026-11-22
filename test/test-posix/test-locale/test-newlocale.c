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
#include "../test-flags.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TEST_POSIX_MULTI_PROCESS
#include <sys/wait.h>
#endif

/*
 * newlocale, duplocale, freelocale and getlocalename_l: their errors,
 * the base argument, LC_GLOBAL_LOCALE and "" from the environment
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

static const int masks[] = { LC_COLLATE_MASK,  LC_CTYPE_MASK,   LC_MESSAGES_MASK,
                             LC_MONETARY_MASK, LC_NUMERIC_MASK, LC_TIME_MASK };
#define NMASK (sizeof(masks) / sizeof(masks[0]))

/* The locale object works: it classifies and can be duplicated */
static void
check_usable(locale_t l, const char *message)
{
    locale_t d;

    check(isalpha_l('a', l) && !isalpha_l('1', l), message);
    d = duplocale(l);
    check(d != (locale_t)0, message);
    freelocale(d);
}

/* getlocalename_l is new in POSIX.1-2024; glibc 2.43 lacks it */
#if TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
static int
is_posix(const char *name)
{
    return name != NULL && (strcmp(name, "C") == 0 || strcmp(name, "POSIX") == 0);
}

static int
name_is(int category, locale_t l, const char *name)
{
    const char *got = getlocalename_l(category, l);

    return got != NULL && strcmp(got, name) == 0;
}
#endif

#if TEST_POSIX_MULTI_PROCESS

static const char * const env_names[] = { "LC_ALL",      "LC_COLLATE", "LC_CTYPE", "LC_MESSAGES",
                                          "LC_MONETARY", "LC_NUMERIC", "LC_TIME",  "LANG" };
#define NENV (sizeof(env_names) / sizeof(env_names[0]))

/*
 * In a child with LC_ALL, LC_CTYPE and LANG set to the given values
 * (NULL for unset), check newlocale(mask, "", 0): it succeeds, or
 * fails with ENOENT
 */
static void
check_env(const char *lc_all, const char *lc_ctype, const char *lang, int mask, int expect,
          const char *message)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        locale_t l;
        size_t   i;

        for (i = 0; i < NENV; i++)
            check(unsetenv(env_names[i]) == 0, "unsetenv");
        if (lc_all)
            check(setenv("LC_ALL", lc_all, 1) == 0, "setenv");
        if (lc_ctype)
            check(setenv("LC_CTYPE", lc_ctype, 1) == 0, "setenv");
        if (lang)
            check(setenv("LANG", lang, 1) == 0, "setenv");
        errno = 0;
        l = newlocale(mask, "", (locale_t)0);
        if (expect && l == (locale_t)0) {
            printf("%s: newlocale failed, errno %d\n", message, errno);
            _exit(1);
        }
        if (!expect && (l != (locale_t)0 || errno != ENOENT)) {
            printf("%s: newlocale didn't fail with ENOENT, errno %d\n", message, errno);
            _exit(1);
        }
#if defined(__PICOLIBC__) && TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
        /* With C.UTF-8, the environment chooses LC_CTYPE */
        if (l != (locale_t)0 && lc_ctype && strcmp(lc_ctype, "C.UTF-8") == 0 && !lc_all
            && (mask & LC_CTYPE_MASK) && !name_is(LC_CTYPE, l, "C.UTF-8")) {
            printf("%s: LC_CTYPE is %s\n", message, getlocalename_l(LC_CTYPE, l));
            _exit(1);
        }
#endif
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("%s: child failed\n", message);
        exit(1);
    }
}

static void
check_environment(void)
{
    check_env(NULL, NULL, NULL, LC_ALL_MASK, 1, "no variables");
    check_env(NULL, NULL, "C", LC_ALL_MASK, 1, "LANG=C");
    check_env(NULL, NULL, BOGUS, LC_ALL_MASK, 0, "LANG unknown");
    check_env(NULL, NULL, BOGUS, LC_NUMERIC_MASK, 0, "LANG unknown, LC_NUMERIC");
    check_env("C", NULL, BOGUS, LC_ALL_MASK, 1, "LC_ALL over LANG");
    check_env(BOGUS, "C", "C", LC_CTYPE_MASK, 0, "LC_ALL unknown");
    check_env(NULL, "C", BOGUS, LC_CTYPE_MASK, 1, "LC_CTYPE over LANG");
    check_env(NULL, "C", BOGUS, LC_CTYPE_MASK | LC_TIME_MASK, 0, "LANG for LC_TIME");
    check_env("", "", "C", LC_ALL_MASK, 1, "empty values ignored");
    check_env("", NULL, BOGUS, LC_ALL_MASK, 0, "empty LC_ALL ignored");
}

#endif /* TEST_POSIX_MULTI_PROCESS */

int
main(void)
{
    locale_t l, b, r, g;
    size_t   i;
    int      bad_bit = 0;

    /* "C" and "POSIX" for all categories and for each one */
    l = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(l != (locale_t)0, "newlocale C");
    check_usable(l, "C object");
#if TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
    check(is_posix(getlocalename_l(LC_CTYPE, l)), "name of C");
    check(is_posix(getlocalename_l(LC_ALL, l)), "LC_ALL name of C");
    check(getlocalename_l(LC_ALL + LC_COLLATE + LC_CTYPE + LC_MESSAGES + LC_MONETARY + LC_NUMERIC
                              + LC_TIME + 1000,
                          l)
              == NULL,
          "invalid category");
#endif
    errno = 1234;
    freelocale(l);
    check(errno == 1234, "freelocale leaves errno alone");

    l = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(l != (locale_t)0, "newlocale POSIX");
    freelocale(l);
    for (i = 0; i < NMASK; i++) {
        l = newlocale(masks[i], "C", (locale_t)0);
        check(l != (locale_t)0, "newlocale one category");
        check_usable(l, "one category object");
        freelocale(l);
    }

    /* A mask bit that isn't a category */
    while (bad_bit < 30 && (LC_ALL_MASK & (1 << bad_bit)))
        bad_bit++;
    errno = 0;
    check(newlocale(LC_CTYPE_MASK | (1 << bad_bit), "C", (locale_t)0) == (locale_t)0
              && errno == EINVAL,
          "invalid mask bit");

    /* A locale that doesn't exist */
    errno = 0;
    check(newlocale(LC_ALL_MASK, BOGUS, (locale_t)0) == (locale_t)0 && errno == ENOENT,
          "unknown locale is ENOENT");
    errno = 0;
    check(newlocale(LC_NUMERIC_MASK, BOGUS, (locale_t)0) == (locale_t)0 && errno == ENOENT,
          "unknown locale for one category");

    /* A failure leaves base alone; a success may replace it */
    b = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(b != (locale_t)0, "base");
    check(newlocale(LC_CTYPE_MASK, BOGUS, b) == (locale_t)0, "failure with base");
    check_usable(b, "base after failure");
    r = newlocale(LC_NUMERIC_MASK, "POSIX", b);
    check(r != (locale_t)0, "modify base");
    check_usable(r, "modified base");
#if TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
    check(is_posix(getlocalename_l(LC_NUMERIC, r)), "modified category");
#endif
    freelocale(r);

    /* duplocale of LC_GLOBAL_LOCALE copies the global locale */
    g = duplocale(LC_GLOBAL_LOCALE);
    check(g != (locale_t)0, "duplocale(LC_GLOBAL_LOCALE)");
    check_usable(g, "copy of the global locale");
#if TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
    check(name_is(LC_CTYPE, g, setlocale(LC_CTYPE, NULL)), "copy has the global LC_CTYPE");
    check(name_is(LC_ALL, LC_GLOBAL_LOCALE, setlocale(LC_ALL, NULL)),
          "getlocalename_l of LC_GLOBAL_LOCALE");
    check(setlocale(LC_ALL, getlocalename_l(LC_ALL, g)) != NULL, "its LC_ALL name restores");
#endif
    freelocale(g);

#if defined(__PICOLIBC__) && TEST_POSIX_MULTI_CONCURRENT_LOCALES >= 202405L
    /* With multibyte support, picolibc has C.UTF-8, whose LC_CTYPE differs */
    l = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (l != (locale_t)0) {
        check(name_is(LC_CTYPE, l, "C.UTF-8"), "LC_CTYPE_MASK sets LC_CTYPE");

        /* Categories outside the mask come from the POSIX locale */
        r = newlocale(LC_COLLATE_MASK, "C.UTF-8", (locale_t)0);
        check(r != (locale_t)0 && name_is(LC_CTYPE, r, "C"), "other masks leave LC_CTYPE");
        freelocale(r);

        /* ... or from base */
        r = newlocale(LC_NUMERIC_MASK, "C", l);
        check(r != (locale_t)0 && name_is(LC_CTYPE, r, "C.UTF-8"), "base keeps LC_CTYPE");
        l = newlocale(LC_CTYPE_MASK, "C", r);
        check(l != (locale_t)0 && name_is(LC_CTYPE, l, "C"), "base's LC_CTYPE replaced");
        freelocale(l);

        /* duplocale(LC_GLOBAL_LOCALE) sees setlocale */
        check(setlocale(LC_CTYPE, "C.UTF-8") != NULL, "setlocale C.UTF-8");
        g = duplocale(LC_GLOBAL_LOCALE);
        check(name_is(LC_CTYPE, g, "C.UTF-8"), "global copy has C.UTF-8");
        check(setlocale(LC_ALL, "C") != NULL, "setlocale C");
        check(name_is(LC_CTYPE, g, "C.UTF-8"), "the copy doesn't follow setlocale");
        freelocale(g);
    } else {
        printf("no C.UTF-8 locale, skipping its checks\n");
    }
#endif

#if TEST_POSIX_MULTI_PROCESS
    check_environment();
#ifdef __PICOLIBC__
    l = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (l != (locale_t)0) {
        freelocale(l);
        check_env(NULL, "C.UTF-8", "C", LC_CTYPE_MASK, 1, "LC_CTYPE=C.UTF-8");
        check_env(NULL, "C.UTF-8", "C", LC_ALL_MASK, 1, "LC_CTYPE=C.UTF-8, all");
    }
#endif
#else
    printf("no fork, skipping the environment checks\n");
#endif

    printf("success\n");
    return 0;
}
