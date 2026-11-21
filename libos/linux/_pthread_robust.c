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

#include "local-pthread.h"
#include <stddef.h>
#include <stdint.h>

/*
 * The kernel's robust list links the __list_next members of the
 * mutexes a thread holds. Bit 0 of each link marks the entry as a
 * priority inheritance futex.
 * __list_prev points back at the link naming the mutex so that
 * unlocking needn't search the list; the kernel ignores it.
 */
#define ROBUST_PI 1

static bool
robust_pi(pthread_mutex_t *mutex)
{
    return mutex->__protocol == PTHREAD_PRIO_INHERIT;
}

static void *
robust_node(pthread_mutex_t *mutex)
{
    return (void *)((uintptr_t)&mutex->__list_next | (robust_pi(mutex) ? ROBUST_PI : 0));
}

static void
robust_register(struct __pthread *self)
{
    if (self->os.robust_registered)
        return;
    self->os.robust.list = &self->os.robust.list;
    self->os.robust.futex_offset
        = (long)offsetof(pthread_mutex_t, __state) - (long)offsetof(pthread_mutex_t, __list_next);
    self->os.robust.list_op_pending = NULL;
    if (_syscall_raw(LINUX_SYS_set_robust_list, &self->os.robust, sizeof(self->os.robust)) == 0)
        self->os.robust_registered = true;
}

/*
 * Whether the kernel will report a robust mutex's owner dying; it
 * won't without a robust list, as under QEMU's user-mode emulation
 */
bool
_pthread_robust_available(void)
{
    struct __pthread *self = _pthread_self();

    robust_register(self);
    return self->os.robust_registered;
}

/* After fork, the child's only thread holds no robust mutexes */
void
_pthread_robust_reset(struct __pthread *self)
{
    if (!self->os.robust_registered)
        return;
    self->os.robust_registered = false;
    robust_register(self);
}

/* The mutex a list link names, or NULL for the list head */
static pthread_mutex_t *
robust_entry(void *link, struct __pthread *self)
{
    void **next = (void **)((uintptr_t)link & ~(uintptr_t)ROBUST_PI);

    if (next == &self->os.robust.list)
        return NULL;
    return (pthread_mutex_t *)((char *)next - offsetof(pthread_mutex_t, __list_next));
}

/* Add the mutex to the front of the thread's list */
static void
robust_link(pthread_mutex_t *mutex, struct __pthread *self)
{
    pthread_mutex_t *first = robust_entry(self->os.robust.list, self);

    mutex->__list_next = self->os.robust.list;
    mutex->__list_prev = &self->os.robust.list;
    if (first)
        first->__list_prev = &mutex->__list_next;
    /* The kernel may walk the list should the thread die; publish the mutex last */
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    self->os.robust.list = robust_node(mutex);
}

/*
 * Without priority inheritance, the futex holds the owner's thread id
 * and FUTEX_WAITERS while threads may be waiting. When an owner dies,
 * the kernel clears the id, sets FUTEX_OWNER_DIED and wakes a waiter.
 * It does that with a shared futex wake, so these futexes are always
 * used as shared ones.
 */
