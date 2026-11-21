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

/*
 * POSIX threads for picolibc, shared by the OS layers which provide
 * them (libos/linux, libos/freebsd, and OS layers built elsewhere,
 * such as Zephyr's). This is an internal interface: it may change
 * with picolibc, and the OS layer is built with the picolibc it
 * belongs to.
 *
 * The types come from <sys/_pthread.h> in libos/pthread/include,
 * which every such OS layer uses. Each OS layer provides
 * local-pthread-os.h, included first below (it may set feature
 * macros), defining:
 *
 * Required, always:
 *
 *   struct __pthread_os      Per-thread OS data, embedded in each
 *                            descriptor as 'os' (at least one member).
 *
 *   _pthread_exit_word(t)    The futex word (unsigned int *) which the
 *   _PTHREAD_EXIT_DONE       OS sets to _PTHREAD_EXIT_DONE, and wakes
 *                            with _futex_wake semantics (pshared), once
 *                            thread t has exited and no longer uses
 *                            its stack, TLS or descriptor. Joining
 *                            and reclaiming detached threads wait on
 *                            it. Before then it must read differently.
 *
 *   and these functions, declared below:
 *
 *   _futex_wait, _futex_wake  Wait on and wake a 32-bit word, as
 *                            described below.
 *   _pthread_os_stack_alloc, Allocate and free the stacks picolibc
 *   _pthread_os_stack_free   provides, unless _PTHREAD_HAVE_MAP selects
 *                            the default (_pthread_alloc.c). Each
 *                            thread's descriptor and TLS block sit at
 *                            the top of its stack region, whether
 *                            picolibc or the application provided it.
 *   _pthread_os_create       Start a thread running _pthread_start.
 *   _pthread_os_exit         End the calling thread.
 *   _pthread_os_set_tid      Give the initial thread its id.
 *
 * Thread ids: t->tid is a non-zero 32-bit value the OS layer assigns
 * (in _pthread_os_create and _pthread_os_set_tid), unique among live
 * threads. Mutex, rwlock and call_once words hold it to name owners.
 * It need not be a kernel thread id unless the OS has PI or robust
 * futexes, whose kernel reads it. After the thread exits it must read
 * zero (the kernel or _pthread_os_exit clears it): pthread_kill and
 * the scheduling functions treat zero as an exited thread.
 *
 * Optional capabilities, each enabled by defining the macro in
 * local-pthread-os.h. Without one, its code compiles out and the
 * functions depending on it fail as POSIX allows:
 *
 *   _PTHREAD_HAVE_MAP        _pthread_map, _pthread_unmap and
 *                            _pthread_guard (anonymous memory, an
 *                            inaccessible guard) give the default
 *                            stack allocator.
 *   _PTHREAD_HAVE_RELEASE    _pthread_os_release frees OS-private
 *                            per-thread objects, which may live outside
 *                            the stack (kernel memory, say) and be
 *                            reached through struct __pthread_os.
 *   _PTHREAD_HAVE_SIGNALS    Per-thread signal masks and directed
 *                            signals: _pthread_sigset_t,
 *                            _pthread_sigblock_all, _pthread_sigsetmask,
 *                            _pthread_sigcancel_unblock,
 *                            _pthread_signal_to_kernel, _pthread_tkill,
 *                            _pthread_os_sigcancel, _sigprocmask and
 *                            _PTHREAD_SIGCANCEL, a kernel signal
 *                            applications can't name. This gives
 *                            pthread_kill, pthread_sigmask, the
 *                            asynchronous cancellation type and the
 *                            interruption of blocked threads by
 *                            pthread_cancel. Without it, those report
 *                            ENOTSUP, and cancellation is deferred and
 *                            acted on at the next cancellation point.
 *   _PTHREAD_HAVE_SCHED      _pthread_setsched, _pthread_getsched,
 *                            _pthread_policy_valid, and
 *                            _PTHREAD_PRIO_MIN/_PTHREAD_PRIO_MAX, the
 *                            real-time priority range. This gives
 *                            thread scheduling and PTHREAD_PRIO_PROTECT
 *                            mutexes; without it they report ENOTSUP
 *                            and only SCHED_OTHER is accepted.
 *   _PTHREAD_HAVE_PI         _futex_lock_pi, _futex_unlock_pi and
 *                            _pthread_pi_available: PTHREAD_PRIO_INHERIT.
 *   _PTHREAD_HAVE_ROBUST     The _pthread_robust_ functions: robust
 *                            mutexes. PI and robust mutexes keep the
 *                            owner's tid in the futex word, under
 *                            _PTHREAD_FUTEX_TID_MASK, with
 *                            _PTHREAD_FUTEX_OWNER_DIED set by the
 *                            kernel when the owner died.
 *   _PTHREAD_HAVE_PSHARED    Futex words in shared memory work across
 *                            processes: PTHREAD_PROCESS_SHARED objects
 *                            and process-shared semaphores. Without it
 *                            those report ENOTSUP.
 *   _PTHREAD_HAVE_FORK       The OS has fork, which calls
 *                            _pthread_fork_prepare, _pthread_fork_parent
 *                            and _pthread_fork_child (or, for _Fork,
 *                            _pthread_fork_reset in the child).
 *   _PTHREAD_HAVE_SEM_NAMED  sem_open and sem_unlink, using open, link,
 *                            unlink, fstat and mmap on files that
 *                            local-shm.h names. Without it they report
 *                            ENOSYS.
 *
 * Waits take a clock and an absolute timeout. An OS layer converting
 * them to relative timeouts may do so: callers loop, recheck their
 * condition after every return, and pass the same absolute time
 * again, so a wait may return early (0 or EINTR) but must return
 * ETIMEDOUT only once the time has passed on that clock.
 */
