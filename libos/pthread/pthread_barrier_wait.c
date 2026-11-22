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

/*
 * Arrivals are counted under a lock which also covers reading __seq,
 * so each thread knows which group it joined. The last thread to
 * arrive resets the count and advances __seq, releasing the others.
 */

/* __lock is 0 when free, 1 when held and 2 when held with possible waiters */
static void
barrier_lock(pthread_barrier_t *barrier, bool pshared)
{
    __pthread_futex_t state = 0;

    if (__atomic_compare_exchange_n(&barrier->__lock, &state, 1, false, __ATOMIC_ACQUIRE,
                                    __ATOMIC_RELAXED))
        return;
    if (state != 2)
        state = __atomic_exchange_n(&barrier->__lock, 2, __ATOMIC_ACQUIRE);
    while (state != 0) {
        (void)_futex_wait(&barrier->__lock, 2, pshared, CLOCK_REALTIME, NULL);
        state = __atomic_exchange_n(&barrier->__lock, 2, __ATOMIC_ACQUIRE);
    }
}

static void
barrier_unlock(pthread_barrier_t *barrier, bool pshared)
{
    if (__atomic_exchange_n(&barrier->__lock, 0, __ATOMIC_RELEASE) == 2)
        _futex_wake(&barrier->__lock, 1, pshared);
}

int
pthread_barrier_wait(pthread_barrier_t *barrier)
{
    bool         pshared = barrier->__pshared == PTHREAD_PROCESS_SHARED;
    unsigned int seq;

    barrier_lock(barrier, pshared);
    seq = barrier->__seq;
    if (++barrier->__waiting == barrier->__count) {
        barrier->__waiting = 0;
        __atomic_store_n(&barrier->__seq, seq + 1, __ATOMIC_RELEASE);
        barrier_unlock(barrier, pshared);
        _futex_wake(&barrier->__seq, INT_MAX, pshared);
        return PTHREAD_BARRIER_SERIAL_THREAD;
    }
    barrier_unlock(barrier, pshared);
    while (__atomic_load_n(&barrier->__seq, __ATOMIC_ACQUIRE) == seq)
        (void)_futex_wait(&barrier->__seq, seq, pshared, CLOCK_REALTIME, NULL);
    return 0;
}
