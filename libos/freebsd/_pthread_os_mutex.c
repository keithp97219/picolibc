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

#include "local-umutex.h"
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-umtx_time-struct.h>
#include <freebsd/freebsd-umtx_robust_lists_params-struct.h>
#include <stdlib.h>

/*
 * local-pthread.h's OS mutexes, priority inheritance and robust ones,
 * on the struct umutex beside each (local-umutex.h). The owner word
 * holds the owner's thread id, with UMUTEX_CONTESTED set while others
 * may wait in the kernel; the uncontended cases are handled here, the
 * rest by _umtx_op. When a robust mutex's owner dies, the kernel sets
 * the word to UMUTEX_RB_OWNERDEAD and wakes a waiter. Its next owner
 * marks it UMUTEX_NONCONSISTENT until pthread_mutex_consistent; an
 * unlock while it is still marked leaves UMUTEX_RB_NOTRECOV, which
 * no thread can lock again.
 */

static int
umtx_op(struct _pthread_umutex *um, int op, unsigned long val, void *uaddr, void *uaddr2)
{
    long ret = _syscall_raw(FREEBSD_SYS__umtx_op, &um->um, op, val, uaddr, uaddr2);

    if (ret < 0)
        return _syscall_errno(ret);
    return 0;
}

static __uint32_t
umutex_flags(pthread_mutex_t *mutex)
{
    __uint32_t flags = 0;

    if (mutex->__protocol == PTHREAD_PRIO_INHERIT)
        flags |= FREEBSD_UMUTEX_PRIO_INHERIT;
    if (mutex->__robust)
        flags |= FREEBSD_UMUTEX_ROBUST;
    if (_pthread_mutex_pshared(mutex))
        flags |= FREEBSD_USYNC_PROCESS_SHARED;
    return flags;
}

/* Give the kernel the thread's robust list, once */
static void
robust_register(struct __pthread *self)
{
    struct __freebsd_umtx_robust_lists_params params;

    if (self->os.robust_registered)
        return;
    memset(&params, 0, sizeof(params));
    params.robust_list_offset = (__uintptr_t)&self->os.robust_list;
    params.robust_inact_offset = (__uintptr_t)&self->os.robust_inact;
    if (_syscall_raw(FREEBSD_SYS__umtx_op, NULL, FREEBSD_UMTX_OP_ROBUST_LISTS, (long)sizeof(params),
                     &params, NULL)
        == 0)
        self->os.robust_registered = true;
}

/* Name the mutex being locked or unlocked to the kernel, should the thread die meanwhile */
static void
robust_inact(struct __pthread *self, struct _pthread_umutex *um)
{
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    __atomic_store_n(&self->os.robust_inact, (__uintptr_t)um, __ATOMIC_RELAXED);
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
}

static struct _pthread_umutex *
robust_first(struct __pthread *self)
{
    return (struct _pthread_umutex *)self->os.robust_list;
}

static struct _pthread_umutex *
robust_next(struct _pthread_umutex *um)
{
    return (struct _pthread_umutex *)um->um.m_rb_lnk;
}

/* Add a mutex just locked to the front of the thread's list */
static void
robust_link(struct __pthread *self, pthread_mutex_t *mutex, struct _pthread_umutex *um)
{
    struct _pthread_umutex *first = robust_first(self);

    um->mutex = mutex;
    um->rb_prev = NULL;
    um->um.m_rb_lnk = (__uintptr_t)first;
    if (first)
        first->rb_prev = um;
    /* The kernel may walk the list should the thread die; publish the mutex last */
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    self->os.robust_list = (__uintptr_t)um;
}

static void
robust_unlink(struct __pthread *self, struct _pthread_umutex *um)
{
    struct _pthread_umutex *next = robust_next(um);

    if (um->rb_prev)
        um->rb_prev->um.m_rb_lnk = (__uintptr_t)next;
    else
        self->os.robust_list = (__uintptr_t)next;
    if (next)
        next->rb_prev = um->rb_prev;
}

int
_pthread_os_mutex_init(pthread_mutex_t *mutex)
{
    struct _pthread_umutex *um;
    __uint32_t              flags = umutex_flags(mutex);

    /* A robust mutex the kernel can't report would only hang */
    if (mutex->__robust) {
        struct __pthread *self = _pthread_self();

        robust_register(self);
        if (!self->os.robust_registered)
            return ENOTSUP;
    }
    if (_pthread_mutex_pshared(mutex))
        return _pthread_umutex_shared_create(mutex, flags);
    um = calloc(1, sizeof(*um));
    if (!um)
        return ENOMEM;
    um->um.m_flags = flags;
    mutex->__list_next = um;
    return 0;
}

