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
#include "../posix-groups.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)
#include <sys/wait.h>
#endif

/*
 * setlocale starts in the POSIX locale, treats "C" and "POSIX" alike,
 * returns strings that restore the locale, leaves the locale alone
 * when it fails and takes "" from LC_ALL, LC_* and LANG
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

static const int categories[]
    = { LC_COLLATE, LC_CTYPE, LC_MESSAGES, LC_MONETARY, LC_NUMERIC, LC_TIME };
#define NCAT (sizeof(categories) / sizeof(categories[0]))

static int
is_posix(const char *name)
{
    return name != NULL && (strcmp(name, "C") == 0 || strcmp(name, "POSIX") == 0);
}

static char *
query(int category)
{
    char *name = setlocale(category, NULL);

    check(name != NULL, "query");
    name = strdup(name);
    check(name != NULL, "strdup");
    return name;
}

#ifdef __PICOLIBC__
/* picolibc has C.UTF-8 when built with multibyte support */
static int
have_utf8(void)
{
    locale_t l = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);

    if (l == (locale_t)0)
        return 0;
    freelocale(l);
    return 1;
}
#endif

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)

static const char * const env_names[] = { "LC_ALL",      "LC_COLLATE", "LC_CTYPE", "LC_MESSAGES",
                                          "LC_MONETARY", "LC_NUMERIC", "LC_TIME",  "LANG" };
#define NENV (sizeof(env_names) / sizeof(env_names[0]))

/* Values for LC_ALL, LC_CTYPE, LC_NUMERIC and LANG; NULL leaves it unset */
struct env {
    const char *lc_all, *lc_ctype, *lc_numeric, *lang;
};

static void
set_env(const struct env *e)
{
    size_t i;

    for (i = 0; i < NENV; i++)
        check(unsetenv(env_names[i]) == 0, "unsetenv");
    if (e->lc_all)
        check(setenv("LC_ALL", e->lc_all, 1) == 0, "setenv");
    if (e->lc_ctype)
        check(setenv("LC_CTYPE", e->lc_ctype, 1) == 0, "setenv");
    if (e->lc_numeric)
        check(setenv("LC_NUMERIC", e->lc_numeric, 1) == 0, "setenv");
    if (e->lang)
        check(setenv("LANG", e->lang, 1) == 0, "setenv");
}

/*
 * In a child with the environment e, check whether setlocale(category, "")
 * succeeds, and that a failure changes nothing
 */
static void
check_env(const struct env *e, int category, int expect, const char *message)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        char *before, *after;
        char *ret;

        set_env(e);
        before = query(LC_ALL);
        ret = setlocale(category, "");
        if (expect) {
            if (ret == NULL) {
                printf("%s: setlocale(%d, \"\") failed\n", message, category);
                _exit(1);
            }
        } else {
            if (ret != NULL) {
                printf("%s: setlocale(%d, \"\") gave \"%s\"\n", message, category, ret);
                _exit(1);
            }
            after = query(LC_ALL);
            if (strcmp(before, after) != 0) {
                printf("%s: failure changed the locale\n", message);
                _exit(1);
            }
        }
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
    static const struct env none = { NULL, NULL, NULL, NULL };
    static const struct env lang_c = { NULL, NULL, NULL, "C" };
    static const struct env lang_posix = { NULL, NULL, NULL, "POSIX" };
    static const struct env lang_bogus = { NULL, NULL, NULL, BOGUS };
    static const struct env all_over_lang = { "C", NULL, NULL, BOGUS };
    static const struct env ctype_over_lang = { NULL, "C", NULL, BOGUS };
    static const struct env bogus_numeric = { NULL, NULL, BOGUS, "C" };
    static const struct env bogus_all = { BOGUS, "C", "C", "C" };
    static const struct env empty_all = { "", NULL, NULL, BOGUS };
    static const struct env empty_all_ctype = { "", "", NULL, "C" };
    size_t                  i;

    /* Nothing set: the default locale */
    check_env(&none, LC_ALL, 1, "no variables");

    /* LANG alone sets every category */
    check_env(&lang_c, LC_ALL, 1, "LANG=C");
    check_env(&lang_posix, LC_ALL, 1, "LANG=POSIX");
    check_env(&lang_bogus, LC_ALL, 0, "LANG unknown");
    for (i = 0; i < NCAT; i++)
        check_env(&lang_bogus, categories[i], 0, "LANG unknown, one category");

    /* LC_ALL takes precedence over everything */
    check_env(&all_over_lang, LC_ALL, 1, "LC_ALL over LANG");
    for (i = 0; i < NCAT; i++)
        check_env(&all_over_lang, categories[i], 1, "LC_ALL over LANG, one category");
    check_env(&bogus_all, LC_ALL, 0, "LC_ALL unknown");
    check_env(&bogus_all, LC_CTYPE, 0, "LC_ALL unknown over LC_CTYPE");

    /* LC_CTYPE takes precedence over LANG for its category only */
    check_env(&ctype_over_lang, LC_CTYPE, 1, "LC_CTYPE over LANG");
    check_env(&ctype_over_lang, LC_NUMERIC, 0, "LANG for LC_NUMERIC");
    check_env(&ctype_over_lang, LC_ALL, 0, "LC_ALL checks every category");

    /* One unknown category fails LC_ALL as a whole */
    check_env(&bogus_numeric, LC_NUMERIC, 0, "LC_NUMERIC unknown");
    check_env(&bogus_numeric, LC_CTYPE, 1, "LC_NUMERIC unknown, LC_CTYPE");
    check_env(&bogus_numeric, LC_ALL, 0, "LC_NUMERIC unknown, LC_ALL");

    /* Empty values are ignored */
    check_env(&empty_all, LC_ALL, 0, "empty LC_ALL, LANG unknown");
    check_env(&empty_all_ctype, LC_ALL, 1, "empty LC_ALL and LC_CTYPE");
}

