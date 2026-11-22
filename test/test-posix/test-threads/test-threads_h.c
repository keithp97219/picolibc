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

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_C_LANG_THREADS
#include <threads.h>

/*
 * The contents of <threads.h>: types, enumeration constants, macros
 * and prototypes, the <time.h> symbols it makes visible, and names
 * it must not declare
 */

/*
 * <threads.h> may not declare these: the pthread_ prefix is reserved
 * for <pthread.h> and sched_ for <sched.h>. A declaration would
 * conflict with these definitions.
 */
static int
pthread_create(void)
{
    return 1;
}

static int
pthread_mutex_lock(void)
{
    return 2;
}

static int
pthread_key_create(void)
{
    return 3;
}

static int
sched_yield(void)
{
    return 4;
}

/* <time.h> symbols, with nothing else included yet */
static struct timespec time_spec;
static time_t          time_value;
static clockid_t       clock_value = CLOCK_REALTIME;
_Static_assert(TIME_UTC > 0, "TIME_UTC");

/* Complete object types, and the function pointer types */
_Static_assert(sizeof(cnd_t) > 0 && sizeof(mtx_t) > 0 && sizeof(once_flag) > 0 && sizeof(thrd_t) > 0
                   && sizeof(tss_t) > 0,
               "complete types");
_Static_assert(_Generic((thrd_start_t)0, int (*)(void *): 1, default: 0), "thrd_start_t");
_Static_assert(_Generic((tss_dtor_t)0, void (*)(void *): 1, default: 0), "tss_dtor_t");

/* Enumeration constants, not macros */
#if defined(mtx_plain) || defined(mtx_recursive) || defined(mtx_timed) || defined(thrd_busy) \
    || defined(thrd_error) || defined(thrd_nomem) || defined(thrd_success)                   \
    || defined(thrd_timedout)
#error <threads.h> constants are macros
#endif

/* Duplicate case labels would fail to compile */
static int
thrd_result_distinct(int v)
{
    switch (v) {
    case thrd_success:
    case thrd_busy:
    case thrd_error:
    case thrd_nomem:
    case thrd_timedout:
        return 1;
    }
    return 0;
}

static int
mtx_type_distinct(int v)
{
    switch (v) {
    case mtx_plain:
    case mtx_timed:
    case mtx_plain | mtx_recursive:
    case mtx_timed | mtx_recursive:
        return 1;
    }
    return 0;
}

/* TSS_DTOR_ITERATIONS works in #if */
#if TSS_DTOR_ITERATIONS < 4
#error TSS_DTOR_ITERATIONS is below _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#endif

#if !defined(__cplusplus) && __STDC_VERSION__ < 202311L
#ifndef thread_local
#error thread_local is not defined
#endif
#endif

#define str(x)  #x
#define xstr(x) str(x)

static thread_local int per_thread = 1;

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(PTHREAD_DESTRUCTOR_ITERATIONS) && TSS_DTOR_ITERATIONS != PTHREAD_DESTRUCTOR_ITERATIONS
#error TSS_DTOR_ITERATIONS differs from PTHREAD_DESTRUCTOR_ITERATIONS
#endif

#if defined(__STDC_NO_THREADS__) && !defined(_POSIX_SUBPROFILE)
#error __STDC_NO_THREADS__ is defined
#endif

/* thrd_t is pthread_t */
_Static_assert(_Generic((thrd_t) { 0 }, pthread_t: 1, default: 0), "thrd_t is pthread_t");

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The checks that need no library support */
static void
header_checks(void)
{
    (void)time_spec;
    (void)time_value;
    (void)clock_value;

    check(pthread_create() == 1 && pthread_mutex_lock() == 2 && pthread_key_create() == 3
              && sched_yield() == 4,
          "names left to the application");

    check(thrd_result_distinct(thrd_success) && mtx_type_distinct(mtx_plain), "constants");

#if !defined(__cplusplus) && __STDC_VERSION__ < 202311L
    check(strcmp(xstr(thread_local), "_Thread_local") == 0,
          "thread_local expands to _Thread_local");
#endif
    check(per_thread == 1, "thread_local initializer");
}

static once_flag once = ONCE_FLAG_INIT;

