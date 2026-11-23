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

#ifndef _LOCAL_CANCEL_H_
#define _LOCAL_CANCEL_H_

/*
 * Bracket the blocking part of each cancellation point. A pending
 * cancellation request acts on entry, and one arriving while blocked
 * acts from the cancellation signal handler.
 */

#include <picolibc.h>

/*
 * The thread core (and an OS layer building it outside picolibc's own
 * build, whose picolibc.h may not say __PICOLIBC_PTHREADS) includes
 * local-pthread.h first
 */
#if defined(__PICOLIBC_PTHREADS) || defined(_LOCAL_PTHREAD_H_)

#include "local-pthread.h"

static inline struct __pthread *
_cancel_enter(void)
{
    struct __pthread *self = _pthread_current;

    /*
     * The initial thread has no descriptor until it calls
     * pthread_self, so nobody can have asked to cancel it yet.
     */
    if (!self || self->cancel_state != PTHREAD_CANCEL_ENABLE)
        return NULL;
    __atomic_add_fetch(&self->cancel_point, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&self->cancel_pending, __ATOMIC_SEQ_CST))
        _pthread_cancel_act(self);
    return self;
}

static inline void
_cancel_leave(struct __pthread *self)
{
    if (self)
        __atomic_sub_fetch(&self->cancel_point, 1, __ATOMIC_SEQ_CST);
}

/*
 * Library functions which aren't cancellation points themselves but
 * call ones which are hold cancellation off around those calls.
 */
static inline int
_cancel_disable(void)
{
    struct __pthread *self = _pthread_current;
    int               old;

    if (!self)
        return PTHREAD_CANCEL_ENABLE;
    old = self->cancel_state;
    __atomic_store_n(&self->cancel_state, PTHREAD_CANCEL_DISABLE, __ATOMIC_SEQ_CST);
    return old;
}

static inline void
_cancel_restore(int old)
{
    struct __pthread *self = _pthread_current;

    if (self)
        __atomic_store_n(&self->cancel_state, old, __ATOMIC_SEQ_CST);
}

#else

#define _cancel_disable()    0
#define _cancel_restore(old) ((void)(old))
#define _cancel_enter()      ((void *)0)
#define _cancel_leave(self)  ((void)(self))

#endif

#endif /* _LOCAL_CANCEL_H_ */
