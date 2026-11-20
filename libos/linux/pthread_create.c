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
#include <linux/linux-errno.h>
#include <picotls.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>

#define LINUX_CLONE_PTHREAD                                                                      \
    (LINUX_CLONE_VM | LINUX_CLONE_FS | LINUX_CLONE_FILES | LINUX_CLONE_SIGHAND                   \
     | LINUX_CLONE_THREAD | LINUX_CLONE_SYSVSEM | LINUX_CLONE_SETTLS | LINUX_CLONE_PARENT_SETTID \
     | LINUX_CLONE_CHILD_CLEARTID)

/* All pages sizes used by the supported machines are multiples of this */
#define PTHREAD_PAGE_SIZE 4096

#define round_up(v, a)    (((v) + (a) - 1) & ~((uintptr_t)(a) - 1))
#define round_down(v, a)  ((v) & ~((uintptr_t)(a) - 1))

/* First code run by a new thread, called by _linux_clone */
static void *
pthread_start(void *arg)
{
    struct __pthread *self = arg;

    _pthread_current = self;

    /* Wait while the creator sets explicit scheduling; give up if that failed */
    while (__atomic_load_n(&self->start_wait, __ATOMIC_ACQUIRE))
        (void)_futex_wait(&self->start_wait, 1, false, CLOCK_REALTIME, NULL);
    if (self->start_error) {
        __kernel_sigset_t cancel = {};

        /* Take any credential change sent meanwhile before leaving */
        __kernel_sigset_set_mask(&cancel, _PTHREAD_SIGCANCEL);
        (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_UNBLOCK, &cancel, NULL,
                           __KERNEL_NSIG_BYTES);
        __atomic_fetch_sub(&_pthread_count, 1, __ATOMIC_RELAXED);
        for (;;)
            _syscall_raw(LINUX_SYS_exit, 0);
    }

    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_SETMASK, &self->sigmask, NULL,
                       __KERNEL_NSIG_BYTES);
    pthread_exit(self->start(self->arg));
}

/*
 * Each thread gets one anonymous mapping holding, from low to high
 * addresses, a guard region, the stack (unless the application
 * provided one), the TLS block and the thread descriptor followed by
 * its thread-specific data values.
 *
 * Returns ENOMEM when the memory can't be had and EAGAIN when another
 * limit, such as the number of threads, is reached; thrd_create tells
 * them apart.
 */
int
_pthread_create(pthread_t * __restrict thread, const pthread_attr_t * __restrict attr,
                void *(*start_routine)(void *), void * __restrict arg)
{
    pthread_attr_t    default_attr;
    size_t            align, tls_size, desc_size, stack_size, guard_size, map_size;
    char             *map;
    uintptr_t         top, stack_top;
    struct __pthread *t;
    char             *tls;
    void            **sp;
    void             *tls_arg;
    char              tls_arg_space[__PTHREAD_TLS_ARG_SIZE] __aligned(sizeof(void *));
    __kernel_sigset_t all, old;
    long              ret;

    if (!attr) {
        pthread_attr_init(&default_attr);
        attr = &default_attr;
    }

    /* Make sure the initial thread has its thread id before any locking */
    (void)_pthread_self();
    _pthread_reap();

    align = _tls_align();
    if (align < alignof(max_align_t))
        align = alignof(max_align_t);
    tls_size = round_up(_tls_size(), align);
    desc_size = round_up(
        sizeof(struct __pthread) + __PTHREAD_KEYS_MAX * sizeof(struct __pthread_specific), align);
    stack_size = 0;
    guard_size = 0;
    if (!attr->__stackaddr) {
        stack_size = attr->__stacksize;
        guard_size = attr->__guardsize;
    }

    /*
     * Sizes too large to add up, with room for rounding each to pages,
     * can't be mapped; don't let them wrap around to small ones
     */
    if (__picolibc_add_overflow(guard_size, stack_size, &map_size)
        || __picolibc_add_overflow(map_size, tls_size + desc_size + align + 3 * PTHREAD_PAGE_SIZE,
                                   &map_size))
        return ENOMEM;

    stack_size = round_up(stack_size, PTHREAD_PAGE_SIZE);
    guard_size = round_up(guard_size, PTHREAD_PAGE_SIZE);
    map_size = round_up(guard_size + stack_size + tls_size + desc_size + align, PTHREAD_PAGE_SIZE);

    map = _pthread_map(map_size);
    if (!map)
        return ENOMEM;
    /* The guard region is best effort */
    if (guard_size)
        (void)_syscall_raw(LINUX_SYS_mprotect, map, guard_size, LINUX_PROT_NONE);

    top = (uintptr_t)map + map_size;
    t = (struct __pthread *)round_down(top - desc_size, align);
    t->specific = (struct __pthread_specific *)(t + 1);
    tls = (char *)round_down((uintptr_t)t - tls_size, align);

    _init_tls(tls);
    tls_arg = _pthread_tls_setup(tls, tls_arg_space);

    if (attr->__stackaddr)
        stack_top = (uintptr_t)attr->__stackaddr;
    else
        stack_top = (uintptr_t)tls;
    stack_top = round_down(stack_top, 16);

    /* _linux_clone pops the start function and its argument from the new stack */
    sp = (void **)stack_top - 2;
    sp[0] = (void *)(uintptr_t)pthread_start;
    sp[1] = t;

    t->start = start_routine;
    t->arg = arg;
    t->map = map;
    t->map_size = map_size;
    t->detach_state = attr->__detachstate;
    t->start_wait = attr->__inheritsched == PTHREAD_EXPLICIT_SCHED;

    /*
     * Block signals until the new thread is running on its own stack;
     * it restores the creator's mask. Take the thread list lock first,
     * so a credential change can't wait on this thread meanwhile.
     */
    pthread_mutex_lock(&_pthread_all_lock);
    memset(&all, 0xff, sizeof(all));
    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_SETMASK, &all, &old,
                       __KERNEL_NSIG_BYTES);
    t->sigmask = old;

    *thread = t;
    __atomic_fetch_add(&_pthread_count, 1, __ATOMIC_RELAXED);
    ret = _linux_clone(LINUX_CLONE_PTHREAD, sp, &t->tid, &t->tid, tls_arg);

    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_SETMASK, &old, NULL,
                       __KERNEL_NSIG_BYTES);

    if (ret >= 0) {
        if (!_pthread_main_listed) {
            _pthread_all_add(&_pthread_main);
            _pthread_main_listed = true;
        }
        _pthread_all_add(t);
    }
    pthread_mutex_unlock(&_pthread_all_lock);

    if (ret < 0) {
        __atomic_fetch_sub(&_pthread_count, 1, __ATOMIC_RELAXED);
        _pthread_unmap(map, map_size);
        return ret == -LINUX_ENOMEM ? ENOMEM : EAGAIN;
    }

    if (t->start_wait) {
        int sched_ret = _pthread_setsched((int)ret, attr->__schedpolicy, attr->__schedpriority);

        t->start_error = sched_ret;
        __atomic_store_n(&t->start_wait, 0, __ATOMIC_RELEASE);
        _futex_wake(&t->start_wait, 1, false);
        if (sched_ret) {
            /* The thread exits without running; reclaim it */
            _pthread_wait_exit(t);
            pthread_mutex_lock(&_pthread_all_lock);
            _pthread_all_remove(t);
            pthread_mutex_unlock(&_pthread_all_lock);
            _pthread_unmap(map, map_size);
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