/* The prototypes, through pointers of the specified types */
static void      (     *volatile p_call_once)(once_flag *, void (*)(void)) = call_once;
static int       (      *volatile p_cnd_broadcast)(cnd_t *) = cnd_broadcast;
static void      (     *volatile p_cnd_destroy)(cnd_t *) = cnd_destroy;
static int       (      *volatile p_cnd_init)(cnd_t *) = cnd_init;
static int       (      *volatile p_cnd_signal)(cnd_t *) = cnd_signal;
static int       (      *volatile p_cnd_timedwait)(cnd_t       *restrict, mtx_t       *restrict,
                                        const struct timespec       *restrict)
    = cnd_timedwait;
static int  ( *volatile p_cnd_wait)(cnd_t *, mtx_t *) = cnd_wait;
static void (* volatile p_mtx_destroy)(mtx_t *) = mtx_destroy;
static int  ( *volatile p_mtx_init)(mtx_t *, int) = mtx_init;
static int  ( *volatile p_mtx_lock)(mtx_t *) = mtx_lock;
static int  ( *volatile p_mtx_timedlock)(mtx_t  *restrict, const struct timespec  *restrict)
    = mtx_timedlock;
static int    (   *volatile p_mtx_trylock)(mtx_t *) = mtx_trylock;
static int    (   *volatile p_mtx_unlock)(mtx_t *) = mtx_unlock;
static int    (   *volatile p_thrd_create)(thrd_t *, thrd_start_t, void *) = thrd_create;
static thrd_t (* volatile p_thrd_current)(void) = thrd_current;
static int    (   *volatile p_thrd_detach)(thrd_t) = thrd_detach;
static int    (   *volatile p_thrd_equal)(thrd_t, thrd_t) = thrd_equal;
static void   (  *volatile p_thrd_exit)(int) = thrd_exit;
static int    (   *volatile p_thrd_join)(thrd_t, int *) = thrd_join;
static int    (   *volatile p_thrd_sleep)(const struct timespec *, struct timespec *) = thrd_sleep;
static void   (  *volatile p_thrd_yield)(void) = thrd_yield;
static int    (   *volatile p_tss_create)(tss_t *, tss_dtor_t) = tss_create;
static void   (  *volatile p_tss_delete)(tss_t) = tss_delete;
static void  *( *volatile p_tss_get)(tss_t) = tss_get;
static int    (   *volatile p_tss_set)(tss_t, void *) = tss_set;

static int    once_count;

static void
once_init(void)
{
    once_count++;
}

static int
read_per_thread(void *arg)
{
    (void)arg;
    return per_thread;
}

int
main(void)
{
    thrd_t thr;
    int    res = -1;
    long   iterations;

    /* <time.h> functions are declared too */
    check(timespec_get(&time_spec, TIME_UTC) == TIME_UTC, "timespec_get");
    check(clock_gettime(clock_value, &time_spec) == 0, "clock_gettime");
    time_value = time(NULL);
    check(time_value != (time_t)-1, "time");

    header_checks();

    iterations = sysconf(_SC_THREAD_DESTRUCTOR_ITERATIONS);
    check(iterations == -1 || iterations >= TSS_DTOR_ITERATIONS,
          "TSS_DTOR_ITERATIONS within sysconf's value");

    check(p_call_once && p_cnd_broadcast && p_cnd_destroy && p_cnd_init && p_cnd_signal
              && p_cnd_timedwait && p_cnd_wait && p_mtx_destroy && p_mtx_init && p_mtx_lock
              && p_mtx_timedlock && p_mtx_trylock && p_mtx_unlock && p_thrd_create && p_thrd_current
              && p_thrd_detach && p_thrd_equal && p_thrd_exit && p_thrd_join && p_thrd_sleep
              && p_thrd_yield && p_tss_create && p_tss_delete && p_tss_get && p_tss_set,
          "functions");

    p_call_once(&once, once_init);
    p_call_once(&once, once_init);
    check(once_count == 1, "ONCE_FLAG_INIT");

    /* A thread_local object is per thread */
    per_thread = 2;
    check(thrd_create(&thr, read_per_thread, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success && res == 1, "thread_local");

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
