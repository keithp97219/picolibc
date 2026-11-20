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

#include "local-aio.h"

static bool
__aio_any_done(const struct aiocb * const list[], int nent)
{
    int i;

    for (i = 0; i < nent; i++)
        if (list[i] && __atomic_load_n(&list[i]->__aio_error, __ATOMIC_ACQUIRE) != EINPROGRESS)
            return true;
    return false;
}

int
aio_suspend(const struct aiocb * const list[], int nent, const struct timespec *timeout)
{
    struct timespec end;
    int             ret = 0;

    if (nent < 0) {
        errno = EINVAL;
        return -1;
    }
    if (timeout) {
        if (timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000L) {
            errno = EINVAL;
            return -1;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        end.tv_sec += timeout->tv_sec;
        end.tv_nsec += timeout->tv_nsec;
        if (end.tv_nsec >= 1000000000L) {
            end.tv_sec++;
            end.tv_nsec -= 1000000000L;
        }
    }
    pthread_mutex_lock(&__aio_lock);
    while (!__aio_any_done(list, nent)) {
        if (timeout)
            ret = pthread_cond_clockwait(&__aio_done, &__aio_lock, CLOCK_MONOTONIC, &end);
        else
            ret = pthread_cond_wait(&__aio_done, &__aio_lock);
        if (ret == ETIMEDOUT && !__aio_any_done(list, nent))
            break;
        ret = 0;
    }
    pthread_mutex_unlock(&__aio_lock);
    if (ret) {
        errno = EAGAIN;
        return -1;
    }
    return 0;
}
