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

#ifndef _LOCAL_AIO_H_
#define _LOCAL_AIO_H_

#define _DEFAULT_SOURCE
#include <aio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

/* Operations beyond the LIO_ opcodes */
#define __AIO_FSYNC     3
#define __AIO_FDATASYNC 4

/*
 * A lio_listio call: the count of operations still running, whether
 * any failed, and how to report the end of them all.
 */
struct __aio_list {
    int             count;
    bool            failed;
    bool            wait;
    struct sigevent sigevent;
};

/*
 * A queued request. It holds what the request needs once its status
 * is set, when the application may reuse the aiocb.
 */
struct __aio_req {
    struct __aio_req  *next;
    struct aiocb      *cb;
    struct __aio_list *list;
    struct sigevent    sigevent;
    int                op;
    /* The application's descriptor, and a duplicate the request uses */
    int                fildes;
    int                fd;
};

/*
 * Guards the list of running requests and the lio_listio counts. A
 * request's status, __aio_error, is also read without it, as aio_error
 * and aio_return must work in signal handlers: __aio_return is set
 * first and __aio_error stored last, with release ordering.
 */
extern pthread_mutex_t   __aio_lock;
/* Broadcast whenever a request completes */
extern pthread_cond_t    __aio_done;
/* The running requests, newest first */
extern struct __aio_req *__aio_running;

/* Take __aio_lock with signals blocked, so no handler runs while it's held */
void                     __aio_lock_nosig(sigset_t *old);
void                     __aio_unlock_nosig(const sigset_t *old);

/* Records a request's final status */
static inline void
__aio_set_status(struct aiocb *cb, ssize_t ret, int error)
{
    cb->__aio_return = ret;
    __atomic_store_n(&cb->__aio_error, error, __ATOMIC_RELEASE);
}

int  __aio_check_sigevent(const struct sigevent *sev);
int  __aio_submit(struct aiocb *cb, int op, struct __aio_list *list);
void __aio_notify(const struct sigevent *sev);

/* Sends 'sig' to the process for a completed request; the OS can supply its own */
int  __aio_signal(int sig, union sigval value);

#endif /* _LOCAL_AIO_H_ */
