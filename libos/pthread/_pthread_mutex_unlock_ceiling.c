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

int
_pthread_mutex_unlock_ceiling(pthread_mutex_t *mutex, int ceiling)
{
    struct __pthread *self = NULL;
    int               protocol = mutex->__protocol;
    bool              pshared = _pthread_mutex_pshared(mutex);

    if (mutex->__type != PTHREAD_MUTEX_NORMAL || protocol != PTHREAD_PRIO_NONE || mutex->__robust)
        self = _pthread_self();
    /* Robust mutexes report unlocking by a thread that doesn't own them, whatever the type */
    if (mutex->__type != PTHREAD_MUTEX_NORMAL || mutex->__robust) {
        if (_pthread_mutex_owner(mutex) != self->tid)
            return EPERM;
        if (mutex->__count) {
            mutex->__count--;
            return 0;
        }
        if (protocol != PTHREAD_PRIO_INHERIT && !mutex->__robust)
            __atomic_store_n(&mutex->__owner, 0, __ATOMIC_RELAXED);
    }

    /* Off the held list while still owning it; robust ones once off the robust list */
    if (protocol == PTHREAD_PRIO_PROTECT && !mutex->__robust)
        _pthread_tpp_unlink(self, mutex, ceiling);
    if (mutex->__robust) {
        _pthread_robust_unlock(mutex, self);
        if (protocol == PTHREAD_PRIO_PROTECT)
            _pthread_tpp_unlink(self, mutex, ceiling);
    } else if (protocol == PTHREAD_PRIO_INHERIT) {
        unsigned int state = (unsigned int)self->tid;

        /* With waiters the kernel hands the lock to the highest priority one */
        if (!__atomic_compare_exchange_n(&mutex->__state, &state, 0, false, __ATOMIC_RELEASE,
                                         __ATOMIC_RELAXED))
            _futex_unlock_pi(&mutex->__state, pshared);
    } else if (__atomic_exchange_n(&mutex->__state, 0, __ATOMIC_RELEASE) == 2) {
        _futex_wake(&mutex->__state, 1, pshared);
    }

    if (protocol == PTHREAD_PRIO_PROTECT)
        _pthread_tpp_lower(self);
    return 0;
}
