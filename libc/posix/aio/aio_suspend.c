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

/* Cancellation removes the waiter */
static void
__aio_suspend_cleanup(void *arg)
{
    struct __aio_waiter *w = arg;

    __aio_remove_waiter(w);
    sem_destroy(&w->sem);
}

int
aio_suspend(const struct aiocb * const list[], int nent, const struct timespec *timeout)
{
    struct __aio_waiter w;
    struct timespec     end;
    int                 ret = 0;

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
    if (__aio_any_done(list, nent))
        return 0;

    /*
     * Requests that complete after the waiter is added post its
     * semaphore; the status of any that completed before is visible
     * once it is added. Nothing is locked while waiting, so a signal
     * handler can use aio functions, and sem_wait and sem_clockwait
     * return EINTR after a handler runs.
     */
    sem_init(&w.sem, 0, 0);
    __aio_add_waiter(&w);
    pthread_cleanup_push(__aio_suspend_cleanup, &w);
    while (!__aio_any_done(list, nent)) {
        int r;

        if (timeout)
            r = sem_clockwait(&w.sem, CLOCK_MONOTONIC, &end);
        else
            r = sem_wait(&w.sem);
        if (r < 0 && (errno == ETIMEDOUT || errno == EINTR)) {
            if (!__aio_any_done(list, nent))
                ret = errno == ETIMEDOUT ? EAGAIN : EINTR;
            break;
        }
    }
    pthread_cleanup_pop(1);
    if (ret) {
        errno = ret;
        return -1;
    }
    return 0;
}
