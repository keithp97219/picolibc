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

#ifndef _LOCAL_PTHREAD_H_
#define _LOCAL_PTHREAD_H_

#include "local-sigaction.h"
#include "local-time.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>

/* Linux futex operations */
#define LINUX_FUTEX_WAIT             0
#define LINUX_FUTEX_WAKE             1
#define LINUX_FUTEX_WAIT_BITSET      9
#define LINUX_FUTEX_PRIVATE_FLAG     128
#define LINUX_FUTEX_CLOCK_REALTIME   256
#define LINUX_FUTEX_BITSET_MATCH_ANY 0xffffffffU

#define LINUX_FUTEX_LOCK_PI          6
#define LINUX_FUTEX_UNLOCK_PI        7
#define LINUX_FUTEX_LOCK_PI2         13
#define LINUX_FUTEX_TID_MASK         0x3fffffffU

/*
 * Use the 64-bit time futex call where the kernel has one, so that
 * 32-bit targets don't truncate timeouts.
 */
#ifdef LINUX_SYS_futex_time64
#define LINUX_SYS_futex_time_t LINUX_SYS_futex_time64
struct __kernel_futex_timespec {
    __int64_t tv_sec;
    __int64_t tv_nsec;
};
#else
#define LINUX_SYS_futex_time_t  LINUX_SYS_futex
#define __kernel_futex_timespec __kernel_timespec
#endif

/* Linux real-time priorities */
#define LINUX_PRIO_RT_MIN 1
#define LINUX_PRIO_RT_MAX 99
/* Linux clone flags */
#define LINUX_CLONE_VM             0x00000100
#define LINUX_CLONE_FS             0x00000200
#define LINUX_CLONE_FILES          0x00000400
#define LINUX_CLONE_SIGHAND        0x00000800
#define LINUX_CLONE_THREAD         0x00010000
#define LINUX_CLONE_SYSVSEM        0x00040000
#define LINUX_CLONE_SETTLS         0x00080000
#define LINUX_CLONE_PARENT_SETTID  0x00100000
#define LINUX_CLONE_CHILD_CLEARTID 0x00200000

#define __PTHREAD_STACK_DEFAULT    (128 * 1024)
#define __PTHREAD_GUARD_DEFAULT    4096

/* Bytes of parent stack space _pthread_tls_setup may use for the clone TLS argument */
#define __PTHREAD_TLS_ARG_SIZE 16

struct __pthread_specific {
    unsigned int seq;
    void        *value;
};

/* Values of struct __pthread detach_state */
#define __PTHREAD_JOINABLE 0
#define __PTHREAD_DETACHED 1
#define __PTHREAD_EXITING  2

struct __pthread {
    void                      *(*start)(void *);
    void                      *arg;
    void                      *result;

    /* Memory holding this descriptor, its TLS block and possibly its stack */
    void                      *map;
    size_t                     map_size;

    /* Detached threads waiting to be unmapped once they have exited */
    struct __pthread          *zombie_next;

    /* Signal mask for the new thread */
    __kernel_sigset_t          sigmask;

    /* Kernel thread id; the kernel clears it and wakes the futex when the thread exits */
    int                        tid;
    int                        detach_state;

    /* Non-zero while a new thread waits for its creator to set its scheduling */
    unsigned int               start_wait;
    int                        start_error;

    /* Ceilings of the PTHREAD_PRIO_PROTECT mutexes held, and the priority without them */
    unsigned short             tpp_count[LINUX_PRIO_RT_MAX + 1];
    unsigned int               tpp_held;
    int                        tpp_base;

    /* Cancellation */
    struct __pthread_cleanup  *cleanup;
    int                        cancel_state;
    int                        cancel_type;
    int                        cancel_pending;
    int                        cancel_point; /* depth of blocking cancellation points */
    pthread_mutex_t           *cancel_mutex; /* relocked when a condition wait is cancelled */
    unsigned int               cancel_mutex_count;
    struct __pthread_specific *specific;
};

extern __thread struct __pthread *_pthread_current;
extern struct __pthread           _pthread_main;
extern int                        _pthread_count;

