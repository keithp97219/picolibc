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

/* The thread-management part of what <pthread.h> provides */

#define _POSIX_C_SOURCE 202405L
#include <pthread.h>

#ifndef __GLIBC__
#if !defined(PTHREAD_CANCEL_ASYNCHRONOUS) || !defined(PTHREAD_CANCEL_ENABLE)                       \
    || !defined(PTHREAD_CANCEL_DEFERRED) || !defined(PTHREAD_CANCEL_DISABLE)                       \
    || !defined(PTHREAD_CANCELED) || !defined(PTHREAD_CREATE_DETACHED)                             \
    || !defined(PTHREAD_CREATE_JOINABLE) || !defined(PTHREAD_EXPLICIT_SCHED)                       \
    || !defined(PTHREAD_INHERIT_SCHED) || !defined(PTHREAD_ONCE_INIT)                              \
    || !defined(PTHREAD_SCOPE_PROCESS) || !defined(PTHREAD_SCOPE_SYSTEM) || !defined(PTHREAD_NULL) \
    || !defined(pthread_cleanup_push) || !defined(pthread_cleanup_pop)
#error <pthread.h> must define the thread constants, PTHREAD_NULL and the cleanup macros
#endif
#endif

/* The constants of each kind are distinct */
_Static_assert(PTHREAD_CANCEL_ENABLE != PTHREAD_CANCEL_DISABLE, "cancelability states");
_Static_assert(PTHREAD_CANCEL_DEFERRED != PTHREAD_CANCEL_ASYNCHRONOUS, "cancelability types");
_Static_assert(PTHREAD_CREATE_DETACHED != PTHREAD_CREATE_JOINABLE, "detach states");
_Static_assert(PTHREAD_EXPLICIT_SCHED != PTHREAD_INHERIT_SCHED, "inheritsched values");
_Static_assert(PTHREAD_SCOPE_PROCESS != PTHREAD_SCOPE_SYSTEM, "contention scopes");

#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)

/* The initializers are constant expressions for objects with static storage */
static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static void * const   s_canceled = PTHREAD_CANCELED;
/* glibc 2.43 doesn't define PTHREAD_NULL yet */
#ifdef PTHREAD_NULL
static pthread_t s_null = PTHREAD_NULL;
#endif

/* The types */
static pthread_attr_t     s_attr;
static pthread_key_t      s_key;
static pthread_t          s_thread;

/* <sched.h> and <time.h> are visible */
static struct sched_param s_param = { .sched_priority = 0 };
static struct timespec    s_ts = { .tv_sec = 0, .tv_nsec = 0 };
static clockid_t          s_clock = CLOCK_REALTIME;

static time_t             s_time;
static const int          s_policies[] = { SCHED_FIFO, SCHED_RR, SCHED_OTHER };

static void
use_sched_time(void)
{
    struct timespec ts = { 0 };

    (void)sched_yield();
    (void)nanosleep(&ts, NULL);
}

#define proto(name, ret, ...) static ret (* const p_##name)(__VA_ARGS__) = name

/* Each function has the specified prototype */
proto(pthread_atfork, int, void (*)(void), void (*)(void), void (*)(void));
proto(pthread_attr_destroy, int, pthread_attr_t *);
proto(pthread_attr_getdetachstate, int, const pthread_attr_t *, int *);
proto(pthread_attr_getguardsize, int, const pthread_attr_t * restrict, size_t * restrict);
proto(pthread_attr_getinheritsched, int, const pthread_attr_t * restrict, int * restrict);
proto(pthread_attr_getschedparam, int, const pthread_attr_t * restrict,
      struct sched_param * restrict);
proto(pthread_attr_getschedpolicy, int, const pthread_attr_t * restrict, int * restrict);
proto(pthread_attr_getscope, int, const pthread_attr_t * restrict, int * restrict);
proto(pthread_attr_getstack, int, const pthread_attr_t * restrict, void ** restrict,
      size_t * restrict);
proto(pthread_attr_getstacksize, int, const pthread_attr_t * restrict, size_t * restrict);
proto(pthread_attr_init, int, pthread_attr_t *);
proto(pthread_attr_setdetachstate, int, pthread_attr_t *, int);
proto(pthread_attr_setguardsize, int, pthread_attr_t *, size_t);
proto(pthread_attr_setinheritsched, int, pthread_attr_t *, int);
proto(pthread_attr_setschedparam, int, pthread_attr_t * restrict,
      const struct sched_param * restrict);
