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

/* Before taking a mutex: run at its ceiling if that's higher */
int
_pthread_tpp_raise(struct __pthread *self, int ceiling)
{
    int  policy, priority;
    int  ret = _pthread_getsched(self, &policy, &priority);
    bool realtime;

    if (ret)
        return ret;
    realtime = policy == SCHED_FIFO || policy == SCHED_RR;
    if (self->tpp_max == _PTHREAD_TPP_NONE)
        self->tpp_base = (int16_t)priority;
    if (realtime && self->tpp_base > ceiling)
        return EINVAL;
    if (realtime && ceiling > priority) {
        ret = _pthread_setsched(self, policy, ceiling);
        if (ret)
            return ret;
    }
    return 0;
}

/* Once the mutex is held: add it to the thread's list */
void
_pthread_tpp_link(struct __pthread *self, pthread_mutex_t *mutex)
{
    int ceiling = mutex->__prioceiling;

    /* Robust mutexes are on the robust list already */
    if (!mutex->__robust) {
        mutex->__list_next = self->tpp_list;
        mutex->__list_prev = &self->tpp_list;
        if (self->tpp_list)
            self->tpp_list->__list_prev = &mutex->__list_next;
        self->tpp_list = mutex;
    }
    if (ceiling > self->tpp_max)
        self->tpp_max = (int16_t)ceiling;
}
