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
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t __aio_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  __aio_done = PTHREAD_COND_INITIALIZER;
struct aiocb   *__aio_running;

/* Run one request; each runs in its own thread */
static void *
__aio_run(void *arg)
{
    struct aiocb      *cb = arg;
    struct sigevent    sigevent = cb->aio_sigevent;
    struct __aio_list *list = cb->__aio_list;
    bool               notify_list = false;
    struct aiocb     **prev;
    ssize_t            ret;
    int                error = 0;

    /*
     * Pipes, sockets and terminals can't seek, so there the offset
     * doesn't apply and the transfer is a plain read or write
     */
    switch (cb->__aio_op) {
    case LIO_READ:
        ret = pread(cb->aio_fildes, (void *)cb->aio_buf, cb->aio_nbytes, cb->aio_offset);
        if (ret < 0 && errno == ESPIPE)
            ret = read(cb->aio_fildes, (void *)cb->aio_buf, cb->aio_nbytes);
        break;
    case LIO_WRITE:
        ret = pwrite(cb->aio_fildes, (const void *)cb->aio_buf, cb->aio_nbytes, cb->aio_offset);
        if (ret < 0 && errno == ESPIPE)
            ret = write(cb->aio_fildes, (const void *)cb->aio_buf, cb->aio_nbytes);
        break;
    case __AIO_FDATASYNC:
        ret = fdatasync(cb->aio_fildes);
        break;
    default:
        ret = fsync(cb->aio_fildes);
        break;
    }
    if (ret < 0)
        error = errno;

    pthread_mutex_lock(&__aio_lock);
    for (prev = &__aio_running; *prev; prev = &(*prev)->__aio_next)
        if (*prev == cb) {
            *prev = cb->__aio_next;
            break;
        }
    __aio_set_status(cb, ret, error);
    if (list) {
        if (error)
            list->failed = true;
        if (--list->count == 0 && !list->wait)
            notify_list = true;
    }
    pthread_cond_broadcast(&__aio_done);
    pthread_mutex_unlock(&__aio_lock);

    /* The caller may reuse cb as soon as the status is visible */
    __aio_notify(&sigevent);
    if (notify_list) {
        __aio_notify(&list->sigevent);
        free(list);
    }
    return NULL;
}

void
__aio_lock_nosig(sigset_t *old)
{
    sigset_t all;

    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, old);
    pthread_mutex_lock(&__aio_lock);
}

void
__aio_unlock_nosig(const sigset_t *old)
{
    pthread_mutex_unlock(&__aio_lock);
    pthread_sigmask(SIG_SETMASK, old, NULL);
}

int
__aio_check_sigevent(const struct sigevent *sev)
{
    switch (sev->sigev_notify) {
    case SIGEV_NONE:
    case SIGEV_THREAD:
        return 0;
    case SIGEV_SIGNAL:
        if (sev->sigev_signo > 0 && sev->sigev_signo < _NSIG)
            return 0;
        break;
    }
    return EINVAL;
}

/* Starts a request; returns 0 or an error number */
int
__aio_submit(struct aiocb *cb, int op, struct __aio_list *list)
{
    pthread_attr_t attr;
    pthread_t      thread;
    sigset_t       old;
    int            ret;

    if (cb->aio_reqprio < 0 || cb->aio_reqprio > AIO_PRIO_DELTA_MAX)
        ret = EINVAL;
    else
        ret = __aio_check_sigevent(&cb->aio_sigevent);
    if (ret) {
        /* Don't leave an earlier request's status behind */
        __aio_set_status(cb, -1, ret);
        return ret;
    }

    cb->__aio_op = op;
    cb->__aio_list = list;

    /*
     * Signals stay blocked while the lock is held, and in the new
     * thread, as they go to the application's threads
     */
    __aio_lock_nosig(&old);
    __aio_set_status(cb, -1, EINPROGRESS);
    cb->__aio_next = __aio_running;
    __aio_running = cb;
    if (list)
        list->count++;
    pthread_mutex_unlock(&__aio_lock);

    ret = pthread_attr_init(&attr);
    if (ret == 0) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&thread, &attr, __aio_run, cb);
        pthread_attr_destroy(&attr);
    }

    if (ret) {
        struct aiocb **prev;

        pthread_mutex_lock(&__aio_lock);
        for (prev = &__aio_running; *prev; prev = &(*prev)->__aio_next)
            if (*prev == cb) {
                *prev = cb->__aio_next;
                break;
            }
        __aio_set_status(cb, -1, EAGAIN);
        if (list)
            list->count--;
        pthread_mutex_unlock(&__aio_lock);
    }
    pthread_sigmask(SIG_SETMASK, &old, NULL);
    return ret ? EAGAIN : 0;
}
