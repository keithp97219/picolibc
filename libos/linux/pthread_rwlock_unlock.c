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
pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    unsigned int state = __atomic_load_n(&rwlock->__state, __ATOMIC_RELAXED);

    if (state == _PTHREAD_RWLOCK_WRITER) {
        if (__atomic_load_n(&rwlock->__writer, __ATOMIC_RELAXED) != _pthread_self()->tid)
            return EPERM;
        __atomic_store_n(&rwlock->__writer, 0, __ATOMIC_RELAXED);
        __atomic_store_n(&rwlock->__state, 0, __ATOMIC_SEQ_CST);
    } else if (state == 0) {
        return EPERM;
    } else if (__atomic_sub_fetch(&rwlock->__state, 1, __ATOMIC_SEQ_CST) != 0) {
        /* Other readers remain, so nobody waiting can proceed yet */
        return 0;
    }
    if (__atomic_load_n(&rwlock->__waiters, __ATOMIC_SEQ_CST)) {
        __atomic_fetch_add(&rwlock->__seq, 1, __ATOMIC_RELEASE);
        _futex_wake(&rwlock->__seq, INT_MAX, _pthread_rwlock_pshared(rwlock));
    }
    return 0;
}
