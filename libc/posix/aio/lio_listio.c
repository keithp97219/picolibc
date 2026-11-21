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
#include <limits.h>
#include <stdlib.h>

int
lio_listio(int mode, struct aiocb * __restrict const list[__restrict], int nent,
           struct sigevent * __restrict sev)
{
    struct __aio_list *l;
    bool               submit_failed = false, failed = false, notify = false, wait;
    int                submit_error = 0, ret;
    sigset_t           old;
    int                i;

    if ((mode != LIO_WAIT && mode != LIO_NOWAIT) || nent < 0 || nent > AIO_LISTIO_MAX) {
        errno = EINVAL;
        return -1;
    }
    if (mode == LIO_NOWAIT && sev && __aio_check_sigevent(sev)) {
        errno = EINVAL;
        return -1;
    }
    l = calloc(1, sizeof(*l));
    if (!l) {
        errno = EAGAIN;
        return -1;
    }
    wait = mode == LIO_WAIT;
    l->wait = wait;
    if (mode == LIO_NOWAIT && sev)
        l->sigevent = *sev;
    else
        l->sigevent.sigev_notify = SIGEV_NONE;

    /* Hold a count while submitting so the list can't finish early */
    l->count = 1;
    for (i = 0; i < nent; i++) {
        struct aiocb *cb = list[i];

        if (!cb)
            continue;
        switch (cb->aio_lio_opcode) {
        case LIO_READ:
        case LIO_WRITE:
            ret = __aio_submit(cb, cb->aio_lio_opcode, l);
            if (ret) {
                submit_failed = true;
                if (ret == EAGAIN)
                    submit_error = EAGAIN;
            }
            break;
        case LIO_NOP:
            break;
        default:
            __aio_set_status(cb, -1, EINVAL);
            submit_failed = true;
            break;
        }
    }

    if (wait) {
        struct __aio_waiter w;
        bool                interrupted = false;
        int                 cancel;

        /*
         * Cancelling the wait would leave l in use, so this isn't a
         * cancellation point. A signal handler ends the wait, and then
         * the last request frees l instead.
         */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel);
        sem_init(&w.sem, 0, 0);
        __aio_add_waiter(&w);
        __aio_lock_nosig(&old);
        --l->count;
        for (;;) {
            if (l->count == 0) {
                failed = l->failed;
                interrupted = false;
                break;
            }
            if (interrupted) {
                l->wait = false;
                break;
            }
            __aio_unlock_nosig(&old);
            if (sem_wait(&w.sem) < 0 && errno == EINTR)
                interrupted = true;
            __aio_lock_nosig(&old);
        }
        __aio_unlock_nosig(&old);
        __aio_remove_waiter(&w);
        sem_destroy(&w.sem);
        pthread_setcancelstate(cancel, NULL);
        if (interrupted) {
            errno = EINTR;
            return -1;
        }
    } else {
        __aio_lock_nosig(&old);
        if (--l->count == 0)
            notify = true;
        __aio_unlock_nosig(&old);
    }

    /* Without LIO_WAIT, the last request may already have freed l */
    if (wait) {
        free(l);
        if (failed || submit_failed) {
            errno = EIO;
            return -1;
        }
        return 0;
    }
    if (notify) {
        __aio_notify(&l->sigevent);
        free(l);
    }
    /* Resources ran out, or some requests were invalid */
    if (submit_failed) {
        errno = submit_error ? submit_error : EIO;
        return -1;
    }
    return 0;
}
