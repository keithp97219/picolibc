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
#include <limits.h>

int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    struct __pthread *self = NULL;
    int               protocol = mutex->__protocol;
    int               ceiling = mutex->__prioceiling;
    __pthread_futex_t state = 0;
    int               tid = 0;
    int               ret;

    if (mutex->__type != PTHREAD_MUTEX_NORMAL || protocol != PTHREAD_PRIO_NONE || mutex->__robust) {
        self = _pthread_self();
        tid = self->tid;
    }
    if (mutex->__type != PTHREAD_MUTEX_NORMAL) {
        ret = _pthread_mutex_relock(mutex, tid, EBUSY);
        if (ret >= 0)
            return ret;
    }
    if (protocol == PTHREAD_PRIO_PROTECT) {
        ret = _pthread_tpp_raise(self, ceiling);
        if (ret)
            return ret;
    }
    if (mutex->__robust) {
        ret = _pthread_robust_lock(mutex, self, CLOCK_REALTIME, NULL, true);
        if (protocol == PTHREAD_PRIO_PROTECT) {
            if (ret == 0 || ret == EOWNERDEAD)
                _pthread_tpp_link(self, mutex);
            else
                _pthread_tpp_lower(self);
        }
        return ret;
    }
    if (!__atomic_compare_exchange_n(&mutex->__state, &state,
                                     protocol == PTHREAD_PRIO_INHERIT ? (__pthread_futex_t)tid : 1,
                                     false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (protocol == PTHREAD_PRIO_PROTECT)
            _pthread_tpp_lower(self);
        return EBUSY;
    }
    if (protocol == PTHREAD_PRIO_PROTECT)
        _pthread_tpp_link(self, mutex);
    if (mutex->__type != PTHREAD_MUTEX_NORMAL && protocol != PTHREAD_PRIO_INHERIT)
        __atomic_store_n(&mutex->__owner, tid, __ATOMIC_RELAXED);
    return 0;
}
