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
#include "local-timer.h"
#include "local-pthread.h"
#include <stdlib.h>
#include <string.h>

/*
 * Each SIGEV_THREAD timer has a helper thread which waits for the
 * kernel's notification signal and calls the function. The helper
 * owns the record and frees it once the timer is deleted.
 */
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
timer_signal_set(__kernel_sigset_t *set)
{
    memset(set, 0, sizeof(*set));
    __kernel_sigset_set_mask(set, _TIMER_SIGNAL);
}

static void *
timer_thread(void *arg)
{
    struct _timer_thread   *t = arg;
    __kernel_sigset_t       set;
    struct __kernel_siginfo info;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    timer_signal_set(&set);
    (void)_syscall(LINUX_SYS_rt_sigprocmask, LINUX_SIG_BLOCK, &set, NULL, __KERNEL_NSIG_BYTES);
    sem_post(&t->ready);

    for (;;) {
        if (_syscall(LINUX_SYS_rt_sigtimedwait, &set, &info, NULL, __KERNEL_NSIG_BYTES) < 0)
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
_timer_create_thread(int kclock, const struct sigevent *evp, timer_t *timerid)
{
    struct __kernel_sigevent kev;
    struct _timer_thread    *t;
    pthread_attr_t           attr;
    int                      save_errno = errno;
    int                      ret;

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
        return ret;
    }
    /* The helper must block the signal before the timer can fire */
    while (sem_wait(&t->ready) != 0)
        ;

    memset(&kev, 0, sizeof(kev));
    kev.sigev_notify = LINUX_SIGEV_THREAD_ID;
    kev.sigev_signo = _TIMER_SIGNAL;
    kev.sigev_tid = t->thread->tid;
    if (_syscall(LINUX_SYS_timer_create, kclock, &kev, &t->kid) < 0) {
        ret = errno;
        errno = save_errno;
        /* Tell the helper to exit; it frees the record */
        __atomic_store_n(&t->deleted, 1, __ATOMIC_RELEASE);
        (void)_syscall(LINUX_SYS_tgkill, getpid(), t->thread->tid, _TIMER_SIGNAL);
        return ret;
    }

    pthread_mutex_lock(&timers_lock);
    t->next = timers;
    timers = t;
    pthread_mutex_unlock(&timers_lock);
    *timerid = (timer_t)t->kid;
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

    (void)_syscall(LINUX_SYS_timer_delete, kid);
    __atomic_store_n(&t->deleted, 1, __ATOMIC_RELEASE);
    (void)_syscall(LINUX_SYS_tgkill, getpid(), t->thread->tid, _TIMER_SIGNAL);
    return true;
}
