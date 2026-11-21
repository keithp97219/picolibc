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

#include "local-mqueue.h"
#include "local-timer.h"
#include "local-pthread.h"
#include <stdlib.h>
#include <string.h>

/*
 * FreeBSD has no SIGEV_THREAD for message queues, so a helper thread
 * blocks SIGLIBRT, the kernel is asked to send that signal to it
 * (SIGEV_THREAD_ID), and the helper calls the function when it comes.
 * The notification is one-shot. The kernel doesn't say when the
 * registration goes away, so mq_notify(mqdes, NULL) and mq_close tell
 * the helper through _mq_notify_cancel.
 */
#define _MQ_SIGNAL FREEBSD_SIGLIBRT

struct _mq_notify_helper {
    struct _mq_notify_helper *next;
    mqd_t                     mqdes;
    pthread_t                 thread;
    void                      (*function)(union sigval);
    union sigval              value;
    sem_t                     ready;
    int                       cancelled;
};

static pthread_mutex_t           helpers_lock = PTHREAD_MUTEX_INITIALIZER;
static struct _mq_notify_helper *helpers;

static void
mq_signal_set(struct __freebsd_sigset *set)
{
    memset(set, 0, sizeof(*set));
    set->__bits[(_MQ_SIGNAL - 1) / 32] = (__uint32_t)1 << ((_MQ_SIGNAL - 1) % 32);
}

/* Take the helper off the list; false when someone already had */
static bool
mq_unlist(struct _mq_notify_helper *h)
{
    struct _mq_notify_helper **prev;
    bool                       found = false;

    pthread_mutex_lock(&helpers_lock);
    for (prev = &helpers; *prev; prev = &(*prev)->next)
        if (*prev == h) {
            *prev = h->next;
            found = true;
            break;
        }
    pthread_mutex_unlock(&helpers_lock);
    return found;
}

static void *
mq_notify_wait(void *arg)
{
    struct _mq_notify_helper *h = arg;
    struct __freebsd_sigset   set;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    mq_signal_set(&set);
    (void)_syscall_raw(FREEBSD_SYS_sigprocmask, FREEBSD_SIG_BLOCK, &set, NULL);
    sem_post(&h->ready);

    while (_syscall_raw(FREEBSD_SYS_sigtimedwait, &set, NULL, NULL) < 0)
        ;
    /* A message arrived, unless the registration was cancelled */
    if (!__atomic_load_n(&h->cancelled, __ATOMIC_ACQUIRE) && mq_unlist(h))
        h->function(h->value);
    sem_destroy(&h->ready);
    free(h);
    return NULL;
}

static void
mq_stop(struct _mq_notify_helper *h)
{
    __atomic_store_n(&h->cancelled, 1, __ATOMIC_RELEASE);
    (void)_pthread_tkill(h->thread, _MQ_SIGNAL);
}

int
_mq_notify_thread(mqd_t mqdes, const struct sigevent *sev)
{
    struct __freebsd_sigevent kev;
    struct _mq_notify_helper *h;
    pthread_attr_t            attr;
    long                      ret;

    h = calloc(1, sizeof(*h));
    if (!h) {
        errno = EAGAIN;
        return -1;
    }
    h->mqdes = mqdes;
    h->function = sev->sigev_notify_function;
    h->value = sev->sigev_value;
    sem_init(&h->ready, 0, 0);

    if (sev->sigev_notify_attributes)
        attr = *(const pthread_attr_t *)sev->sigev_notify_attributes;
    else
        pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&h->thread, &attr, mq_notify_wait, h);
    if (ret) {
        sem_destroy(&h->ready);
        free(h);
        errno = EAGAIN;
        return -1;
    }
    /* The helper must block the signal before the kernel can send it */
    while (sem_wait(&h->ready) != 0)
        ;

    /* Listed first, so a message arriving at once finds it */
    pthread_mutex_lock(&helpers_lock);
    h->next = helpers;
    helpers = h;
    pthread_mutex_unlock(&helpers_lock);

    memset(&kev, 0, sizeof(kev));
    kev.sigev_notify = FREEBSD_SIGEV_THREAD_ID;
    kev.sigev_signo = _MQ_SIGNAL;
    kev.sigev_notify_thread_id = h->thread->tid;
    ret = _syscall_raw(FREEBSD_SYS_kmq_notify, mqdes, &kev);
    if (ret < 0) {
        if (mq_unlist(h))
            mq_stop(h);
        errno = _syscall_errno(ret);
        return -1;
    }
    return 0;
}

/* The registration for mqdes is gone: let its helper go */
void
_mq_notify_cancel(mqd_t mqdes)
{
    struct _mq_notify_helper **prev, *h;

    pthread_mutex_lock(&helpers_lock);
    for (prev = &helpers; (h = *prev) != NULL; prev = &h->next)
        if (h->mqdes == mqdes) {
            *prev = h->next;
            break;
        }
    pthread_mutex_unlock(&helpers_lock);
    if (h)
        mq_stop(h);
}
