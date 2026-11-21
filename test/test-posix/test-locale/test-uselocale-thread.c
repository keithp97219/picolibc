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

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0

#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Each thread has its own current locale: a new thread starts with
 * the global locale, and uselocale in one thread doesn't change
 * another's
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int             stage;
static locale_t        main_locale, thread_locale, utf8;

/* Wait until stage reaches want */
static void
wait_stage(int want)
{
    pthread_mutex_lock(&lock);
    while (stage < want)
        pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);
}

static void
set_stage(int now)
{
    pthread_mutex_lock(&lock);
    stage = now;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

static void *
thread(void *arg)
{
    (void)arg;

    /* The thread-local locale isn't inherited */
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "new thread uses the global locale");

    check(uselocale(thread_locale) == LC_GLOBAL_LOCALE, "thread installs its locale");
    if (utf8 != (locale_t)0)
        check(MB_CUR_MAX > 1, "thread sees its own C.UTF-8");
    set_stage(1);

    /* main changes its own locale meanwhile */
    wait_stage(2);
    check(uselocale((locale_t)0) == thread_locale, "thread's locale is unchanged");
    if (utf8 != (locale_t)0)
        check(MB_CUR_MAX > 1, "thread still sees C.UTF-8");
    return NULL;
}

int
main(void)
{
    pthread_t t;

    main_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    check(main_locale != (locale_t)0, "newlocale");
#ifdef __PICOLIBC__
    /* With multibyte support, picolibc's C.UTF-8 shows which locale is in use */
    utf8 = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
#endif
    thread_locale = utf8 != (locale_t)0 ? utf8 : newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    check(thread_locale != (locale_t)0, "newlocale");
    if (utf8 == (locale_t)0)
        printf("no C.UTF-8 locale, only checking uselocale's results\n");

    check(uselocale(main_locale) == LC_GLOBAL_LOCALE, "main installs its locale");
    check(pthread_create(&t, NULL, thread, NULL) == 0, "pthread_create");

    /* The thread's uselocale doesn't change main's locale */
    wait_stage(1);
    check(uselocale((locale_t)0) == main_locale, "main's locale is unchanged");
    check(MB_CUR_MAX == 1, "main sees C");

    /* Nor does main's change the thread's */
    check(uselocale(LC_GLOBAL_LOCALE) == main_locale, "main uninstalls its locale");
    set_stage(2);
    check(pthread_join(t, NULL) == 0, "pthread_join");
    check(uselocale((locale_t)0) == LC_GLOBAL_LOCALE, "main uses the global locale");

    freelocale(thread_locale);
    freelocale(main_locale);
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
