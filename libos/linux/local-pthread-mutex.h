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

#ifndef _LOCAL_PTHREAD_MUTEX_H_
#define _LOCAL_PTHREAD_MUTEX_H_

#include "local-pthread.h"

/*
 * Priority inheritance and robust mutexes (local-pthread.h's OS
 * mutexes) keep the owner's thread id in __state, under
 * LINUX_FUTEX_TID_MASK, with LINUX_FUTEX_OWNER_DIED set by the kernel
 * when the owner died. A robust mutex's __robust_state says whether
 * it is consistent, and its __list_next and __list_prev link it into
 * the owner's robust list.
 */
#define _PTHREAD_ROBUST_CONSISTENT     0
#define _PTHREAD_ROBUST_INCONSISTENT   1
#define _PTHREAD_ROBUST_NOTRECOVERABLE 2

/* Priority inheritance futexes; these return 0 or an errno value */
int  _futex_lock_pi(__pthread_futex_t *addr, bool pshared, clockid_t clock,
                    const struct timespec *abstime);
void _futex_unlock_pi(__pthread_futex_t *addr, bool pshared);

/* Whether the kernel has priority inheritance futexes */
bool _pthread_pi_available(void);

/*
 * Robust mutexes: locking returns 0, EOWNERDEAD, ENOTRECOVERABLE or
 * an error; unlocking assumes the caller owns the mutex
 */
int  _pthread_robust_lock(pthread_mutex_t *mutex, struct __pthread *self, clockid_t clock,
                          const struct timespec *abstime, bool trylock);
void _pthread_robust_unlock(pthread_mutex_t *mutex, struct __pthread *self);
bool _pthread_robust_available(void);

#endif /* _LOCAL_PTHREAD_MUTEX_H_ */
