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
_sem_wait_timed(sem_t *sem, clockid_t clock, const struct timespec *abstime)
{
    bool pshared = sem->__pshared != 0;

    for (;;) {
        __pthread_futex_t value = __atomic_load_n(&sem->__value, __ATOMIC_RELAXED);
        int               ret;

        while (value > 0)
            if (__atomic_compare_exchange_n(&sem->__value, &value, value - 1, false,
                                            __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return 0;

        /* Register before checking again so that sem_post sees the waiter */
        __atomic_fetch_add(&sem->__waiters, 1, __ATOMIC_SEQ_CST);
        ret = 0;
        if (__atomic_load_n(&sem->__value, __ATOMIC_SEQ_CST) == 0)
            ret = _futex_wait(&sem->__value, 0, pshared, clock, abstime);
        __atomic_fetch_sub(&sem->__waiters, 1, __ATOMIC_RELAXED);
        if (ret == ETIMEDOUT || ret == EINVAL || ret == EINTR)
            return ret;
    }
}