int
_pthread_os_mutex_destroy(pthread_mutex_t *mutex)
{
    struct _pthread_umutex *um;
    __uint32_t              owner;
    int                     error;

    um = _pthread_umutex(mutex, &error);
    if (!um)
        return 0;
    /* Destroying a mutex whose owner died, or which can't be recovered, is fine */
    owner = __atomic_load_n(&um->um.m_owner, __ATOMIC_RELAXED);
    if (owner != _UMUTEX_UNOWNED && owner != _UMUTEX_OWNERDEAD && owner != _UMUTEX_NOTRECOV)
        return EBUSY;
    if (_pthread_mutex_pshared(mutex)) {
        _pthread_umutex_shared_destroy(mutex, um);
    } else {
        mutex->__list_next = NULL;
        free(um);
    }
    return 0;
}

int
_pthread_os_mutex_owner(pthread_mutex_t *mutex)
{
    struct _pthread_umutex *um;
    __uint32_t              owner;
    int                     error;

    um = _pthread_umutex(mutex, &error);
    if (!um)
        return 0;
    owner = __atomic_load_n(&um->um.m_owner, __ATOMIC_RELAXED);
    if (owner == _UMUTEX_OWNERDEAD || owner == _UMUTEX_NOTRECOV)
        return 0;
    return (int)(owner & ~_UMUTEX_CONTESTED);
}

/* The kernel's timeout for a lock, or ETIMEDOUT or EINVAL */
static int
umtx_timeout(struct __freebsd_umtx_time *ut, clockid_t clock, const struct timespec *abstime)
{
    if (clock != CLOCK_REALTIME && clock != CLOCK_MONOTONIC)
        return EINVAL;
    if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000L)
        return EINVAL;
    /*
     * The kernel waits forever for an absolute time of zero on clocks
     * other than CLOCK_REALTIME, but every clock has passed that
     * already
     */
    if (abstime->tv_sec < 0 || (abstime->tv_sec == 0 && abstime->tv_nsec == 0))
        return ETIMEDOUT;
    memset(ut, 0, sizeof(*ut));
    ut->_timeout.tv_sec = abstime->tv_sec;
    ut->_timeout.tv_nsec = abstime->tv_nsec;
    ut->_flags = FREEBSD_UMTX_ABSTIME;
    ut->_clockid = (__uint32_t)_clock_to_freebsd(clock);
    return 0;
}

