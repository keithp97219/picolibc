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

#include "local-timer.h"
#include "local-pthread.h"
#include <stdlib.h>
#include <string.h>

/*
 * Each SIGEV_THREAD timer has a helper thread which waits for the
 * kernel's notification signal and calls the function. The kernel
 * signals that thread alone (SIGEV_THREAD_ID) with SIGLIBRT, which
 * picolibc's signal numbers never reach. The helper owns the record
 * and frees it once the timer is deleted.
 */
#define _TIMER_SIGNAL FREEBSD_SIGLIBRT

struct _timer_thread {
    struct _timer_thread *next;
    int                   kid;
    pthread_t             thread;
    void                  (*function)(union sigval);
    union sigval          value;
    sem_t                 ready;
    int                   deleted;
};

static pthread_mutex_t       timers_lock = PTHREAD_MUTEX_INITIALIZER;
static struct _timer_thread *timers;

static void
timer_signal_set(struct __freebsd_sigset *set)
{
    memset(set, 0, sizeof(*set));
    set->__bits[(_TIMER_SIGNAL - 1) / 32] = (__uint32_t)1 << ((_TIMER_SIGNAL - 1) % 32);
}

static void *
timer_thread(void *arg)
{
    struct _timer_thread   *t = arg;
    struct __freebsd_sigset set;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    timer_signal_set(&set);
    (void)_syscall_raw(FREEBSD_SYS_sigprocmask, FREEBSD_SIG_BLOCK, &set, NULL);
    sem_post(&t->ready);

    for (;;) {
        if (_syscall_raw(FREEBSD_SYS_sigtimedwait, &set, NULL, NULL) < 0)
            continue;
        if (__atomic_load_n(&t->deleted, __ATOMIC_ACQUIRE))
            break;
        t->function(t->value);
    }
    sem_destroy(&t->ready);
    free(t);
    return NULL;
}

int
_timer_create_thread(long kclock, const struct sigevent *evp, timer_t *timerid)
{
    struct __freebsd_sigevent kev;
    struct _timer_thread     *t;
    pthread_attr_t            attr;
    long                      ret;
    int                       kid;

    t = calloc(1, sizeof(*t));
    if (!t)
        return EAGAIN;
    t->function = evp->sigev_notify_function;
    t->value = evp->sigev_value;
    sem_init(&t->ready, 0, 0);

    if (evp->sigev_notify_attributes)
        attr = *(pthread_attr_t *)evp->sigev_notify_attributes;
    else
        pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&t->thread, &attr, timer_thread, t);
    if (ret) {
        sem_destroy(&t->ready);
        free(t);
        return (int)ret;
    }
    /* The helper must block the signal before the timer can fire */
    while (sem_wait(&t->ready) != 0)
        ;

    memset(&kev, 0, sizeof(kev));
    kev.sigev_notify = FREEBSD_SIGEV_THREAD_ID;
    kev.sigev_signo = _TIMER_SIGNAL;
    kev.sigev_notify_thread_id = t->thread->tid;
    ret = _syscall_raw(FREEBSD_SYS_ktimer_create, kclock, &kev, &kid);
    if (ret < 0) {
        /* Tell the helper to exit; it frees the record */
        __atomic_store_n(&t->deleted, 1, __ATOMIC_RELEASE);
        (void)_pthread_tkill(t->thread, _TIMER_SIGNAL);
        return _syscall_errno(ret);
    }
    t->kid = kid;

    pthread_mutex_lock(&timers_lock);
    t->next = timers;
    timers = t;
    pthread_mutex_unlock(&timers_lock);
    *timerid = (timer_t)kid;
    return 0;
}

bool
_timer_delete_thread(timer_t timerid)
{
    struct _timer_thread **prev, *t;
    int                    kid = _timer_id(timerid);

    pthread_mutex_lock(&timers_lock);
    for (prev = &timers; (t = *prev) != NULL; prev = &t->next)
        if (t->kid == kid) {
            *prev = t->next;
            break;
        }
    pthread_mutex_unlock(&timers_lock);
    if (!t)
        return false;

    (void)_syscall_raw(FREEBSD_SYS_ktimer_delete, kid);
    __atomic_store_n(&t->deleted, 1, __ATOMIC_RELEASE);
    (void)_pthread_tkill(t->thread, _TIMER_SIGNAL);
    return true;
}