#include "local-pthread-os.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>

#ifndef _PTHREAD_HAVE_SIGNALS
typedef char _pthread_sigset_t;
#define _pthread_sigblock_all(old)   ((void)(old))
#define _pthread_sigsetmask(set)     ((void)(set))
#define _pthread_sigcancel_unblock() ((void)0)
#endif

#ifndef _PTHREAD_HAVE_SCHED
#define _PTHREAD_PRIO_MIN 0
#define _PTHREAD_PRIO_MAX 0
#endif

#if !defined(_PTHREAD_HAVE_PI) && !defined(_PTHREAD_HAVE_ROBUST)
#define _PTHREAD_FUTEX_TID_MASK   UINT_MAX
#define _PTHREAD_FUTEX_OWNER_DIED 0U
#endif

#define __PTHREAD_STACK_DEFAULT (128 * 1024)
#define __PTHREAD_GUARD_DEFAULT 4096

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

    /*
     * The stack region picolibc allocated, holding this descriptor and
     * its TLS block at the top; map is NULL for the initial thread and
     * for threads on application stacks
     */
    void                      *map;
    size_t                     map_size;
    /* Where the new thread's stack starts and its TLS block, for _pthread_os_create */
    void                      *stack_top;
    void                      *tls;

    /* Detached threads waiting to be unmapped once they have exited */
    struct __pthread          *zombie_next;

    /* The running threads, for changing every thread's credentials */
    struct __pthread          *all_next;
    struct __pthread          *all_prev;
    unsigned int               setxid_pending;

    /* Signal mask for the new thread */
    _pthread_sigset_t          sigmask;

    /* Thread id, set by the OS layer and zero once the thread has exited */
    int                        tid;
    int                        detach_state;

    /* Non-zero while a new thread waits for its creator to set its scheduling */
    unsigned int               start_wait;
    int                        start_error;

    /* Ceilings of the PTHREAD_PRIO_PROTECT mutexes held, and the priority without them */
    unsigned short             tpp_count[_PTHREAD_PRIO_MAX + 1];
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

    /* What the OS layer keeps for each thread */
    struct __pthread_os        os;
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
 * leave errno untouched. _futex_wait sleeps while *addr holds val,
 * until woken, the absolute time abstime on clock (CLOCK_REALTIME or
 * CLOCK_MONOTONIC, else EINVAL) passes (ETIMEDOUT), or spuriously
 * (0 or EINTR); it returns at once if *addr differs. A timespec with
 * tv_nsec out of range gives EINVAL. _futex_wake wakes up to count
 * waiters. pshared says whether the word may be shared with other
 * processes, and is the same for every wait and wake on a word.
 */
int  _futex_wait(unsigned int *addr, unsigned int val, bool pshared, clockid_t clock,
                 const struct timespec *abstime);
void _futex_wake(unsigned int *addr, int count, bool pshared);

/* Priority inheritance futexes; the word holds the owner's thread id */
#ifdef _PTHREAD_HAVE_PI
int  _futex_lock_pi(unsigned int *addr, bool pshared, clockid_t clock,
                    const struct timespec *abstime);
void _futex_unlock_pi(unsigned int *addr, bool pshared);

/* Whether the kernel has priority inheritance futexes */
bool _pthread_pi_available(void);
#else
static inline int
_futex_lock_pi(unsigned int *addr, bool pshared, clockid_t clock, const struct timespec *abstime)
{
    (void)addr;
    (void)pshared;
    (void)clock;
    (void)abstime;
    return ENOTSUP;
}

