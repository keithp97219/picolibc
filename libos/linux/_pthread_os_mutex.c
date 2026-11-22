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

#include "local-pthread-mutex.h"

/*
 * local-pthread.h's OS mutexes: priority inheritance futexes, managed
 * with the kernel, and robust mutexes (_pthread_robust.c), which may
 * also inherit priority.
 */

int
_pthread_os_mutex_init(pthread_mutex_t *mutex)
{
    if (mutex->__protocol == PTHREAD_PRIO_INHERIT && !_pthread_pi_available())
        return ENOTSUP;
    if (mutex->__robust && !_pthread_robust_available())
        return ENOTSUP;
    mutex->__robust_state = _PTHREAD_ROBUST_CONSISTENT;
    return 0;
}

int
_pthread_os_mutex_destroy(pthread_mutex_t *mutex)
{
    if (__atomic_load_n(&mutex->__state, __ATOMIC_RELAXED) != 0)
        return EBUSY;
    return 0;
}

int
_pthread_os_mutex_owner(pthread_mutex_t *mutex)
{
    return (int)(__atomic_load_n(&mutex->__state, __ATOMIC_RELAXED) & LINUX_FUTEX_TID_MASK);
}

int
_pthread_os_mutex_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                       const struct timespec *abstime, bool trylock)
{
    __pthread_futex_t state = 0;

    if (mutex->__robust)
        return _pthread_robust_lock(mutex, self, clock, abstime, trylock);
    if (__atomic_compare_exchange_n(&mutex->__state, &state, (__pthread_futex_t)self->tid, false,
                                    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return 0;
    if (trylock)
        return EBUSY;
    return _futex_lock_pi(&mutex->__state, _pthread_mutex_pshared(mutex), clock, abstime);
}

void
_pthread_os_mutex_unlock(pthread_mutex_t *mutex, struct __pthread *self)
{
    __pthread_futex_t state = (__pthread_futex_t)self->tid;

    if (mutex->__robust) {
        _pthread_robust_unlock(mutex, self);
        return;
    }
    /* With waiters the kernel hands the lock to the highest priority one */
    if (!__atomic_compare_exchange_n(&mutex->__state, &state, 0, false, __ATOMIC_RELEASE,
                                     __ATOMIC_RELAXED))
        _futex_unlock_pi(&mutex->__state, _pthread_mutex_pshared(mutex));
}

int
_pthread_os_mutex_consistent(pthread_mutex_t *mutex)
{
    if (mutex->__robust_state != _PTHREAD_ROBUST_INCONSISTENT)
        return EINVAL;
    mutex->__robust_state = _PTHREAD_ROBUST_CONSISTENT;
    __atomic_fetch_and(&mutex->__state, ~LINUX_FUTEX_OWNER_DIED, __ATOMIC_RELAXED);
    return 0;
}
