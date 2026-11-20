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

/* The synchronization part of what <pthread.h> provides */

#define _POSIX_C_SOURCE 202405L
#include <pthread.h>

/*
 * Picolibc defines the constants as macros; glibc makes most of them
 * enumerators, which the static assertions below still check
 */
#ifdef __PICOLIBC__
#if !defined(PTHREAD_BARRIER_SERIAL_THREAD) || !defined(PTHREAD_MUTEX_DEFAULT)   \
    || !defined(PTHREAD_MUTEX_ERRORCHECK) || !defined(PTHREAD_MUTEX_NORMAL)      \
    || !defined(PTHREAD_MUTEX_RECURSIVE) || !defined(PTHREAD_MUTEX_ROBUST)       \
    || !defined(PTHREAD_MUTEX_STALLED) || !defined(PTHREAD_PRIO_INHERIT)         \
    || !defined(PTHREAD_PRIO_NONE) || !defined(PTHREAD_PRIO_PROTECT)             \
    || !defined(PTHREAD_PROCESS_SHARED) || !defined(PTHREAD_PROCESS_PRIVATE)     \
    || !defined(PTHREAD_COND_INITIALIZER) || !defined(PTHREAD_MUTEX_INITIALIZER) \
    || !defined(PTHREAD_RWLOCK_INITIALIZER)
#error <pthread.h> must define the synchronization constants and initializers
#endif
#endif

/* The constants of each kind are distinct */
_Static_assert(PTHREAD_MUTEX_NORMAL != PTHREAD_MUTEX_ERRORCHECK
                   && PTHREAD_MUTEX_NORMAL != PTHREAD_MUTEX_RECURSIVE
                   && PTHREAD_MUTEX_ERRORCHECK != PTHREAD_MUTEX_RECURSIVE,
               "mutex types");
_Static_assert(PTHREAD_MUTEX_ROBUST != PTHREAD_MUTEX_STALLED, "mutex robustness");
_Static_assert(PTHREAD_PRIO_NONE != PTHREAD_PRIO_INHERIT
                   && PTHREAD_PRIO_NONE != PTHREAD_PRIO_PROTECT
                   && PTHREAD_PRIO_INHERIT != PTHREAD_PRIO_PROTECT,
               "mutex protocols");
_Static_assert(PTHREAD_PROCESS_SHARED != PTHREAD_PROCESS_PRIVATE, "process-shared values");

/* PTHREAD_BARRIER_SERIAL_THREAD differs from 0, which the other threads get */
_Static_assert(PTHREAD_BARRIER_SERIAL_THREAD != 0, "PTHREAD_BARRIER_SERIAL_THREAD");

/* The initializers are constant expressions for objects with static storage */
static pthread_mutex_t       s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t        s_cond = PTHREAD_COND_INITIALIZER;
static pthread_rwlock_t      s_rwlock = PTHREAD_RWLOCK_INITIALIZER;

/* The types */
static pthread_barrier_t     s_barrier;
static pthread_barrierattr_t s_barrierattr;
static pthread_condattr_t    s_condattr;
static pthread_mutexattr_t   s_mutexattr;
static pthread_rwlockattr_t  s_rwlockattr;
static pthread_spinlock_t    s_spinlock;

#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)

#define proto(name, ret, ...) static ret (* const p_##name)(__VA_ARGS__) = name

/* Each function has the specified prototype */
proto(pthread_barrier_destroy, int, pthread_barrier_t *);
proto(pthread_barrier_init, int, pthread_barrier_t * restrict,
      const pthread_barrierattr_t * restrict, unsigned);
proto(pthread_barrier_wait, int, pthread_barrier_t *);
proto(pthread_barrierattr_destroy, int, pthread_barrierattr_t *);
proto(pthread_barrierattr_getpshared, int, const pthread_barrierattr_t * restrict, int * restrict);
proto(pthread_barrierattr_init, int, pthread_barrierattr_t *);
proto(pthread_barrierattr_setpshared, int, pthread_barrierattr_t *, int);
proto(pthread_cond_broadcast, int, pthread_cond_t *);
proto(pthread_cond_destroy, int, pthread_cond_t *);
proto(pthread_cond_init, int, pthread_cond_t * restrict, const pthread_condattr_t * restrict);
proto(pthread_cond_signal, int, pthread_cond_t *);
proto(pthread_cond_timedwait, int, pthread_cond_t * restrict, pthread_mutex_t * restrict,
      const struct timespec * restrict);