static inline struct __pthread *
_pthread_self(void)
{
    struct __pthread *self = _pthread_current;
    if (!self)
        self = pthread_self();
    return self;
}

/*
 * Futex operations. These return 0 or a picolibc errno value and
 * leave errno untouched.
 */
int  _futex_wait(unsigned int *addr, unsigned int val, bool pshared, clockid_t clock,
                 const struct timespec *abstime);
void _futex_wake(unsigned int *addr, int count, bool pshared);

/* Convert an absolute timeout for the kernel: 0, EINVAL or ETIMEDOUT */
int  _futex_timespec(struct __kernel_futex_timespec *kts, const struct timespec *abstime);

/* Priority inheritance futexes; the word holds the owner's thread id */
int  _futex_lock_pi(unsigned int *addr, bool pshared, clockid_t clock,
                    const struct timespec *abstime);
void _futex_unlock_pi(unsigned int *addr, bool pshared);
/*
 * Futex mutex: __state is 0 when unlocked, 1 when locked and 2 when
 * locked with possible waiters. Recursive and error checking mutexes
 * also track the owner's thread id.
 */

static inline bool
_pthread_mutex_pshared(pthread_mutex_t *mutex)
{
    return mutex->__pshared == PTHREAD_PROCESS_SHARED;
}

static inline int
_pthread_mutex_owner(pthread_mutex_t *mutex)
{
    /* Priority inheritance mutexes hold the owner in the futex word */
    if (mutex->__protocol == PTHREAD_PRIO_INHERIT)
        return (int)(__atomic_load_n(&mutex->__state, __ATOMIC_RELAXED) & LINUX_FUTEX_TID_MASK);
    return __atomic_load_n(&mutex->__owner, __ATOMIC_RELAXED);
}

/*
 * Check whether the caller already owns a recursive or error checking
 * mutex. Returns -1 when the lock must still be acquired.
 */
static inline int
_pthread_mutex_relock(pthread_mutex_t *mutex, int tid, int busy)
{
    if (_pthread_mutex_owner(mutex) != tid)
        return -1;
    if (mutex->__type == PTHREAD_MUTEX_RECURSIVE) {
        if (mutex->__count == UINT_MAX)
            return EAGAIN;
        mutex->__count++;
        return 0;
    }
    return busy;
}

/*
 * Sequence-based condition variable: waiters sleep on __seq, which
 * signal and broadcast advance before waking them. A change between
 * reading __seq and sleeping makes the futex wait return at once, so
 * no wakeup is lost.
 */

static inline bool
_pthread_cond_pshared(pthread_cond_t *cond)
{
    return cond->__pshared == PTHREAD_PROCESS_SHARED;
}

/*
 * Priority protection: count the ceilings of the PTHREAD_PRIO_PROTECT
 * mutexes a thread holds and run a real-time thread at the highest.
 */
int  _pthread_tpp_raise(struct __pthread *self, int ceiling);
void _pthread_tpp_lower(struct __pthread *self, int ceiling);

/* Unlock, lowering the thread's priority from ceiling for PTHREAD_PRIO_PROTECT */
int  _pthread_mutex_unlock_ceiling(pthread_mutex_t *mutex, int ceiling);

int  _pthread_mutex_lock_timed(pthread_mutex_t *mutex, clockid_t clock,
                               const struct timespec *abstime);
int  _pthread_cond_wait_timed(pthread_cond_t *cond, pthread_mutex_t *mutex, clockid_t clock,
                              const struct timespec *abstime);

/*
 * Reader-writer lock: __state counts readers, or holds
 * _PTHREAD_RWLOCK_WRITER while write locked. Waiters register in
 * __waiters and sleep on __seq, which unlock advances when there are
 * waiters. Readers are preferred, so a thread can always take
 * another read lock.
 */
#define _PTHREAD_RWLOCK_WRITER UINT_MAX

static inline bool
_pthread_rwlock_pshared(pthread_rwlock_t *rwlock)
{
    return rwlock->__pshared == PTHREAD_PROCESS_SHARED;
}