static inline void
_futex_unlock_pi(unsigned int *addr, bool pshared)
{
    (void)addr;
    (void)pshared;
}

static inline bool
_pthread_pi_available(void)
{
    return false;
}
#endif

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

/* Robust mutex states */
#define _PTHREAD_ROBUST_CONSISTENT     0
#define _PTHREAD_ROBUST_INCONSISTENT   1
#define _PTHREAD_ROBUST_NOTRECOVERABLE 2

/*
 * Robust mutexes keep the owner's thread id in the futex word, as
 * priority inheritance ones do, so the kernel can mark them when the
 * owner dies. Locking returns 0, EOWNERDEAD, ENOTRECOVERABLE or an
 * error; unlocking assumes the caller owns the mutex.
 */
#ifdef _PTHREAD_HAVE_ROBUST
int  _pthread_robust_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                          const struct timespec *abstime, bool trylock);
void _pthread_robust_unlock(pthread_mutex_t *mutex, struct __pthread *self);
void _pthread_robust_reset(struct __pthread *self);
bool _pthread_robust_available(void);
#else
static inline int
_pthread_robust_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                     const struct timespec *abstime, bool trylock)
{
    (void)mutex;
    (void)self;
    (void)clock;
    (void)abstime;
    (void)trylock;
    return ENOTSUP;
}

static inline void
_pthread_robust_unlock(pthread_mutex_t *mutex, struct __pthread *self)
{
    (void)mutex;
    (void)self;
}

static inline void
_pthread_robust_reset(struct __pthread *self)
{
    (void)self;
}

static inline bool
_pthread_robust_available(void)
{
    return false;
}
#endif

