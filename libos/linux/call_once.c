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
#include <threads.h>
#include <limits.h>

/*
 * *flag is ONCE_FLAG_INIT (0) before func runs, the id of the thread
 * running func while it runs, and CALL_ONCE_DONE afterwards. Callers
 * wait on the flag itself, so only callers using the same flag wait
 * for func, and a nested call from func on its own flag returns at
 * once instead of deadlocking.
 */
#define CALL_ONCE_DONE (-1)

/*
 * A func cancelled at a cancellation point leaves *flag as if
 * call_once had never been called; a waiting caller runs its func
 */
static void
call_once_cancelled(void *arg)
{
    once_flag *flag = arg;

    __atomic_store_n(flag, ONCE_FLAG_INIT, __ATOMIC_RELEASE);
    _futex_wake((unsigned int *)flag, INT_MAX, false);
}

void
call_once(once_flag *flag, void (*func)(void))
{
    int state = __atomic_load_n(flag, __ATOMIC_ACQUIRE);
    int self;

    if (state == CALL_ONCE_DONE)
        return;
    self = _pthread_self()->tid;
    while (state != CALL_ONCE_DONE) {
        if (state == ONCE_FLAG_INIT) {
            if (__atomic_compare_exchange_n(flag, &state, self, false, __ATOMIC_ACQUIRE,
                                            __ATOMIC_ACQUIRE)) {
                pthread_cleanup_push(call_once_cancelled, flag);
                func();
                pthread_cleanup_pop(0);
                __atomic_store_n(flag, CALL_ONCE_DONE, __ATOMIC_RELEASE);
                _futex_wake((unsigned int *)flag, INT_MAX, false);
                return;
            }
        } else if (state == self) {
            return;
        } else {
            (void)_futex_wait((unsigned int *)flag, (unsigned int)state, false, CLOCK_REALTIME,
                              NULL);
            state = __atomic_load_n(flag, __ATOMIC_ACQUIRE);
        }
    }
}