/* Try to take the lock once: 0 on success, EBUSY, EAGAIN or EDEADLK otherwise */
static inline int
_pthread_rwlock_try(pthread_rwlock_t *rwlock, bool write, int tid)
{
    unsigned int state = __atomic_load_n(&rwlock->__state, __ATOMIC_RELAXED);

    if (__atomic_load_n(&rwlock->__writer, __ATOMIC_RELAXED) == tid)
        return EDEADLK;
    for (;;) {
        if (write) {
            if (state != 0)
                return EBUSY;
        } else {
            if (state == _PTHREAD_RWLOCK_WRITER)
                return EBUSY;
            if (state == _PTHREAD_RWLOCK_WRITER - 1)
                return EAGAIN;
        }
        if (__atomic_compare_exchange_n(&rwlock->__state, &state,
                                        write ? _PTHREAD_RWLOCK_WRITER : state + 1, false,
                                        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            if (write)
                __atomic_store_n(&rwlock->__writer, tid, __ATOMIC_RELAXED);
            return 0;
        }
    }
}

int _pthread_rwlock_lock_timed(pthread_rwlock_t *rwlock, bool write, clockid_t clock,
                               const struct timespec *abstime);

/*
 * Semaphores: waiters register in __waiters and sleep on __value while
 * it is zero; sem_post wakes one when there are waiters. Returns 0 or
 * an errno value.
 */
int _sem_wait_timed(sem_t *sem, clockid_t clock, const struct timespec *abstime);

/* Named semaphores mapped by this process */
struct _sem_named {
    struct _sem_named *next;
    sem_t             *sem;
    __dev_t            dev;
    __ino_t            ino;
    unsigned int       refs;
};

extern pthread_mutex_t    _sem_named_lock;
extern struct _sem_named *_sem_named;

/* Scheduling for a thread id; these return 0 or an errno value */
int                       _pthread_setsched(int tid, int policy, int priority);
int                       _pthread_getsched(int tid, int *policy, int *priority);

extern int                _pthread_concurrency;

/*
 * Cancellation uses a Linux real-time signal which picolibc's signal
 * numbers never reach, so applications can't block or catch it.
 */
#define _PTHREAD_SIGCANCEL 32

__noreturn void _pthread_cancel_act(struct __pthread *self);

/* Run the calling thread's cleanup handlers */
void            _pthread_cleanup_exit(struct __pthread *self);

/* Queue an exiting detached thread to be unmapped once the kernel is done with it */
void            _pthread_zombie(struct __pthread *thread);

/* Thread-specific data keys; seq is odd while the key is in use */
struct _pthread_key {
    unsigned int seq;
    void         (*destructor)(void *);
};

extern struct _pthread_key _pthread_keys[__PTHREAD_KEYS_MAX];
extern pthread_mutex_t     _pthread_keys_lock;

static inline unsigned int
_pthread_key_seq(pthread_key_t key)
{
    return __atomic_load_n(&_pthread_keys[key].seq, __ATOMIC_ACQUIRE);
}

/* Detached threads which have exited but are not yet unmapped */
extern pthread_mutex_t   _pthread_zombie_lock;
extern struct __pthread *_pthread_zombies;

/* Unmap detached threads which have exited */
void                     _pthread_reap(void);

/* Wait until the kernel has finished with an exiting thread */
void                     _pthread_wait_exit(struct __pthread *thread);

/* Run thread-specific data destructors for the calling thread */
void                     _pthread_specific_exit(struct __pthread *self);

/*
 * Machine-specific pieces
 */

/*
 * Raw clone(2) call. In the child, pops a function and its argument
 * from the new stack, calls the function and exits the thread with
 * its return value. Returns the child thread id or a negative Linux
 * error code.
 */
long  _linux_clone(unsigned long flags, void *stack, int *ptid, int *ctid, void *tls);

/*
 * Initialize the thread control block for a new TLS block (already
 * filled by _init_tls) and return the value to pass as the clone TLS
 * argument. arg_space provides __PTHREAD_TLS_ARG_SIZE bytes for any
 * structure the kernel needs to read during clone.
 */
void *_pthread_tls_setup(void *tls, void *arg_space);

#endif /* _LOCAL_PTHREAD_H_ */
