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
_pthread_rwlock_lock_timed(pthread_rwlock_t *rwlock, bool write, clockid_t clock,
                           const struct timespec *abstime)
{
    int tid = _pthread_self()->tid;

    for (;;) {
        unsigned int seq, state;
        int          ret;

        ret = _pthread_rwlock_try(rwlock, write, tid);
        if (ret != EBUSY)
            return ret;

        /*
         * Register as a waiter before checking the lock again, so that
         * an unlock either sees the waiter and advances __seq, or
         * happens before the check.
         */
        seq = __atomic_load_n(&rwlock->__seq, __ATOMIC_ACQUIRE);
        __atomic_fetch_add(&rwlock->__waiters, 1, __ATOMIC_SEQ_CST);
        state = __atomic_load_n(&rwlock->__state, __ATOMIC_SEQ_CST);
        ret = 0;
        if (write ? state != 0 : state == _PTHREAD_RWLOCK_WRITER)
            ret = _futex_wait(&rwlock->__seq, seq, _pthread_rwlock_pshared(rwlock), clock, abstime);
        __atomic_fetch_sub(&rwlock->__waiters, 1, __ATOMIC_RELAXED);
        if (ret == ETIMEDOUT || ret == EINVAL)
            return ret;
    }
}