static int
umutex_lock(struct _pthread_umutex *um, __uint32_t id, clockid_t clock,
            const struct timespec *abstime, bool trylock)
{
    struct __freebsd_umtx_time ut;
    void                      *size = NULL, *put = NULL;
    __uint32_t                 flags = __atomic_load_n(&um->um.m_flags, __ATOMIC_RELAXED);
    bool                       pi = (flags & FREEBSD_UMUTEX_PRIO_INHERIT) != 0;
    __uint32_t                 owner = __atomic_load_n(&um->um.m_owner, __ATOMIC_RELAXED);
    int                        ret;

    for (;;) {
        /*
         * Take a free mutex, keeping the contested bit for the waiters;
         * the kernel tracks the waiters of a contested priority
         * inheritance mutex, so it must hand that over
         */
        if (owner == _UMUTEX_UNOWNED || (owner == _UMUTEX_CONTESTED && !pi)) {
            if (__atomic_compare_exchange_n(&um->um.m_owner, &owner, owner | id, false,
                                            __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return 0;
            continue;
        }
        if (owner == _UMUTEX_OWNERDEAD) {
            if (__atomic_compare_exchange_n(&um->um.m_owner, &owner, id | _UMUTEX_CONTESTED, false,
                                            __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
                return EOWNERDEAD;
            continue;
        }
        if (owner == _UMUTEX_NOTRECOV)
            return ENOTRECOVERABLE;
        if ((owner & ~_UMUTEX_CONTESTED) == id)
            return trylock ? EBUSY : EDEADLK;
        break;
    }

    if (trylock) {
        if ((owner & ~_UMUTEX_CONTESTED) != 0)
            return EBUSY;
        return umtx_op(um, FREEBSD_UMTX_OP_MUTEX_TRYLOCK, 0, NULL, NULL);
    }

    if (abstime) {
        ret = umtx_timeout(&ut, clock, abstime);
        if (ret)
            return ret;
        /* The size of the timeout structure goes in uaddr */
        size = (void *)(uintptr_t)sizeof(ut);
        put = &ut;
    }
    /* The kernel doesn't restart a timed lock interrupted by a signal */
    do
        ret = umtx_op(um, FREEBSD_UMTX_OP_MUTEX_LOCK, 0, size, put);
    while (ret == EINTR);

    /*
     * Making the mutex unrecoverable woke one waiter; this one wakes
     * the next, so each learns of it in turn
     */
    if (ret == ENOTRECOVERABLE)
        (void)umtx_op(um, FREEBSD_UMTX_OP_MUTEX_WAKE2, flags, NULL, NULL);
    return ret;
}

static void
umutex_unlock(struct _pthread_umutex *um, __uint32_t id)
{
    __uint32_t flags = __atomic_load_n(&um->um.m_flags, __ATOMIC_RELAXED);
    __uint32_t owner = id;

    /* Without waiters, release the mutex here; otherwise the kernel hands it on */
    if (!__atomic_compare_exchange_n(&um->um.m_owner, &owner,
                                     (flags & FREEBSD_UMUTEX_NONCONSISTENT) ? _UMUTEX_NOTRECOV
                                                                            : _UMUTEX_UNOWNED,
                                     false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
        (void)umtx_op(um, FREEBSD_UMTX_OP_MUTEX_UNLOCK, 0, NULL, NULL);
}

int
_pthread_os_mutex_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                       const struct timespec *abstime, bool trylock)
{
    struct _pthread_umutex *um;
    bool                    robust = mutex->__robust != 0;
    int                     ret;

    um = _pthread_umutex(mutex, &ret);
    if (!um)
        return ret;
    if (robust) {
        robust_register(self);
        robust_inact(self, um);
    }
    ret = umutex_lock(um, (__uint32_t)self->tid, clock, abstime, trylock);
    if (ret == 0 || ret == EOWNERDEAD) {
        /* A recursive mutex whose owner died keeps none of its depth */
        mutex->__count = 0;
        if (robust) {
            /* The owner died holding it; the caller must make it consistent */
            if (ret == EOWNERDEAD)
                __atomic_fetch_or(&um->um.m_flags, FREEBSD_UMUTEX_NONCONSISTENT, __ATOMIC_RELAXED);
            robust_link(self, mutex, um);
        }
    }
    if (robust)
        robust_inact(self, NULL);
    return ret;
}

void
_pthread_os_mutex_unlock(pthread_mutex_t *mutex, struct __pthread *self)
{
    struct _pthread_umutex *um;
    int                     error;

    um = _pthread_umutex(mutex, &error);
    if (!um)
        return;
    if (mutex->__robust) {
        robust_inact(self, um);
        robust_unlink(self, um);
    }
    umutex_unlock(um, (__uint32_t)self->tid);
    if (mutex->__robust)
        robust_inact(self, NULL);
}

int
_pthread_os_mutex_consistent(pthread_mutex_t *mutex)
{
    struct _pthread_umutex *um;
    int                     error;

    um = _pthread_umutex(mutex, &error);
    if (!um)
        return error;
    if (!(__atomic_load_n(&um->um.m_flags, __ATOMIC_RELAXED) & FREEBSD_UMUTEX_NONCONSISTENT))
        return EINVAL;
    __atomic_fetch_and(&um->um.m_flags, ~(__uint32_t)FREEBSD_UMUTEX_NONCONSISTENT,
                       __ATOMIC_RELAXED);
    return 0;
}

/* The fork child's only thread holds none of the robust mutexes its parent's did */
void
_pthread_os_mutex_fork_child(struct __pthread *self)
{
    _pthread_umutex_shared_fork_child();
    self->os.robust_list = 0;
    self->os.robust_inact = 0;
    if (self->os.robust_registered) {
        self->os.robust_registered = false;
        robust_register(self);
    }
}

int
_pthread_os_mutex_ceiling(struct __pthread *self)
{
    int                     max = _PTHREAD_TPP_NONE;
    struct _pthread_umutex *um;

    for (um = robust_first(self); um; um = robust_next(um))
        if (um->mutex->__protocol == PTHREAD_PRIO_PROTECT && um->mutex->__prioceiling > max)
            max = um->mutex->__prioceiling;
    return max;
}
