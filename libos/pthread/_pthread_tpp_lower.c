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
 * While the mutex is still held: take it off the thread's list. Only
 * when it had the highest ceiling does that need finding again, from
 * the few mutexes still held.
 */
void
_pthread_tpp_unlink(struct __pthread *self, pthread_mutex_t *mutex, int ceiling)
{
    pthread_mutex_t *m;
    int              max;

    if (!mutex->__robust) {
        pthread_mutex_t *next = mutex->__list_next;

        *(pthread_mutex_t **)mutex->__list_prev = next;
        if (next)
            next->__list_prev = mutex->__list_prev;
        mutex->__list_next = NULL;
        mutex->__list_prev = NULL;
    }
    if (ceiling < self->tpp_max)
        return;
    max = _pthread_os_mutex_ceiling(self);
    for (m = self->tpp_list; m; m = m->__list_next)
        if (m->__prioceiling > max)
            max = m->__prioceiling;
    self->tpp_max = (int16_t)max;
}

/* After unlocking, or failing to lock: run at the highest ceiling still held */
void
_pthread_tpp_lower(struct __pthread *self)
{
    int policy, priority, target;

    if (_pthread_getsched(self, &policy, &priority) != 0)
        return;
    if (policy != SCHED_FIFO && policy != SCHED_RR)
        return;
    target = self->tpp_max > self->tpp_base ? self->tpp_max : self->tpp_base;
    if (target != priority)
        (void)_pthread_setsched(self, policy, target);
}