static int
robust_lock_plain(pthread_mutex_t *mutex, unsigned int tid, clockid_t clock,
                  const struct timespec *abstime, bool trylock)
{
    unsigned int state = __atomic_load_n(&mutex->__state, __ATOMIC_RELAXED);
    unsigned int waiters = 0;
    int          ret;

    for (;;) {
        if ((state & LINUX_FUTEX_TID_MASK) == 0) {
            /* Having waited, others may be waiting too */
            if (__atomic_compare_exchange_n(&mutex->__state, &state,
                                            tid | waiters | (state & LINUX_FUTEX_WAITERS), false,
                                            __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return (state & LINUX_FUTEX_OWNER_DIED) ? EOWNERDEAD : 0;
            continue;
        }
        if (trylock)
            return EBUSY;
        if ((state & LINUX_FUTEX_TID_MASK) == tid)
            return EDEADLK;
        if (!(state & LINUX_FUTEX_WAITERS)) {
            if (!__atomic_compare_exchange_n(&mutex->__state, &state, state | LINUX_FUTEX_WAITERS,
                                             false, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
                continue;
            state |= LINUX_FUTEX_WAITERS;
        }
        ret = _futex_wait(&mutex->__state, state, true, clock, abstime);
        if (ret == ETIMEDOUT || ret == EINVAL)
            return ret;
        waiters = LINUX_FUTEX_WAITERS;
        state = __atomic_load_n(&mutex->__state, __ATOMIC_RELAXED);
    }
}

static int
robust_lock_pi(pthread_mutex_t *mutex, unsigned int tid, clockid_t clock,
               const struct timespec *abstime, bool trylock)
{
    bool         pshared = _pthread_mutex_pshared(mutex);
    unsigned int state = 0;
    int          ret;

    if (__atomic_compare_exchange_n(&mutex->__state, &state, tid, false, __ATOMIC_ACQUIRE,
                                    __ATOMIC_RELAXED)) {
        ret = 0;
    } else if (trylock) {
        int op = LINUX_FUTEX_TRYLOCK_PI | (pshared ? 0 : LINUX_FUTEX_PRIVATE_FLAG);

        ret = (int)_syscall_raw(LINUX_SYS_futex_time_t, &mutex->__state, op, 0, NULL, NULL, 0);
        if (ret) {
            ret = _syscall_errno(ret);
            if (ret == EAGAIN || ret == EDEADLK)
                ret = EBUSY;
        }
    } else {
        ret = _futex_lock_pi(&mutex->__state, pshared, clock, abstime);
    }
    /* The kernel leaves the owner-died bit set for the next owner */
    if (ret == 0 && (__atomic_load_n(&mutex->__state, __ATOMIC_RELAXED) & LINUX_FUTEX_OWNER_DIED))
        ret = EOWNERDEAD;
    return ret;
}

static void
robust_release(pthread_mutex_t *mutex, struct __pthread *self)
{
    unsigned int     state = (unsigned int)self->tid;
    pthread_mutex_t *next = robust_entry(mutex->__list_next, self);

    /* Unlink the mutex from the thread's list */
    if (next)
        next->__list_prev = mutex->__list_prev;
    *(void **)mutex->__list_prev = mutex->__list_next;
    if (robust_pi(mutex)) {
        /* With waiters, or the owner-died bit, the kernel hands the lock on */
        if (!__atomic_compare_exchange_n(&mutex->__state, &state, 0, false, __ATOMIC_RELEASE,
                                         __ATOMIC_RELAXED))
            _futex_unlock_pi(&mutex->__state, _pthread_mutex_pshared(mutex));
    } else if (__atomic_exchange_n(&mutex->__state, 0, __ATOMIC_RELEASE) & LINUX_FUTEX_WAITERS) {
        _futex_wake(&mutex->__state, 1, true);
    }
}

int
_pthread_robust_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                     const struct timespec *abstime, bool trylock)
{
    unsigned int tid = (unsigned int)self->tid;
    int          ret;

    robust_register(self);
    /* Tells the kernel about the mutex while it is being locked */
    self->os.robust.list_op_pending = robust_node(mutex);
    __atomic_signal_fence(__ATOMIC_SEQ_CST);

    if (robust_pi(mutex))
        ret = robust_lock_pi(mutex, tid, clock, abstime, trylock);
    else
        ret = robust_lock_plain(mutex, tid, clock, abstime, trylock);

    if (ret == 0 || ret == EOWNERDEAD) {
        /* A recursive mutex whose owner died keeps none of its depth */
        mutex->__count = 0;
        robust_link(mutex, self);
        if (mutex->__robust_state == _PTHREAD_ROBUST_NOTRECOVERABLE) {
            robust_release(mutex, self);
            ret = ENOTRECOVERABLE;
        } else if (ret == EOWNERDEAD) {
            /* The owner died holding it; the caller must make it consistent */
            mutex->__robust_state = _PTHREAD_ROBUST_INCONSISTENT;
        }
    }
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    self->os.robust.list_op_pending = NULL;
    return ret;
}

void
_pthread_robust_unlock(pthread_mutex_t *mutex, struct __pthread *self)
{
    /* Unlocked without being made consistent, it can never be used again */
    if (mutex->__robust_state == _PTHREAD_ROBUST_INCONSISTENT)
        mutex->__robust_state = _PTHREAD_ROBUST_NOTRECOVERABLE;
    self->os.robust.list_op_pending = robust_node(mutex);
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    robust_release(mutex, self);
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    self->os.robust.list_op_pending = NULL;
}

int
_pthread_robust_ceiling(struct __pthread *self)
{
    int              max = _PTHREAD_TPP_NONE;
    pthread_mutex_t *m;

    /* The list exists once the thread has locked a robust mutex */
    if (!self->os.robust.list)
        return max;
    for (m = robust_entry(self->os.robust.list, self); m; m = robust_entry(m->__list_next, self))
        if (m->__protocol == PTHREAD_PRIO_PROTECT && m->__prioceiling > max)
            max = m->__prioceiling;
    return max;
}