proto(pthread_cond_wait, int, pthread_cond_t * restrict, pthread_mutex_t * restrict);
proto(pthread_condattr_destroy, int, pthread_condattr_t *);
proto(pthread_condattr_getclock, int, const pthread_condattr_t * restrict, clockid_t * restrict);
proto(pthread_condattr_getpshared, int, const pthread_condattr_t * restrict, int * restrict);
proto(pthread_condattr_init, int, pthread_condattr_t *);
proto(pthread_condattr_setclock, int, pthread_condattr_t *, clockid_t);
proto(pthread_condattr_setpshared, int, pthread_condattr_t *, int);
proto(pthread_mutex_consistent, int, pthread_mutex_t *);
proto(pthread_mutex_destroy, int, pthread_mutex_t *);
proto(pthread_mutex_getprioceiling, int, const pthread_mutex_t * restrict, int * restrict);
proto(pthread_mutex_init, int, pthread_mutex_t * restrict, const pthread_mutexattr_t * restrict);
proto(pthread_mutex_lock, int, pthread_mutex_t *);
proto(pthread_mutex_setprioceiling, int, pthread_mutex_t * restrict, int, int * restrict);
proto(pthread_mutex_timedlock, int, pthread_mutex_t * restrict, const struct timespec * restrict);
proto(pthread_mutex_trylock, int, pthread_mutex_t *);
proto(pthread_mutex_unlock, int, pthread_mutex_t *);
proto(pthread_mutexattr_destroy, int, pthread_mutexattr_t *);
proto(pthread_mutexattr_getprioceiling, int, const pthread_mutexattr_t * restrict, int * restrict);
proto(pthread_mutexattr_getprotocol, int, const pthread_mutexattr_t * restrict, int * restrict);
proto(pthread_mutexattr_getpshared, int, const pthread_mutexattr_t * restrict, int * restrict);
proto(pthread_mutexattr_getrobust, int, const pthread_mutexattr_t * restrict, int * restrict);
proto(pthread_mutexattr_gettype, int, const pthread_mutexattr_t * restrict, int * restrict);
proto(pthread_mutexattr_init, int, pthread_mutexattr_t *);
proto(pthread_mutexattr_setprioceiling, int, pthread_mutexattr_t *, int);
proto(pthread_mutexattr_setprotocol, int, pthread_mutexattr_t *, int);
proto(pthread_mutexattr_setpshared, int, pthread_mutexattr_t *, int);
proto(pthread_mutexattr_setrobust, int, pthread_mutexattr_t *, int);
proto(pthread_mutexattr_settype, int, pthread_mutexattr_t *, int);
proto(pthread_rwlock_destroy, int, pthread_rwlock_t *);
proto(pthread_rwlock_init, int, pthread_rwlock_t * restrict, const pthread_rwlockattr_t * restrict);
proto(pthread_rwlock_rdlock, int, pthread_rwlock_t *);
proto(pthread_rwlock_timedrdlock, int, pthread_rwlock_t * restrict,
      const struct timespec * restrict);
proto(pthread_rwlock_timedwrlock, int, pthread_rwlock_t * restrict,
      const struct timespec * restrict);
proto(pthread_rwlock_tryrdlock, int, pthread_rwlock_t *);
proto(pthread_rwlock_trywrlock, int, pthread_rwlock_t *);
proto(pthread_rwlock_unlock, int, pthread_rwlock_t *);
proto(pthread_rwlock_wrlock, int, pthread_rwlock_t *);
proto(pthread_rwlockattr_destroy, int, pthread_rwlockattr_t *);
proto(pthread_rwlockattr_getpshared, int, const pthread_rwlockattr_t * restrict, int * restrict);
proto(pthread_rwlockattr_init, int, pthread_rwlockattr_t *);
proto(pthread_rwlockattr_setpshared, int, pthread_rwlockattr_t *, int);
proto(pthread_spin_destroy, int, pthread_spinlock_t *);
proto(pthread_spin_init, int, pthread_spinlock_t *, int);
proto(pthread_spin_lock, int, pthread_spinlock_t *);
proto(pthread_spin_trylock, int, pthread_spinlock_t *);
proto(pthread_spin_unlock, int, pthread_spinlock_t *);

