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

#ifndef _LOCAL_PTHREAD_OS_H_
#define _LOCAL_PTHREAD_OS_H_

/*
 * The Linux part of libos/pthread's thread implementation, included
 * by local-pthread.h: clone, futexes and the kernel signal mask.
 */

#include "local-sigaction.h"
#include "local-time.h"
#include "local-sched.h"
#include <stdbool.h>
#include <string.h>

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
#define LINUX_FUTEX_OWNER_DIED       0x40000000U
#define LINUX_FUTEX_WAITERS          0x80000000U
#define LINUX_FUTEX_TRYLOCK_PI       8

/*
 * Use the 64-bit time futex call where the kernel has one, so that
 * 32-bit targets don't truncate timeouts.
 */
#ifdef LINUX_SYS_futex_time64
#define LINUX_SYS_futex_time_t LINUX_SYS_futex_time64
#else
#define LINUX_SYS_futex_time_t LINUX_SYS_futex
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

/* What Linux provides of local-pthread.h's optional capabilities: all of them */
#define _PTHREAD_HAVE_MAP       1
#define _PTHREAD_HAVE_SIGNALS   1
#define _PTHREAD_HAVE_SCHED     1
#define _PTHREAD_HAVE_PI        1
#define _PTHREAD_HAVE_ROBUST    1
#define _PTHREAD_HAVE_PSHARED   1
#define _PTHREAD_HAVE_FORK      1
#define _PTHREAD_HAVE_SEM_NAMED 1

/* The range of real-time priorities, and of mutex priority ceilings */
#define _PTHREAD_PRIO_MIN LINUX_PRIO_RT_MIN
#define _PTHREAD_PRIO_MAX LINUX_PRIO_RT_MAX

/*
 * The parts of the futex word the kernel manages for priority
 * inheritance and robust mutexes
 */
#define _PTHREAD_FUTEX_TID_MASK   LINUX_FUTEX_TID_MASK
#define _PTHREAD_FUTEX_OWNER_DIED LINUX_FUTEX_OWNER_DIED

/*
 * Cancellation uses a Linux real-time signal which picolibc's signal
 * numbers never reach, so applications can't block or catch it.
 */
#define _PTHREAD_SIGCANCEL 32

/*
 * Linux credentials belong to each thread, so the C library makes a
 * credential change in every thread, signalled with _PTHREAD_SIGCANCEL.
 */
#define _PTHREAD_SETXID 1

/* The kernel's signal mask */
typedef __kernel_sigset_t _pthread_sigset_t;

/* What each thread descriptor holds for Linux */
struct __pthread_os {
    /*
     * The robust mutexes held, which the kernel marks if the thread
     * dies holding them; laid out as the kernel's robust_list_head
     */
    struct {
        void *list;
        long  futex_offset;
        void *list_op_pending;
    } robust;
    bool robust_registered;
};

/*
 * The kernel clears the thread id and wakes the futex there once an
 * exiting thread no longer uses its memory
 */
#define _pthread_exit_word(t) ((__pthread_futex_t *)&(t)->tid)
#define _PTHREAD_EXIT_DONE    0U

/* Convert an absolute timeout for the kernel: 0, EINVAL or ETIMEDOUT */
int _futex_timespec(struct __kernel_timespec64 *kts, const struct timespec *abstime);

/*
 * Thread memory: anonymous mappings made directly, leaving errno
 * alone. _pthread_map returns NULL on failure.
 */
static inline void *
_pthread_map(size_t size)
{
    long ret;

#ifdef LINUX_SYS_mmap2
    ret = _syscall_raw(LINUX_SYS_mmap2, NULL, size, LINUX_PROT_READ | LINUX_PROT_WRITE,
                       LINUX_MAP_PRIVATE | LINUX_MAP_ANONYMOUS, -1, 0);
#else
    ret = _syscall_raw(LINUX_SYS_mmap, NULL, size, LINUX_PROT_READ | LINUX_PROT_WRITE,
                       LINUX_MAP_PRIVATE | LINUX_MAP_ANONYMOUS, -1, 0);
#endif
    if ((unsigned long)ret >= (unsigned long)-4095)
        return NULL;
    return (void *)ret;
}

static inline void
_pthread_unmap(void *addr, size_t size)
{
    (void)_syscall_raw(LINUX_SYS_munmap, addr, size);
}

/* Make the guard region inaccessible, as far as possible */
static inline void
_pthread_guard(void *addr, size_t size)
{
    (void)_syscall_raw(LINUX_SYS_mprotect, addr, size, LINUX_PROT_NONE);
}

/* Block every signal in the calling thread, returning the old mask */
static inline void
_pthread_sigblock_all(_pthread_sigset_t *old)
{
    _pthread_sigset_t all;

    memset(&all, 0xff, sizeof(all));
    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_SETMASK, &all, old, __KERNEL_NSIG_BYTES);
}

/* Set the calling thread's signal mask */
static inline void
_pthread_sigsetmask(const _pthread_sigset_t *set)
{
    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_SETMASK, set, NULL, __KERNEL_NSIG_BYTES);
}

/* Unblock _PTHREAD_SIGCANCEL in the calling thread */
static inline void
_pthread_sigcancel_unblock(void)
{
    _pthread_sigset_t cancel = {};

    __kernel_sigset_set_mask(&cancel, _PTHREAD_SIGCANCEL);
    (void)_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_UNBLOCK, &cancel, NULL,
                       __KERNEL_NSIG_BYTES);
}

/* The kernel's number for a signal, or -1 when there is none */
static inline int
_pthread_signal_to_kernel(int sig)
{
    return _signal_to_linux(sig);
}

/* Whether the kernel supports a scheduling policy */
static inline bool
_pthread_policy_valid(int policy)
{
    return _sched_policy_to_linux(policy) >= 0;
}

/*
 * Raw clone(2) call. In the child, pops a function and its argument
 * from the new stack, calls the function and exits the thread with
 * its return value. Returns the child thread id or a negative Linux
 * error code.
 */
long _linux_clone(unsigned long flags, void *stack, int *ptid, int *ctid, void *tls);

#endif /* _LOCAL_PTHREAD_OS_H_ */
