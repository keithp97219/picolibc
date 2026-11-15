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

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sched.h>
#include <time.h>

_BEGIN_STD_C

/* Detach state */
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

/* Scheduling inheritance and contention scope */
#define PTHREAD_INHERIT_SCHED  0
#define PTHREAD_EXPLICIT_SCHED 1
#define PTHREAD_SCOPE_SYSTEM   0
#define PTHREAD_SCOPE_PROCESS  1

/* Process sharing */
#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED  1

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL          0
#define PTHREAD_MUTEX_RECURSIVE       1
#define PTHREAD_MUTEX_ERRORCHECK      2
#define PTHREAD_MUTEX_DEFAULT         PTHREAD_MUTEX_NORMAL

#define PTHREAD_MUTEX_INITIALIZER     { 0, PTHREAD_MUTEX_DEFAULT, PTHREAD_PROCESS_PRIVATE, 0, 0 }
#define PTHREAD_COND_INITIALIZER      { 0, CLOCK_REALTIME, PTHREAD_PROCESS_PRIVATE }
#define PTHREAD_ONCE_INIT             0
#define PTHREAD_RWLOCK_INITIALIZER    { 0, 0, 0, 0, PTHREAD_PROCESS_PRIVATE }

#define PTHREAD_BARRIER_SERIAL_THREAD (-1)

/* Thread creation and termination */
int pthread_create(pthread_t * __restrict, const pthread_attr_t * __restrict, void *(*)(void *),
                   void * __restrict) __picolibc_export;
__noreturn void pthread_exit(void *) __picolibc_export;
int             pthread_join(pthread_t, void **) __picolibc_export;
int             pthread_detach(pthread_t) __picolibc_export;
pthread_t       pthread_self(void) __picolibc_export;
int             pthread_equal(pthread_t, pthread_t) __picolibc_export;

/* Thread attributes */
int             pthread_attr_init(pthread_attr_t *) __picolibc_export;
int             pthread_attr_destroy(pthread_attr_t *) __picolibc_export;
int             pthread_attr_getdetachstate(const pthread_attr_t *, int *) __picolibc_export;
int             pthread_attr_setdetachstate(pthread_attr_t *, int) __picolibc_export;
int             pthread_attr_getstacksize(const pthread_attr_t             *__restrict, size_t             *__restrict)
    __picolibc_export;
int pthread_attr_setstacksize(pthread_attr_t *, size_t) __picolibc_export;
int pthread_attr_getstack(const pthread_attr_t * __restrict, void ** __restrict,
                          size_t * __restrict) __picolibc_export;
int pthread_attr_setstack(pthread_attr_t *, void *, size_t) __picolibc_export;
int pthread_attr_getguardsize(const pthread_attr_t * __restrict, size_t * __restrict)
    __picolibc_export;
int pthread_attr_setguardsize(pthread_attr_t *, size_t) __picolibc_export;

/* Mutexes */
int pthread_mutex_init(pthread_mutex_t * __restrict, const pthread_mutexattr_t * __restrict)
    __picolibc_export;
int pthread_mutex_destroy(pthread_mutex_t *) __picolibc_export;
int pthread_mutex_lock(pthread_mutex_t *) __picolibc_export;
int pthread_mutex_trylock(pthread_mutex_t *) __picolibc_export;
int pthread_mutex_timedlock(pthread_mutex_t * __restrict, const struct timespec * __restrict)
    __picolibc_export;
int pthread_mutex_clocklock(pthread_mutex_t * __restrict, clockid_t,
                            const struct timespec * __restrict) __picolibc_export;
int pthread_mutex_unlock(pthread_mutex_t *) __picolibc_export;

int pthread_mutexattr_init(pthread_mutexattr_t *) __picolibc_export;
int pthread_mutexattr_destroy(pthread_mutexattr_t *) __picolibc_export;
int pthread_mutexattr_gettype(const pthread_mutexattr_t * __restrict, int * __restrict)
    __picolibc_export;
int pthread_mutexattr_settype(pthread_mutexattr_t *, int) __picolibc_export;
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * __restrict, int * __restrict)
    __picolibc_export;
int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int) __picolibc_export;

/* Condition variables */
int pthread_cond_init(pthread_cond_t * __restrict, const pthread_condattr_t * __restrict)
    __picolibc_export;