#ifdef __PICOLIBC__
/* With C.UTF-8, check which variable chose LC_CTYPE */
static void
check_env_utf8(const struct env *e, int utf8, const char *message)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        set_env(e);
        if (setlocale(LC_ALL, "") == NULL) {
            printf("%s: setlocale failed\n", message);
            _exit(1);
        }
        if ((MB_CUR_MAX > 1) != utf8
            || (strcmp(setlocale(LC_CTYPE, NULL), "C.UTF-8") == 0) != utf8) {
            printf("%s: LC_CTYPE is %s\n", message, setlocale(LC_CTYPE, NULL));
            _exit(1);
        }
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("%s: child failed\n", message);
        exit(1);
    }
}

static void
check_environment_utf8(void)
{
    static const struct env lang = { NULL, NULL, NULL, "C.UTF-8" };
    static const struct env all = { "C", NULL, NULL, "C.UTF-8" };
    static const struct env ctype = { NULL, "C.UTF-8", NULL, "C" };
    static const struct env numeric = { NULL, NULL, "C.UTF-8", "C" };

    check_env_utf8(&lang, 1, "LANG=C.UTF-8");
    check_env_utf8(&all, 0, "LC_ALL=C over LANG");
    check_env_utf8(&ctype, 1, "LC_CTYPE=C.UTF-8");
    check_env_utf8(&numeric, 0, "LC_NUMERIC=C.UTF-8");
}
#endif

#endif /* __PICOLIBC_POSIX_MULTI_PROCESS */

int
main(void)
{
    char  *saved, *name;
    size_t i;

    /* The POSIX locale at entry to main */
    check(is_posix(setlocale(LC_ALL, NULL)), "POSIX locale at startup");
    for (i = 0; i < NCAT; i++)
        check(is_posix(setlocale(categories[i], NULL)), "each category starts as POSIX");

    /* "C" and "POSIX" both name the POSIX locale */
    check(is_posix(setlocale(LC_ALL, "POSIX")), "setlocale POSIX");
    check(is_posix(setlocale(LC_ALL, "C")), "setlocale C");
    for (i = 0; i < NCAT; i++) {
        check(is_posix(setlocale(categories[i], "POSIX")), "category POSIX");
        check(is_posix(setlocale(categories[i], "C")), "category C");
    }

    /* A query changes nothing, and its result restores the locale */
    saved = query(LC_ALL);
    name = query(LC_ALL);
    check(strcmp(saved, name) == 0, "query twice");
    free(name);
    check(setlocale(LC_ALL, saved) != NULL, "restore LC_ALL");
    for (i = 0; i < NCAT; i++) {
        name = query(categories[i]);
        check(setlocale(categories[i], name) != NULL, "restore a category");
        free(name);
    }

    /* An unknown locale fails and changes nothing */
    check(setlocale(LC_ALL, BOGUS) == NULL, "unknown locale");
    for (i = 0; i < NCAT; i++)
        check(setlocale(categories[i], BOGUS) == NULL, "unknown locale for a category");
    name = query(LC_ALL);
    check(strcmp(saved, name) == 0, "failure changes nothing");
    free(name);
    free(saved);

#ifdef __PICOLIBC__
    if (have_utf8()) {
        /* A failure leaves LC_CTYPE alone */
        check(setlocale(LC_CTYPE, "C.UTF-8") != NULL, "C.UTF-8");
        check(MB_CUR_MAX > 1, "C.UTF-8 is multibyte");
        check(setlocale(LC_ALL, BOGUS) == NULL, "unknown locale");
        check(strcmp(setlocale(LC_CTYPE, NULL), "C.UTF-8") == 0, "LC_CTYPE unchanged");

        /* The LC_ALL string restores LC_CTYPE */
        saved = query(LC_ALL);
        check(is_posix(setlocale(LC_ALL, "C")), "back to C");
        check(MB_CUR_MAX == 1, "C is single byte");
        check(setlocale(LC_ALL, saved) != NULL, "restore");
        check(strcmp(setlocale(LC_CTYPE, NULL), "C.UTF-8") == 0, "LC_CTYPE restored");
        free(saved);
        check(is_posix(setlocale(LC_ALL, "POSIX")), "back to POSIX");
    } else {
        printf("no C.UTF-8 locale, skipping its checks\n");
    }
#endif

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)
    check_environment();
#ifdef __PICOLIBC__
    if (have_utf8())
        check_environment_utf8();
#endif
#else
    printf("no fork, skipping the environment checks\n");
#endif

    printf("success\n");
    return 0;
}
