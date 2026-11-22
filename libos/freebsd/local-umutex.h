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

#ifndef _LOCAL_UMUTEX_H_
#define _LOCAL_UMUTEX_H_

#include "local-pthread.h"
#include <freebsd/freebsd-umutex.h>
#include <freebsd/freebsd-umutex-struct.h>

/*
 * FreeBSD's priority inheritance and robust mutexes are operations on
 * the kernel's struct umutex, which pthread_mutex_t doesn't contain,
 * so each such mutex has one beside it, in a struct _pthread_umutex:
 *
 *  - A process-private mutex's comes from malloc in
 *    pthread_mutex_init, with its address in the mutex's
 *    __list_next, and goes in pthread_mutex_destroy.
 *
 *  - A process-shared mutex's is the kernel's off-page for the
 *    mutex's address (_umtx_op UMTX_OP_SHM), which the kernel keys by
 *    the memory object and offset holding the mutex, so every process
 *    mapping the mutex finds the same page. Each process maps it once
 *    and caches it by the mutex's address (_pthread_umutex_shared.c).
 *    The mutex's __state holds a number naming the initialization,
 *    also kept in the page, which lets a process spot a cached page
 *    that no longer belongs to the mutex at that address.
 *
 * The robust mutexes a thread holds are on its robust list, which the
 * kernel walks when the thread exits: the thread's os.robust_list
 * holds the address of the first one's struct umutex, and each
 * umutex's m_rb_lnk the address of the next's. As struct umutex
 * starts struct _pthread_umutex, those are the side structures'
 * addresses, in the owner's address space. The owner alone uses the
 * list, so a shared page holds the current owner's links.
 */
struct _pthread_umutex {
    struct __freebsd_umutex um;

    /* The owner's address of the mutex and the previous entry on its robust list */
    pthread_mutex_t        *mutex;
    struct _pthread_umutex *rb_prev;

    /* Process-shared: the initialization, matching the mutex's __state, and whether destroyed */
    unsigned int            gen;
    unsigned int            destroyed;
};

/* Owner values of struct umutex */
#define _UMUTEX_UNOWNED   ((__uint32_t)FREEBSD_UMUTEX_UNOWNED)
#define _UMUTEX_CONTESTED ((__uint32_t)FREEBSD_UMUTEX_CONTESTED)
#define _UMUTEX_OWNERDEAD ((__uint32_t)FREEBSD_UMUTEX_RB_OWNERDEAD)
#define _UMUTEX_NOTRECOV  ((__uint32_t)FREEBSD_UMUTEX_RB_NOTRECOV)

/*
 * A process-shared mutex's page, mapped and cached on first use; NULL
 * with *error set when there is none (the mutex was destroyed or never
 * initialized: EINVAL) or it can't be mapped
 */
struct _pthread_umutex *_pthread_umutex_shared(pthread_mutex_t *mutex, int *error);

/* Create the page for a process-shared mutex, with its flags: 0, ENOMEM or EAGAIN */
int                     _pthread_umutex_shared_create(pthread_mutex_t *mutex, __uint32_t flags);

/* Destroy a process-shared mutex's page, for every process */
void _pthread_umutex_shared_destroy(pthread_mutex_t *mutex, struct _pthread_umutex *um);

/* In the child of fork */
void _pthread_umutex_shared_fork_child(void);

/* The side structure of an OS mutex, or NULL with *error set */
static inline struct _pthread_umutex *
_pthread_umutex(pthread_mutex_t *mutex, int *error)
{
    struct _pthread_umutex *um;

    if (_pthread_mutex_pshared(mutex))
        return _pthread_umutex_shared(mutex, error);
    um = mutex->__list_next;
    if (!um)
        *error = EINVAL;
    return um;
}

#endif /* _LOCAL_UMUTEX_H_ */