int pthread_cond_destroy(pthread_cond_t *) __picolibc_export;
int pthread_cond_wait(pthread_cond_t * __restrict, pthread_mutex_t * __restrict) __picolibc_export;
int pthread_cond_timedwait(pthread_cond_t * __restrict, pthread_mutex_t * __restrict,
                           const struct timespec * __restrict) __picolibc_export;
int pthread_cond_clockwait(pthread_cond_t * __restrict, pthread_mutex_t * __restrict, clockid_t,
                           const struct timespec * __restrict) __picolibc_export;
int pthread_cond_signal(pthread_cond_t *) __picolibc_export;
int pthread_cond_broadcast(pthread_cond_t *) __picolibc_export;

int pthread_condattr_init(pthread_condattr_t *) __picolibc_export;
int pthread_condattr_destroy(pthread_condattr_t *) __picolibc_export;
int pthread_condattr_getclock(const pthread_condattr_t * __restrict, clockid_t * __restrict)
    __picolibc_export;
int pthread_condattr_setclock(pthread_condattr_t *, clockid_t) __picolibc_export;
int pthread_condattr_getpshared(const pthread_condattr_t * __restrict, int * __restrict)
    __picolibc_export;
int pthread_condattr_setpshared(pthread_condattr_t *, int) __picolibc_export;

/* Reader-writer locks */
int pthread_rwlock_init(pthread_rwlock_t * __restrict, const pthread_rwlockattr_t * __restrict)
    __picolibc_export;
int pthread_rwlock_destroy(pthread_rwlock_t *) __picolibc_export;
int pthread_rwlock_rdlock(pthread_rwlock_t *) __picolibc_export;
int pthread_rwlock_tryrdlock(pthread_rwlock_t *) __picolibc_export;
int pthread_rwlock_timedrdlock(pthread_rwlock_t * __restrict, const struct timespec * __restrict)
    __picolibc_export;
int pthread_rwlock_clockrdlock(pthread_rwlock_t * __restrict, clockid_t,
                               const struct timespec * __restrict) __picolibc_export;
int pthread_rwlock_wrlock(pthread_rwlock_t *) __picolibc_export;
int pthread_rwlock_trywrlock(pthread_rwlock_t *) __picolibc_export;
int pthread_rwlock_timedwrlock(pthread_rwlock_t * __restrict, const struct timespec * __restrict)
    __picolibc_export;
int pthread_rwlock_clockwrlock(pthread_rwlock_t * __restrict, clockid_t,
                               const struct timespec * __restrict) __picolibc_export;
int pthread_rwlock_unlock(pthread_rwlock_t *) __picolibc_export;

int pthread_rwlockattr_init(pthread_rwlockattr_t *) __picolibc_export;
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *) __picolibc_export;
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t * __restrict, int * __restrict)
    __picolibc_export;
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int) __picolibc_export;

/* Barriers */
int pthread_barrier_init(pthread_barrier_t * __restrict, const pthread_barrierattr_t * __restrict,
                         unsigned) __picolibc_export;
int pthread_barrier_destroy(pthread_barrier_t *) __picolibc_export;
int pthread_barrier_wait(pthread_barrier_t *) __picolibc_export;

int pthread_barrierattr_init(pthread_barrierattr_t *) __picolibc_export;
int pthread_barrierattr_destroy(pthread_barrierattr_t *) __picolibc_export;
int pthread_barrierattr_getpshared(const pthread_barrierattr_t * __restrict, int * __restrict)
    __picolibc_export;
int   pthread_barrierattr_setpshared(pthread_barrierattr_t *, int) __picolibc_export;

/* Spin locks */
int   pthread_spin_init(pthread_spinlock_t *, int) __picolibc_export;
int   pthread_spin_destroy(pthread_spinlock_t *) __picolibc_export;
int   pthread_spin_lock(pthread_spinlock_t *) __picolibc_export;
int   pthread_spin_trylock(pthread_spinlock_t *) __picolibc_export;
int   pthread_spin_unlock(pthread_spinlock_t *) __picolibc_export;

/* Dynamic package initialization */
int   pthread_once(pthread_once_t *, void (*)(void)) __picolibc_export;

/* Thread-specific data */
int   pthread_key_create(pthread_key_t *, void (*)(void *)) __picolibc_export;
int   pthread_key_delete(pthread_key_t) __picolibc_export;
void *pthread_getspecific(pthread_key_t) __picolibc_export;
int   pthread_setspecific(pthread_key_t, const void *) __picolibc_export;

_END_STD_C

#endif /* _PTHREAD_H_ */
