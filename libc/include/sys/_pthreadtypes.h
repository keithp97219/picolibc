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
 * POSIX thread types. These are part of picolibc's OS-independent ABI;
 * an OS layer implements them, translating to its own kernel interface.
 * <sys/types.h> includes this header when it is available, so an OS
 * providing its own threads can supply a replacement.
 */

#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_

#include <sys/_types.h>

/* Implementation limits, published by <limits.h> */
#define __PTHREAD_DESTRUCTOR_ITERATIONS 4
#define __PTHREAD_KEYS_MAX              128
#define __PTHREAD_STACK_MIN             16384
#define __SEM_VALUE_MAX                 0x7fffffff

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
} pthread_mutex_t;

typedef struct {
    int __type;
    int __pshared;
    int __protocol;
    int __prioceiling;
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
    unsigned int __seq; /* advanced as each group of threads leaves */
    int          __pshared;
} pthread_barrier_t;

typedef struct {
    int __pshared;
} pthread_barrierattr_t;

typedef volatile int pthread_spinlock_t;

typedef int          pthread_once_t;

typedef unsigned int pthread_key_t;

#endif /* _SYS__PTHREADTYPES_H_ */
