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

/*
 * libos/linux's threads: the types behind <pthread.h>, <semaphore.h>
 * and <threads.h>, their static initializers, and the implementation's
 * limits. picolibc's public headers include this; an OS that provides
 * threads supplies its own <sys/_pthread.h> defining the same names.
 */

#ifndef _SYS__PTHREAD_H_
#define _SYS__PTHREAD_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

/* Implementation limits, published by <limits.h> */
#define __PTHREAD_DESTRUCTOR_ITERATIONS 4
#define __PTHREAD_KEYS_MAX              128
#define __SEM_VALUE_MAX                 0x7fffffff

/*
 * The smallest thread stack: each thread's descriptor and TLS block
 * sit at the top of its stack, so this is their size, known only when
 * the program is linked, plus __PTHREAD_STACK_WORK_MIN for the code to
 * run on. sysconf(_SC_THREAD_STACK_MIN) reports it.
 */
#define __PTHREAD_STACK_WORK_MIN 16384
#define __PTHREAD_STACK_MIN      ((__size_t)__pthread_stack_min())

_BEGIN_STD_C
long __pthread_stack_min(void);
_END_STD_C

struct __pthread;

typedef struct __pthread *pthread_t;

typedef struct {
    void    *__stackaddr;
    __size_t __stacksize;
    __size_t __guardsize;
    int      __detachstate;
    int      __inheritsched;
    int      __schedpolicy;
    int      __schedpriority;
    int      __scope;
} pthread_attr_t;

typedef struct {
    /*
     * 0 unlocked, 1 locked, 2 locked with waiters. For priority
     * inheritance mutexes, the owner's thread id, managed with the
     * kernel.
     */
    unsigned int __state;
    int          __type;
    int          __pshared;
    int          __protocol;
    int          __prioceiling;
    int          __owner; /* thread id of the owner, for recursive and error checking mutexes */
    unsigned int __count; /* recursion depth beyond the first lock */
    int          __robust;
    int          __robust_state; /* consistent, inconsistent or not recoverable */
    /*
     * The owner's list of the mutexes it must track: robust ones, for
     * the kernel, or else PTHREAD_PRIO_PROTECT ones, for their ceilings
     */
    void        *__list_next;
    void        *__list_prev; /* the list link pointing at this mutex */
} pthread_mutex_t;

typedef struct {
    int __type;
    int __pshared;
    int __protocol;
    int __prioceiling;
    int __robust;
} pthread_mutexattr_t;

typedef struct {
    unsigned int __seq;
    __clockid_t  __clock;
    int          __pshared;
} pthread_cond_t;

typedef struct {
    __clockid_t __clock;
    int         __pshared;
} pthread_condattr_t;

typedef struct {
    unsigned int __state; /* number of readers, or UINT_MAX when write locked */
    unsigned int __seq;   /* advanced when the lock is released with waiters */
    unsigned int __waiters;
    int          __writer; /* thread id of the writer */
    int          __pshared;
} pthread_rwlock_t;

typedef struct {
    int __pshared;
} pthread_rwlockattr_t;

typedef struct {
    unsigned int __count;
    unsigned int __waiting;
    unsigned int __seq;  /* advanced as each group of threads leaves */
    unsigned int __lock; /* held while a thread arrives */
    int          __pshared;
} pthread_barrier_t;

typedef struct {
    int __pshared;
} pthread_barrierattr_t;

typedef volatile int pthread_spinlock_t;

typedef int          pthread_once_t;

typedef unsigned int pthread_key_t;

typedef struct {
    unsigned int __value;
    unsigned int __waiters;
    int          __pshared;
} sem_t;

/*
 * Static initializers. They use <pthread.h>'s constants and
 * CLOCK_REALTIME, which are defined wherever they're expanded.
 */
#define PTHREAD_MUTEX_INITIALIZER \
    { 0,                          \
      PTHREAD_MUTEX_DEFAULT,      \
      PTHREAD_PROCESS_PRIVATE,    \
      PTHREAD_PRIO_NONE,          \
      0,                          \
      0,                          \
      0,                          \
      PTHREAD_MUTEX_STALLED,      \
      0,                          \
      0,                          \
      0 }
#define PTHREAD_COND_INITIALIZER   { 0, CLOCK_REALTIME, PTHREAD_PROCESS_PRIVATE }
#define PTHREAD_ONCE_INIT          0
#define PTHREAD_RWLOCK_INITIALIZER { 0, 0, 0, 0, PTHREAD_PROCESS_PRIVATE }

#endif /* _SYS__PTHREAD_H_ */
