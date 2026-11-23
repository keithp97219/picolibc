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
#include <stdlib.h>

#ifndef _PTHREAD_HAVE_LAST_EXIT
/* The process exits normally once its last thread does */
#define _pthread_os_last_exit(self) exit(0)
#endif

void
pthread_exit(void *value_ptr)
{
    struct __pthread *self = _pthread_self();
    int               state = __PTHREAD_JOINABLE;

    /* Cancellation stays disabled and deferred while the thread exits */
    __atomic_store_n(&self->cancel_state, PTHREAD_CANCEL_DISABLE, __ATOMIC_SEQ_CST);
    self->cancel_type = PTHREAD_CANCEL_DEFERRED;

    /*
     * Cancellation may exit from the handler of _PTHREAD_SIGCANCEL,
     * which blocks it; unblock it so this thread can still take part
     * in a credential change.
     */
    _pthread_sigcancel_unblock();

    _pthread_cleanup_exit(self);

    _pthread_specific_exit(self);
    self->result = value_ptr;

    pthread_mutex_lock(&_pthread_all_lock);
    _pthread_all_remove(self);
    pthread_mutex_unlock(&_pthread_all_lock);

    if (__atomic_sub_fetch(&_pthread_count, 1, __ATOMIC_ACQ_REL) == 0)
        _pthread_os_last_exit(self);

    if (!__atomic_compare_exchange_n(&self->detach_state, &state, __PTHREAD_EXITING, false,
                                     __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
        && self->map)
        _pthread_zombie(self);

    /* Exit just this thread; the kernel changes and wakes _pthread_exit_word(self) */
    _pthread_os_exit(self);
}
