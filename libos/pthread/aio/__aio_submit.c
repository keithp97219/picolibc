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
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t      __aio_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t       __aio_done = PTHREAD_COND_INITIALIZER;
struct __aio_req    *__aio_running;
struct __aio_waiter *__aio_waiters;

/*
 * Whether an earlier request on the same descriptor must finish first:
 * appending writes go in the order they were queued, and a
 * synchronization waits for every request queued before it. The lock
 * is held.
 */
static bool
__aio_must_wait(const struct __aio_req *req)
{
    const struct __aio_req *r;

    /* Earlier requests follow this one in the list */
    for (r = req->next; r; r = r->next) {
        if (r->fildes != req->fildes)
            continue;
        if (req->op == __AIO_FSYNC || req->op == __AIO_FDATASYNC)
            return true;
        if (req->append && r->op == LIO_WRITE)
            return true;
    }
    return false;
}

/* Performs a request's transfer or synchronization */
static ssize_t
__aio_io(struct __aio_req *req)
{
    struct aiocb *cb = req->cb;
    ssize_t       ret;

    /*
     * Pipes, sockets and terminals can't seek, so there the offset
     * doesn't apply and the transfer is a plain read or write
     */
    switch (req->op) {
    case LIO_READ:
        ret = pread(req->fd, (void *)cb->aio_buf, cb->aio_nbytes, cb->aio_offset);
        if (ret < 0 && errno == ESPIPE)
            ret = read(req->fd, (void *)cb->aio_buf, cb->aio_nbytes);
        break;
    case LIO_WRITE:
        ret = pwrite(req->fd, (const void *)cb->aio_buf, cb->aio_nbytes, cb->aio_offset);
        if (ret < 0 && errno == ESPIPE)
            ret = write(req->fd, (const void *)cb->aio_buf, cb->aio_nbytes);
#ifdef SIGXFSZ
        /*
         * Past the file size limit, the kernel sends SIGXFSZ to this
         * thread, which blocks it, and it would go when the thread
         * exits; send it to the process instead
         */
        if (ret < 0 && errno == EFBIG) {
            sigset_t pending;

            if (sigpending(&pending) == 0 && sigismember(&pending, SIGXFSZ) == 1)
                kill(getpid(), SIGXFSZ);
            errno = EFBIG;
        }
#endif
        break;
    case __AIO_FDATASYNC:
        ret = fdatasync(req->fd);
        break;
    default:
        ret = fsync(req->fd);
        break;
    }
    return ret;
}

/* Run one request; each runs in its own thread */
static void *
__aio_run(void *arg)
{
    struct __aio_req  *req = arg;
    struct aiocb      *cb = req->cb;
    struct __aio_list *list = req->list;
    bool               notify_list = false;
    ssize_t            ret = -1;
    int                error = 0;
    bool               canceled;

    /* aio_cancel can cancel the request until it starts */
    pthread_mutex_lock(&__aio_lock);
    while (!req->canceled && __aio_must_wait(req))
        pthread_cond_wait(&__aio_done, &__aio_lock);
    canceled = req->canceled;
    req->started = true;
    pthread_mutex_unlock(&__aio_lock);

    if (canceled) {
        error = ECANCELED;
    } else {
        ret = __aio_io(req);
        if (ret < 0)
            error = errno;
    }
    close(req->fd);

    pthread_mutex_lock(&__aio_lock);
    /* aio_cancel has already set the status of a canceled request */
    if (!canceled) {
        __aio_unlink(req);
        __aio_set_status(cb, ret, error);
    }
    if (list) {
        if (error)
            list->failed = true;
        if (--list->count == 0 && !list->wait)
            notify_list = true;
    }
    __aio_wake();
    pthread_mutex_unlock(&__aio_lock);

    /* The caller may reuse cb as soon as the status is visible */
    __aio_notify(&req->sigevent);
    if (notify_list) {
        __aio_notify(&list->sigevent);
        free(list);
    }
    free(req);
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

void
__aio_wake(void)
{
    struct __aio_waiter *w;

    pthread_cond_broadcast(&__aio_done);
    for (w = __aio_waiters; w; w = w->next)
        sem_post(&w->sem);
}

void
__aio_unlink(struct __aio_req *req)
{
    struct __aio_req **prev;

    for (prev = &__aio_running; *prev; prev = &(*prev)->next)
        if (*prev == req) {
            *prev = req->next;
            break;
        }
}

void
__aio_add_waiter(struct __aio_waiter *w)
{
    sigset_t old;

    __aio_lock_nosig(&old);
    w->next = __aio_waiters;
    __aio_waiters = w;
    __aio_unlock_nosig(&old);
}

void
__aio_remove_waiter(struct __aio_waiter *w)
{
    struct __aio_waiter **prev;
    sigset_t              old;

    __aio_lock_nosig(&old);
    for (prev = &__aio_waiters; *prev; prev = &(*prev)->next)
        if (*prev == w) {
            *prev = w->next;
            break;
        }
    __aio_unlock_nosig(&old);
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
    struct __aio_req *req = NULL;
    pthread_attr_t    attr;
    pthread_t         thread;
    sigset_t          old;
    int               ret;

    /* aio_fsync ignores every member but aio_fildes and aio_sigevent */
    if (op != __AIO_FSYNC && op != __AIO_FDATASYNC
        && (cb->aio_reqprio < 0 || cb->aio_reqprio > AIO_PRIO_DELTA_MAX))
        ret = EINVAL;
    else
        ret = __aio_check_sigevent(&cb->aio_sigevent);
    if (ret == 0) {
        req = calloc(1, sizeof(*req));
        if (!req)
            ret = EAGAIN;
    }
    if (ret == 0) {
        /*
         * The request uses its own descriptor, so it completes as if a
         * close of the application's descriptor came after it
         */
        req->fd = fcntl(cb->aio_fildes, F_DUPFD_CLOEXEC, 0);
        if (req->fd < 0) {
            ret = errno == EBADF ? EBADF : EAGAIN;
            free(req);
        }
    }
    if (ret) {
        /* Don't leave an earlier request's status behind */
        __aio_set_status(cb, -1, ret);
        return ret;
    }

    req->cb = cb;
    req->list = list;
    req->sigevent = cb->aio_sigevent;
    req->op = op;
    req->fildes = cb->aio_fildes;
    /* Writes with O_APPEND, or to files that can't seek, append */
    if (op == LIO_WRITE)
        req->append = (fcntl(req->fd, F_GETFL) & O_APPEND) != 0
            || (lseek(req->fd, 0, SEEK_CUR) < 0 && errno == ESPIPE);

    /*
     * Signals stay blocked while the lock is held, and in the new
     * thread, as they go to the application's threads
     */
    __aio_lock_nosig(&old);
    __aio_set_status(cb, -1, EINPROGRESS);
    req->next = __aio_running;
    __aio_running = req;
    if (list)
        list->count++;
    pthread_mutex_unlock(&__aio_lock);

    ret = pthread_attr_init(&attr);
    if (ret == 0) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&thread, &attr, __aio_run, req);
        pthread_attr_destroy(&attr);
    }

    if (ret) {
        pthread_mutex_lock(&__aio_lock);
        __aio_unlink(req);
        __aio_set_status(cb, -1, EAGAIN);
        if (list)
            list->count--;
        pthread_mutex_unlock(&__aio_lock);
        close(req->fd);
        free(req);
    }
    pthread_sigmask(SIG_SETMASK, &old, NULL);
    return ret ? EAGAIN : 0;
}
