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

#include "local-signal.h"
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-sigevent-struct.h>
#include <freebsd/freebsd-aiocb-struct.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int __aio_signal(int sig, union sigval value);

/*
 * Only the kernel sends SI_ASYNCIO, from its own asynchronous I/O. Ask
 * it for an aio_mlock of no bytes, which completes at once and sends
 * the completion signal with that code and the value. Each request
 * stays with the kernel until aio_return collects it, which happens
 * on a later call once it is done.
 */
struct aio_note {
    struct aio_note       *next;
    struct __freebsd_aiocb cb;
};

static pthread_mutex_t  notes_lock = PTHREAD_MUTEX_INITIALIZER;
static struct aio_note *notes;
static char             note_buf;

int
__aio_signal(int sig, union sigval value)
{
    struct aio_note **prev, *n;
    int               ksig = _signal_to_freebsd(sig);
    long              ret;

    if (ksig < 0) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&notes_lock);
    prev = &notes;
    while ((n = *prev) != NULL) {
        if (_syscall_raw(FREEBSD_SYS_aio_error, &n->cb) != FREEBSD_EINPROGRESS) {
            (void)_syscall_raw(FREEBSD_SYS_aio_return, &n->cb);
            *prev = n->next;
            free(n);
        } else {
            prev = &n->next;
        }
    }

    n = calloc(1, sizeof(*n));
    if (!n) {
        pthread_mutex_unlock(&notes_lock);
        errno = EAGAIN;
        return -1;
    }
    n->cb.aio_fildes = -1;
    n->cb.aio_buf = &note_buf;
    n->cb.aio_nbytes = 0;
    n->cb.aio_sigevent.sigev_notify = FREEBSD_SIGEV_SIGNAL;
    n->cb.aio_sigevent.sigev_signo = ksig;
    n->cb.aio_sigevent.sigev_value = (__uint64_t)(uintptr_t)value.sival_ptr;
    if (sizeof(value.sival_ptr) < sizeof(n->cb.aio_sigevent.sigev_value))
        memcpy(&n->cb.aio_sigevent.sigev_value, &value, sizeof(value));
    ret = _syscall(FREEBSD_SYS_aio_mlock, &n->cb);
    if (ret < 0) {
        free(n);
    } else {
        n->next = notes;
        notes = n;
    }
    pthread_mutex_unlock(&notes_lock);
    return ret < 0 ? -1 : 0;
}
