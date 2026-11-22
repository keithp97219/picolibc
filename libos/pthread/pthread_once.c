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

/* *once is 0 before the routine runs, 1 while it runs and 2 afterwards */

/*
 * An init routine cancelled at a cancellation point leaves *once as
 * if pthread_once had never been called; a waiting caller runs it
 */
static void
once_cancelled(void *arg)
{
    pthread_once_t *once = arg;

    __atomic_store_n(once, 0, __ATOMIC_RELEASE);
    _futex_wake(once, INT_MAX, false);
}

int
pthread_once(pthread_once_t *once, void (*init_routine)(void))
{
    __pthread_futex_t state = __atomic_load_n(once, __ATOMIC_ACQUIRE);

    while (state != 2) {
        if (state == 0) {
            if (__atomic_compare_exchange_n(once, &state, 1, false, __ATOMIC_ACQUIRE,
                                            __ATOMIC_ACQUIRE)) {
                pthread_cleanup_push(once_cancelled, once);
                init_routine();
                pthread_cleanup_pop(0);
                __atomic_store_n(once, 2, __ATOMIC_RELEASE);
                _futex_wake(once, INT_MAX, false);
                return 0;
            }
        } else {
            (void)_futex_wait(once, 1, false, CLOCK_REALTIME, NULL);
            state = __atomic_load_n(once, __ATOMIC_ACQUIRE);
        }
    }
    return 0;
}
