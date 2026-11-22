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
 * The FreeBSD part of libos/pthread's thread implementation, included
 * by local-pthread.h: thr_new, _umtx_op and per-thread signal masks.
 */

#include "local-signal.h"
#include "local-time.h"
#include "local-sched.h"
#include <freebsd/freebsd-rtprio.h>
#include <freebsd/freebsd-rtprio-struct.h>
#include <freebsd/freebsd-thr.h>
#include <sched.h>
#include <stdbool.h>
#include <string.h>

/*
 * What FreeBSD provides of local-pthread.h's optional capabilities:
 * all of them. Its priority inheritance and robust mutexes are
 * _umtx_op mutex operations on the kernel's struct umutex, which each
 * such mutex has beside it (_pthread_os_mutex.c).
 */
#define _PTHREAD_HAVE_MAP       1
#define _PTHREAD_HAVE_SIGNALS   1
#define _PTHREAD_HAVE_SCHED     1
#define _PTHREAD_HAVE_PI        1
#define _PTHREAD_HAVE_ROBUST    1
#define _PTHREAD_HAVE_PSHARED   1
#define _PTHREAD_HAVE_FORK      1
#define _PTHREAD_HAVE_SEM_NAMED 1

/* Real-time priorities, as POSIX numbers them: rtprio's 31 to 0 */
#define _PTHREAD_PRIO_MIN 0
#define _PTHREAD_PRIO_MAX FREEBSD_RTP_PRIO_MAX

/*
 * Cancellation uses SIGTHR, FreeBSD's signal 32, which picolibc's
 * signal numbers never reach, so applications can't block or catch it.
 */
#define _PTHREAD_SIGCANCEL FREEBSD_SIGTHR

/* The kernel's signal mask */
typedef struct __freebsd_sigset _pthread_sigset_t;

/* What each thread descriptor holds for FreeBSD */
struct __pthread_os {
    /* thr_new stores the thread id here, as a long, for both threads */
    long        ktid;
    /* thr_exit stores 1 here and wakes its waiters once the thread is gone */
    long        exit_state;
    /*
     * The robust mutexes held, which the kernel marks should the
     * thread die holding them: the address of the first one's struct
     * umutex, which links to the next; and the one being locked or
     * unlocked, which may be on the list or not
     */
    __uintptr_t robust_list;
    __uintptr_t robust_inact;
    bool        robust_registered;
};

/* Waiting on the low half of exit_state works as amd64 is little-endian */
#define _pthread_exit_word(t) ((__pthread_futex_t *)&(t)->os.exit_state)
#define _PTHREAD_EXIT_DONE    1U

/*
 * Thread memory: anonymous mappings made directly, leaving errno
 * alone. _pthread_map returns NULL on failure.
 */
static inline void *
_pthread_map(size_t size)
{
    /* The offset is 64 bits, so pass it as a long */
    long ret = _syscall_raw(FREEBSD_SYS_mmap, NULL, size, FREEBSD_PROT_READ | FREEBSD_PROT_WRITE,
                            FREEBSD_MAP_PRIVATE | FREEBSD_MAP_ANON, -1, (long)0);

    if (ret < 0)
        return NULL;
    return (void *)ret;
}

static inline void
_pthread_unmap(void *addr, size_t size)
{
    (void)_syscall_raw(FREEBSD_SYS_munmap, addr, size);
}

/* Make the guard region inaccessible, as far as possible */
static inline void
_pthread_guard(void *addr, size_t size)
{
    (void)_syscall_raw(FREEBSD_SYS_mprotect, addr, size, FREEBSD_PROT_NONE);
}

/* Block every signal in the calling thread, returning the old mask */
static inline void
_pthread_sigblock_all(_pthread_sigset_t *old)
{
    _pthread_sigset_t all;

    memset(&all, 0xff, sizeof(all));
    (void)_syscall_raw(FREEBSD_SYS_sigprocmask, FREEBSD_SIG_SETMASK, &all, old);
}

/* Set the calling thread's signal mask */
static inline void
_pthread_sigsetmask(const _pthread_sigset_t *set)
{
    (void)_syscall_raw(FREEBSD_SYS_sigprocmask, FREEBSD_SIG_SETMASK, set, NULL);
}

/* Unblock _PTHREAD_SIGCANCEL in the calling thread */
static inline void
_pthread_sigcancel_unblock(void)
{
    _pthread_sigset_t cancel = {};

    cancel.__bits[(_PTHREAD_SIGCANCEL - 1) / 32] = (__uint32_t)1 << ((_PTHREAD_SIGCANCEL - 1) % 32);
    (void)_syscall_raw(FREEBSD_SYS_sigprocmask, FREEBSD_SIG_UNBLOCK, &cancel, NULL);
}

/* The kernel's number for a signal, or -1 when there is none */
static inline int
_pthread_signal_to_kernel(int sig)
{
    if (sig == 0)
        return 0;
    return _signal_to_freebsd(sig);
}

/* Whether the kernel supports a scheduling policy */
static inline bool
_pthread_policy_valid(int policy)
{
    return _sched_policy_to_freebsd(policy) >= 0;
}

#endif /* _LOCAL_PTHREAD_OS_H_ */
