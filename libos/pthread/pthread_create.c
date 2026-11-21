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
#include "local-pthread.h"
#include <picotls.h>
#include <string.h>

#define round_down(v, a) ((v) & ~((uintptr_t)(a) - 1))

/* First code run by a new thread, called by the OS layer */
void
_pthread_start(struct __pthread *self)
{
    _pthread_current = self;

    /* Wait while the creator sets explicit scheduling; give up if that failed */
    while (__atomic_load_n(&self->start_wait, __ATOMIC_ACQUIRE))
        (void)_futex_wait(&self->start_wait, 1, false, CLOCK_REALTIME, NULL);
    if (self->start_error) {
        /* Take any credential change sent meanwhile before leaving */
        _pthread_sigcancel_unblock();
        __atomic_fetch_sub(&_pthread_count, 1, __ATOMIC_RELAXED);
        _pthread_os_exit(self);
    }

    _pthread_sigsetmask(&self->sigmask);
    pthread_exit(self->start(self->arg));
}

/*
 * Every thread's descriptor, followed by its thread-specific data
 * values, sits at the top of its stack region, with the TLS block
 * just below and the stack below that, growing down. picolibc
 * allocates the region, with room for both and a guard below, unless
 * the application provided the stack.
 *
 * Returns ENOMEM when the memory can't be had and EAGAIN when another
 * limit, such as the number of threads, is reached; thrd_create tells
 * them apart.
 */
int
_pthread_create(pthread_t * __restrict thread, const pthread_attr_t * __restrict attr,
                void *(*start_routine)(void *), void * __restrict arg)
{
    pthread_attr_t         default_attr;
    struct _pthread_layout layout;
    struct __pthread      *t;
    void                  *map = NULL;
    size_t                 map_size = 0;
    uintptr_t              top;
    char                  *tls;
    _pthread_sigset_t      old;
    int                    ret;

    if (!attr) {
        pthread_attr_init(&default_attr);
        attr = &default_attr;
    }

    /* Make sure the initial thread has its thread id before any locking */
    (void)_pthread_self();
    _pthread_reap();

    _pthread_layout(&layout);
    if (attr->__stackaddr) {
        /* An application stack must hold the descriptor and TLS as well */
        if (attr->__stacksize < __PTHREAD_STACK_MIN)
            return EINVAL;
        top = (uintptr_t)attr->__stackaddr;
    } else {
        size_t size;

        if (__picolibc_add_overflow(attr->__stacksize, layout.overhead, &size))
            return ENOMEM;
        top = (uintptr_t)_pthread_os_stack_alloc(size, attr->__guardsize, &map, &map_size);
        if (!top)
            return ENOMEM;
    }

    t = (struct __pthread *)round_down(top - layout.desc_size, layout.align);
    memset(t, 0, layout.desc_size);
    t->specific = (struct __pthread_specific *)(t + 1);
    tls = (char *)round_down((uintptr_t)t - layout.tls_size, layout.align);
    _init_tls(tls);

    t->map = map;
    t->map_size = map_size;
    t->tls = tls;
    t->stack_top = (void *)round_down((uintptr_t)tls, 16);
    t->start = start_routine;
    t->arg = arg;
    t->detach_state = attr->__detachstate;
    t->start_wait = attr->__inheritsched == PTHREAD_EXPLICIT_SCHED;

    /*
     * Block signals until the new thread is running on its own stack;
     * it restores the creator's mask. Take the thread list lock first,
     * so a credential change can't wait on this thread meanwhile.
     */
    pthread_mutex_lock(&_pthread_all_lock);
    _pthread_sigblock_all(&old);
    t->sigmask = old;

    *thread = t;
    __atomic_fetch_add(&_pthread_count, 1, __ATOMIC_RELAXED);
    ret = _pthread_os_create(t);

    _pthread_sigsetmask(&old);

    if (ret == 0) {
        if (!_pthread_main_listed) {
            _pthread_all_add(&_pthread_main);
            _pthread_main_listed = true;
        }
        _pthread_all_add(t);
    }
    pthread_mutex_unlock(&_pthread_all_lock);

    if (ret != 0) {
        __atomic_fetch_sub(&_pthread_count, 1, __ATOMIC_RELAXED);
        if (map)
            _pthread_os_stack_free(map, map_size);
        return ret;
    }

    if (t->start_wait) {
        int sched_ret = _pthread_setsched(t, attr->__schedpolicy, attr->__schedpriority);

        t->start_error = sched_ret;
        __atomic_store_n(&t->start_wait, 0, __ATOMIC_RELEASE);
        _futex_wake(&t->start_wait, 1, false);
        if (sched_ret) {
            /* The thread exits without running; reclaim it */
            _pthread_wait_exit(t);
            pthread_mutex_lock(&_pthread_all_lock);
            _pthread_all_remove(t);
            pthread_mutex_unlock(&_pthread_all_lock);
            _pthread_release(t);
            return sched_ret;
        }
    }
    return 0;
}

int
pthread_create(pthread_t * __restrict thread, const pthread_attr_t * __restrict attr,
               void *(*start_routine)(void *), void * __restrict arg)
{
    int ret = _pthread_create(thread, attr, start_routine, arg);

    /* pthread_create reports a lack of memory as EAGAIN too */
    return ret == ENOMEM ? EAGAIN : ret;
}
