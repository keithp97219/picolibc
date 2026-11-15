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

#define _DEFAULT_SOURCE
#include "local-pthread.h"

static int
mutex_lock_futex(pthread_mutex_t *mutex, clockid_t clock, const struct timespec *abstime)
{
    unsigned int state = 0;
    int          ret;

    if (__atomic_compare_exchange_n(&mutex->__state, &state, 1, false, __ATOMIC_ACQUIRE,
                                    __ATOMIC_RELAXED))
        return 0;
    if (state != 2)
        state = __atomic_exchange_n(&mutex->__state, 2, __ATOMIC_ACQUIRE);
    while (state != 0) {
        ret = _futex_wait(&mutex->__state, 2, _pthread_mutex_pshared(mutex), clock, abstime);
        if (ret == ETIMEDOUT || ret == EINVAL)
            return ret;
        state = __atomic_exchange_n(&mutex->__state, 2, __ATOMIC_ACQUIRE);
    }
    return 0;
}

static int
mutex_lock_pi(pthread_mutex_t *mutex, int tid, clockid_t clock, const struct timespec *abstime)
{
    unsigned int state = 0;

    if (__atomic_compare_exchange_n(&mutex->__state, &state, (unsigned int)tid, false,
                                    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return 0;
    return _futex_lock_pi(&mutex->__state, _pthread_mutex_pshared(mutex), clock, abstime);
}

static int
deadlock(clockid_t clock, const struct timespec *abstime)
{
    unsigned int never = 0;
    int          ret;

    do
        ret = _futex_wait(&never, 0, false, clock, abstime);
    while (ret != ETIMEDOUT && ret != EINVAL);
    return ret;
}

int
_pthread_mutex_lock_timed(pthread_mutex_t *mutex, clockid_t clock, const struct timespec *abstime)
{
    struct __pthread *self = NULL;
    int               protocol = mutex->__protocol;
    int               ceiling = mutex->__prioceiling;
    int               tid = 0;
    int               ret;

    if (mutex->__type != PTHREAD_MUTEX_NORMAL || protocol != PTHREAD_PRIO_NONE) {
        self = _pthread_self();
        tid = self->tid;
    }
    if (mutex->__type != PTHREAD_MUTEX_NORMAL) {
        ret = _pthread_mutex_relock(mutex, tid, EDEADLK);
        if (ret >= 0)
            return ret;
    }

    if (protocol == PTHREAD_PRIO_PROTECT) {
        ret = _pthread_tpp_raise(self, ceiling);
        if (ret)
            return ret;
    }

    if (protocol == PTHREAD_PRIO_INHERIT) {
        ret = mutex_lock_pi(mutex, tid, clock, abstime);
        /*
         * The kernel reports relocking with EDEADLK, but relocking a
         * normal mutex must deadlock: wait until the timeout, if any.
         */
        if (ret == EDEADLK && mutex->__type == PTHREAD_MUTEX_NORMAL)
            ret = deadlock(clock, abstime);
    } else
        ret = mutex_lock_futex(mutex, clock, abstime);

    if (ret) {
        if (protocol == PTHREAD_PRIO_PROTECT)
            _pthread_tpp_lower(self, ceiling);
        return ret;
    }
    if (mutex->__type != PTHREAD_MUTEX_NORMAL && protocol != PTHREAD_PRIO_INHERIT)
        __atomic_store_n(&mutex->__owner, tid, __ATOMIC_RELAXED);
    return 0;
}