/* glibc 2.43 declares the clock_id variants only for _GNU_SOURCE */
#ifdef __PICOLIBC__
proto(pthread_cond_clockwait, int, pthread_cond_t * restrict, pthread_mutex_t * restrict, clockid_t,
      const struct timespec * restrict);
proto(pthread_mutex_clocklock, int, pthread_mutex_t * restrict, clockid_t,
      const struct timespec * restrict);
proto(pthread_rwlock_clockrdlock, int, pthread_rwlock_t * restrict, clockid_t,
      const struct timespec * restrict);
proto(pthread_rwlock_clockwrlock, int, pthread_rwlock_t * restrict, clockid_t,
      const struct timespec * restrict);
#endif

#include <stdio.h>

int
main(void)
{
    const void *used[] = {
        &s_mutex,
        &s_cond,
        &s_rwlock,
        &s_barrier,
        &s_barrierattr,
        &s_condattr,
        &s_mutexattr,
        &s_rwlockattr,
        (const void *)&s_spinlock,
        &p_pthread_barrier_destroy,
        &p_pthread_barrier_init,
        &p_pthread_barrier_wait,
        &p_pthread_barrierattr_destroy,
        &p_pthread_barrierattr_getpshared,
        &p_pthread_barrierattr_init,
        &p_pthread_barrierattr_setpshared,
        &p_pthread_cond_broadcast,
        &p_pthread_cond_destroy,
        &p_pthread_cond_init,
        &p_pthread_cond_signal,
        &p_pthread_cond_timedwait,
        &p_pthread_cond_wait,
        &p_pthread_condattr_destroy,
        &p_pthread_condattr_getclock,
        &p_pthread_condattr_getpshared,
        &p_pthread_condattr_init,
        &p_pthread_condattr_setclock,
        &p_pthread_condattr_setpshared,
        &p_pthread_mutex_consistent,
        &p_pthread_mutex_destroy,
        &p_pthread_mutex_getprioceiling,
        &p_pthread_mutex_init,
        &p_pthread_mutex_lock,
        &p_pthread_mutex_setprioceiling,
        &p_pthread_mutex_timedlock,
        &p_pthread_mutex_trylock,
        &p_pthread_mutex_unlock,
        &p_pthread_mutexattr_destroy,
        &p_pthread_mutexattr_getprioceiling,
        &p_pthread_mutexattr_getprotocol,
        &p_pthread_mutexattr_getpshared,
        &p_pthread_mutexattr_getrobust,
        &p_pthread_mutexattr_gettype,
        &p_pthread_mutexattr_init,
        &p_pthread_mutexattr_setprioceiling,
        &p_pthread_mutexattr_setprotocol,
        &p_pthread_mutexattr_setpshared,
        &p_pthread_mutexattr_setrobust,
        &p_pthread_mutexattr_settype,
        &p_pthread_rwlock_destroy,
        &p_pthread_rwlock_init,
        &p_pthread_rwlock_rdlock,
        &p_pthread_rwlock_timedrdlock,
        &p_pthread_rwlock_timedwrlock,
        &p_pthread_rwlock_tryrdlock,
        &p_pthread_rwlock_trywrlock,
        &p_pthread_rwlock_unlock,
        &p_pthread_rwlock_wrlock,
        &p_pthread_rwlockattr_destroy,
        &p_pthread_rwlockattr_getpshared,
        &p_pthread_rwlockattr_init,
        &p_pthread_rwlockattr_setpshared,
        &p_pthread_spin_destroy,
        &p_pthread_spin_init,
        &p_pthread_spin_lock,
        &p_pthread_spin_trylock,
        &p_pthread_spin_unlock,
#ifdef __PICOLIBC__
        &p_pthread_cond_clockwait,
        &p_pthread_mutex_clocklock,
        &p_pthread_rwlock_clockrdlock,
        &p_pthread_rwlock_clockwrlock,
#endif
    };

    (void)used;
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    (void)s_mutex;
    (void)s_cond;
    (void)s_rwlock;
    (void)s_barrier;
    (void)s_barrierattr;
    (void)s_condattr;
    (void)s_mutexattr;
    (void)s_rwlockattr;
    (void)s_spinlock;
    printf("no system support, skipping\n");
    return 77;
}

#endif