proto(pthread_attr_setschedpolicy, int, pthread_attr_t *, int);
proto(pthread_attr_setscope, int, pthread_attr_t *, int);
proto(pthread_attr_setstack, int, pthread_attr_t *, void *, size_t);
proto(pthread_attr_setstacksize, int, pthread_attr_t *, size_t);
proto(pthread_cancel, int, pthread_t);
proto(pthread_create, int, pthread_t * restrict, const pthread_attr_t * restrict, void *(*)(void *),
      void * restrict);
proto(pthread_detach, int, pthread_t);
proto(pthread_equal, int, pthread_t, pthread_t);
proto(pthread_exit, void, void *);
proto(pthread_getcpuclockid, int, pthread_t, clockid_t *);
proto(pthread_getschedparam, int, pthread_t, int * restrict, struct sched_param * restrict);
proto(pthread_getspecific, void *, pthread_key_t);
proto(pthread_join, int, pthread_t, void **);
proto(pthread_key_create, int, pthread_key_t *, void (*)(void *));
proto(pthread_key_delete, int, pthread_key_t);
proto(pthread_once, int, pthread_once_t *, void (*)(void));
proto(pthread_self, pthread_t, void);
proto(pthread_setcancelstate, int, int, int *);
proto(pthread_setcanceltype, int, int, int *);
proto(pthread_setschedparam, int, pthread_t, int, const struct sched_param *);
proto(pthread_setschedprio, int, pthread_t, int);
proto(pthread_setspecific, int, pthread_key_t, const void *);
proto(pthread_testcancel, void, void);

#include <stdio.h>
#include <stdlib.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int cleanup_ran;

static void
cleanup(void *arg)
{
    cleanup_ran += *(int *)arg;
}

/* The cleanup macros pair up as statements in one lexical scope */
static void
use_cleanup(int execute)
{
    int one = 1, ten = 10;

    pthread_cleanup_push(cleanup, &one);
    pthread_cleanup_push(cleanup, &ten);
    pthread_cleanup_pop(execute);
    pthread_cleanup_pop(execute);
}

int
main(void)
{
    static int  object;
    int         local;
    const void *used[] = {
        &s_once,
        &s_attr,
        &s_key,
        &s_thread,
        &s_param,
        &s_ts,
        &s_clock,
        &s_time,
        s_policies,
        &p_pthread_atfork,
        &p_pthread_attr_destroy,
        &p_pthread_attr_getdetachstate,
        &p_pthread_attr_getguardsize,
        &p_pthread_attr_getinheritsched,
        &p_pthread_attr_getschedparam,
        &p_pthread_attr_getschedpolicy,
        &p_pthread_attr_getscope,
        &p_pthread_attr_getstack,
        &p_pthread_attr_getstacksize,
        &p_pthread_attr_init,
        &p_pthread_attr_setdetachstate,
        &p_pthread_attr_setguardsize,
        &p_pthread_attr_setinheritsched,
        &p_pthread_attr_setschedparam,
        &p_pthread_attr_setschedpolicy,
        &p_pthread_attr_setscope,
        &p_pthread_attr_setstack,
        &p_pthread_attr_setstacksize,
        &p_pthread_cancel,
        &p_pthread_create,
        &p_pthread_detach,
        &p_pthread_equal,
        &p_pthread_exit,
        &p_pthread_getcpuclockid,
        &p_pthread_getschedparam,
        &p_pthread_getspecific,
        &p_pthread_join,
        &p_pthread_key_create,
        &p_pthread_key_delete,
        &p_pthread_once,
        &p_pthread_self,
        &p_pthread_setcancelstate,
        &p_pthread_setcanceltype,
        &p_pthread_setschedparam,
        &p_pthread_setschedprio,
        &p_pthread_setspecific,
        &p_pthread_testcancel,
    };
    void *heap = malloc(1);

    (void)used;
    use_sched_time();

    /* PTHREAD_CANCELED is neither NULL nor the address of any object */
    check(s_canceled != NULL, "PTHREAD_CANCELED");
    check(s_canceled != (void *)&object && s_canceled != (void *)&local
              && s_canceled != (void *)&s_once && s_canceled != heap,
          "PTHREAD_CANCELED");
    free(heap);

    /* PTHREAD_NULL is a pthread_t that no thread has */
#ifdef PTHREAD_NULL
    check(!pthread_equal(pthread_self(), s_null), "PTHREAD_NULL");
    check(!pthread_equal(pthread_self(), PTHREAD_NULL), "PTHREAD_NULL");
    check(pthread_equal(PTHREAD_NULL, s_null), "PTHREAD_NULL equals itself");
#endif

    use_cleanup(1);
    check(cleanup_ran == 11, "pthread_cleanup_pop(1) ran the handlers");
    use_cleanup(0);
    check(cleanup_ran == 11, "pthread_cleanup_pop(0) didn't run the handler");

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
