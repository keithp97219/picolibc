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
 * The POSIX thread type names, taken from the OS's <sys/_pthread.h>,
 * which defines them with reserved names so that headers needing the
 * types (<threads.h>, or <signal.h> in a strict ISO C compilation)
 * don't publish them. <sys/types.h> and <signal.h> include this when
 * the OS provides threads.
 */

#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_

#include <sys/_pthread.h>

#ifndef _PTHREAD_T_DECLARED
typedef __pthread_t pthread_t;
#define _PTHREAD_T_DECLARED
#endif
#ifndef _PTHREAD_ATTR_T_DECLARED
typedef __pthread_attr_t pthread_attr_t;
#define _PTHREAD_ATTR_T_DECLARED
#endif
#ifndef _PTHREAD_MUTEX_T_DECLARED
typedef __pthread_mutex_t pthread_mutex_t;
#define _PTHREAD_MUTEX_T_DECLARED
#endif
#ifndef _PTHREAD_MUTEXATTR_T_DECLARED
typedef __pthread_mutexattr_t pthread_mutexattr_t;
#define _PTHREAD_MUTEXATTR_T_DECLARED
#endif
#ifndef _PTHREAD_COND_T_DECLARED
typedef __pthread_cond_t pthread_cond_t;
#define _PTHREAD_COND_T_DECLARED
#endif
#ifndef _PTHREAD_CONDATTR_T_DECLARED
typedef __pthread_condattr_t pthread_condattr_t;
#define _PTHREAD_CONDATTR_T_DECLARED
#endif
#ifndef _PTHREAD_RWLOCK_T_DECLARED
typedef __pthread_rwlock_t pthread_rwlock_t;
#define _PTHREAD_RWLOCK_T_DECLARED
#endif
#ifndef _PTHREAD_RWLOCKATTR_T_DECLARED
typedef __pthread_rwlockattr_t pthread_rwlockattr_t;
#define _PTHREAD_RWLOCKATTR_T_DECLARED
#endif
#ifndef _PTHREAD_BARRIER_T_DECLARED
typedef __pthread_barrier_t pthread_barrier_t;
#define _PTHREAD_BARRIER_T_DECLARED
#endif
#ifndef _PTHREAD_BARRIERATTR_T_DECLARED
typedef __pthread_barrierattr_t pthread_barrierattr_t;
#define _PTHREAD_BARRIERATTR_T_DECLARED
#endif
#ifndef _PTHREAD_SPINLOCK_T_DECLARED
typedef __pthread_spinlock_t pthread_spinlock_t;
#define _PTHREAD_SPINLOCK_T_DECLARED
#endif
#ifndef _PTHREAD_ONCE_T_DECLARED
typedef __pthread_once_t pthread_once_t;
#define _PTHREAD_ONCE_T_DECLARED
#endif
#ifndef _PTHREAD_KEY_T_DECLARED
typedef __pthread_key_t pthread_key_t;
#define _PTHREAD_KEY_T_DECLARED
#endif

#endif /* _SYS__PTHREADTYPES_H_ */
