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

#if defined(_PTHREAD_HAVE_SIGNALS) || defined(_PTHREAD_HAVE_CANCEL_WAKE)

/*
 * Act on a cancellation request which interrupted the calling thread,
 * if it arrived at the right time
 */
void
_pthread_cancel_interrupted(void)
{
    struct __pthread *self = _pthread_current;

    if (!self || self->cancel_state != PTHREAD_CANCEL_ENABLE
        || !__atomic_load_n(&self->cancel_pending, __ATOMIC_SEQ_CST))
        return;
    if (self->cancel_type != PTHREAD_CANCEL_ASYNCHRONOUS
        && !__atomic_load_n(&self->cancel_point, __ATOMIC_SEQ_CST))
        return;
    _pthread_cancel_act(self);
}

#endif

#ifdef _PTHREAD_HAVE_SIGNALS

static void
cancel_handler(int sig)
{
#ifdef _PTHREAD_SETXID
    struct __pthread *self = _pthread_current;

    /* The same signal asks each thread to repeat a credential change */
    if (self && __atomic_exchange_n(&self->setxid_pending, 0, __ATOMIC_ACQ_REL))
        _pthread_setxid_act();
#endif
    (void)sig;
    _pthread_cancel_interrupted();
}

static pthread_once_t cancel_once = PTHREAD_ONCE_INIT;

static void
cancel_init(void)
{
    _pthread_os_sigcancel(cancel_handler);
}

void
_pthread_sigcancel_init(void)
{
    pthread_once(&cancel_once, cancel_init);
}

#endif

int
pthread_cancel(pthread_t thread)
{
    struct __pthread *self = _pthread_self();

#ifdef _PTHREAD_HAVE_SIGNALS
    _pthread_sigcancel_init();
#endif
    __atomic_store_n(&thread->cancel_pending, 1, __ATOMIC_SEQ_CST);
    if (thread == self) {
        if (self->cancel_state == PTHREAD_CANCEL_ENABLE
            && self->cancel_type == PTHREAD_CANCEL_ASYNCHRONOUS)
            _pthread_cancel_act(self);
        return 0;
    }
#if defined(_PTHREAD_HAVE_SIGNALS) || defined(_PTHREAD_HAVE_CANCEL_WAKE)
    /*
     * A thread with cancellation disabled acts on the request at its
     * first cancellation point after enabling it. Signalling it now
     * would only interrupt calls which don't restart.
     */
    if (__atomic_load_n(&thread->cancel_state, __ATOMIC_SEQ_CST) != PTHREAD_CANCEL_ENABLE)
        return 0;
    /* Interrupt the thread in case it is blocked; an exited thread needs nothing */
    if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE))
#ifdef _PTHREAD_HAVE_SIGNALS
        (void)_pthread_tkill(thread, _PTHREAD_SIGCANCEL);
#else
        _pthread_os_cancel_wake(thread);
#endif
#endif
    return 0;
}
