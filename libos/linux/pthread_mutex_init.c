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
#include <errno.h>
#include <limits.h>

int
pthread_mutex_init(pthread_mutex_t * __restrict mutex, const pthread_mutexattr_t * __restrict attr)
{
    /* A robust mutex the system can't support would only hang */
    if (attr && attr->__robust == PTHREAD_MUTEX_ROBUST && !_pthread_robust_available())
        return ENOTSUP;
    mutex->__state = 0;
    mutex->__type = attr ? attr->__type : PTHREAD_MUTEX_DEFAULT;
    mutex->__pshared = attr ? attr->__pshared : PTHREAD_PROCESS_PRIVATE;
    mutex->__protocol = attr ? attr->__protocol : PTHREAD_PRIO_NONE;
    mutex->__prioceiling = attr ? attr->__prioceiling : LINUX_PRIO_RT_MIN;
    mutex->__owner = 0;
    mutex->__count = 0;
    mutex->__robust = attr ? attr->__robust : PTHREAD_MUTEX_STALLED;
    mutex->__robust_state = _PTHREAD_ROBUST_CONSISTENT;
    mutex->__robust_next = NULL;
    mutex->__robust_prev = NULL;
    return 0;
}