static inline int
_pthread_mutex_owner(pthread_mutex_t *mutex)
{
    /* Priority inheritance and robust mutexes hold the owner in the futex word */
    if (mutex->__protocol == PTHREAD_PRIO_INHERIT || mutex->__robust)
        return (int)(__atomic_load_n(&mutex->__state, __ATOMIC_RELAXED) & _PTHREAD_FUTEX_TID_MASK);
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

/* pthread_create, but reports a lack of memory as ENOMEM */
int _pthread_create(pthread_t * __restrict thread, const pthread_attr_t * __restrict attr,
                    void *(*start_routine)(void *), void * __restrict arg);

/* Scheduling for a thread id; these return 0 or an errno value */
#ifdef _PTHREAD_HAVE_SCHED
int _pthread_setsched(struct __pthread *thread, int policy, int priority);
int _pthread_getsched(struct __pthread *thread, int *policy, int *priority);
#else
static inline int
_pthread_setsched(struct __pthread *thread, int policy, int priority)
{
    (void)thread;
    (void)policy;
    (void)priority;
    return ENOTSUP;
}

static inline int
_pthread_getsched(struct __pthread *thread, int *policy, int *priority)
{
    (void)thread;
    (void)policy;
    (void)priority;
    return ENOTSUP;
}

static inline bool
_pthread_policy_valid(int policy)
{
    return policy == SCHED_OTHER;
}
#endif

extern int      _pthread_concurrency;

__noreturn void _pthread_cancel_act(struct __pthread *self);

/* Run the calling thread's cleanup handlers */
void            _pthread_cleanup_exit(struct __pthread *self);

/* Fork handlers registered with pthread_atfork */
struct _pthread_atfork {
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
};

extern pthread_mutex_t         _pthread_atfork_lock;
extern struct _pthread_atfork *_pthread_atfork;
extern size_t                  _pthread_atfork_count;

/*
 * Called by fork around the system call; the C library lock is held
 * from prepare until parent or child
 */
void                           _pthread_fork_prepare(void) __no_thread_safety_analysis;
void                           _pthread_fork_parent(void) __no_thread_safety_analysis;
void                           _pthread_fork_child(void) __no_thread_safety_analysis;
void                           _pthread_fork_reset(void);

/* Queue an exiting detached thread to be unmapped once the kernel is done with it */
void                           _pthread_zombie(struct __pthread *thread);

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

/*
 * The running threads. pthread_create holds the lock from before it
 * blocks signals until the new thread is listed, so a credential
 * change sees every thread, and every thread it sees can take its
 * signal.
 */
extern pthread_mutex_t   _pthread_all_lock;
extern struct __pthread *_pthread_all;
extern bool              _pthread_main_listed;

static inline void
_pthread_all_add(struct __pthread *t)
{
    t->all_prev = NULL;
    t->all_next = _pthread_all;
    if (_pthread_all)
        _pthread_all->all_prev = t;
    _pthread_all = t;
}

static inline void
_pthread_all_remove(struct __pthread *t)
{
    if (t->all_prev)
        t->all_prev->all_next = t->all_next;
    else if (_pthread_all == t)
        _pthread_all = t->all_next;
    else
        return;
    if (t->all_next)
        t->all_next->all_prev = t->all_prev;
    t->all_next = t->all_prev = NULL;
}

/* Repeats the current credential change in the calling thread; see _syscall_setxid */
void                     _pthread_setxid_act(void);

/* Installs the handler for _PTHREAD_SIGCANCEL, which also carries credential changes */
void                     _pthread_sigcancel_init(void);
extern struct __pthread *_pthread_zombies;

/* Unmap detached threads which have exited */
void                     _pthread_reap(void);

/* Wait until the kernel has finished with an exiting thread */
void                     _pthread_wait_exit(struct __pthread *thread);

/* Run thread-specific data destructors for the calling thread */
void                     _pthread_specific_exit(struct __pthread *self);

/* The thread's first code, called from the OS layer's thread start */
__noreturn void          _pthread_start(struct __pthread *self);

/*
 * Where a thread's descriptor and TLS block go at the top of its
 * stack region, and the room they take there
 */
struct _pthread_layout {
    size_t align;
    size_t tls_size;
    size_t desc_size; /* descriptor and thread-specific data values */
    size_t overhead;  /* all of that, with room for aligning the top */
};

void  _pthread_layout(struct _pthread_layout *layout);

/*
 * Allocate a stack region of at least size bytes, with guard bytes
 * below it made inaccessible where the OS can. Stores what
 * _pthread_os_stack_free needs in *map and *map_size and returns the
 * top of the region, or NULL. Only stacks picolibc provides come from
 * here; the descriptor and TLS block go at the top of the region.
 */
void *_pthread_os_stack_alloc(size_t size, size_t guard, void **map, size_t *map_size);

/* Release a region from _pthread_os_stack_alloc once nothing uses it */
void  _pthread_os_stack_free(void *map, size_t map_size);

#ifdef _PTHREAD_HAVE_RELEASE
/*
 * Release the OS's private objects for a thread which has exited, as
 * the thread is joined or a detached thread is reclaimed. A detached
 * thread on an application stack is never reclaimed (its memory
 * belongs to the application), so the OS releases those itself.
 */
void _pthread_os_release(struct __pthread *t);
#endif

/* Reclaim an exited thread */
static inline void
_pthread_release(struct __pthread *t)
{
    void  *map = t->map;
    size_t map_size = t->map_size;

#ifdef _PTHREAD_HAVE_RELEASE
    _pthread_os_release(t);
#endif
    if (map)
        _pthread_os_stack_free(map, map_size);
}

/*
 * Initialize the thread control block for a new TLS block (already
 * filled by _init_tls) and return the thread pointer value the kernel
 * takes for the new thread. arg_space provides __PTHREAD_TLS_ARG_SIZE
 * bytes for any structure the kernel needs to read while creating it.
 * This depends only on the machine; libos/pthread/machine has it for
 * OS layers which set up TLS this way.
 */
void           *_pthread_tls_setup(void *tls, void *arg_space);

/*
 * Start a thread for t, sharing the creator's address space, files
 * and signal handlers, with every signal blocked, which calls
 * _pthread_start(t). Set t->tid before either thread can use it.
 * Returns 0, ENOMEM or EAGAIN.
 */
int             _pthread_os_create(struct __pthread *t);

/*
 * End the calling thread alone, then clear self->tid (unless the
 * kernel does) and set and wake _pthread_exit_word(self) once nothing
 * of the thread's memory is in use.
 */
__noreturn void _pthread_os_exit(struct __pthread *self);

/* Set the id of the initial thread, or of the only thread in a fork child */
void            _pthread_os_set_tid(struct __pthread *self);

#ifdef _PTHREAD_HAVE_SIGNALS
/*
 * Send kernel signal ksig to a thread of this process: 0 or an errno
 * value; 0 once the thread has exited
 */
int  _pthread_tkill(struct __pthread *thread, int ksig);

/* Install the handler for _PTHREAD_SIGCANCEL */
void _pthread_os_sigcancel(void (*handler)(int));

/* sigprocmask for the calling thread, returning 0 or an errno value, leaving errno alone */
int  _sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
#endif

#endif /* _LOCAL_PTHREAD_H_ */
