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

#define _DEFAULT_SOURCE
#include "local-pthread.h"
#include <stdlib.h>
#include <sys/lock.h>

/* C library locks built on POSIX thread mutexes */

struct __lock {
    pthread_mutex_t mutex;
};

#define LOCK_INITIALIZER(type)       \
    {                                \
        .mutex = {.__type = (type) } \
    }

struct __lock        __lock___libc_recursive_mutex = LOCK_INITIALIZER(PTHREAD_MUTEX_RECURSIVE);

/* Shared by every lock which couldn't be allocated */
static struct __lock fallback_lock = LOCK_INITIALIZER(PTHREAD_MUTEX_RECURSIVE);

static void
lock_init(_LOCK_T *lock, int type)
{
    struct __lock *l = malloc(sizeof(struct __lock));

    if (!l) {
        *lock = &fallback_lock;
        return;
    }
    pthread_mutex_init(&l->mutex, NULL);
    l->mutex.__type = type;
    *lock = l;
}

static void
lock_close(_LOCK_T lock)
{
    if (lock != &fallback_lock && lock != &__lock___libc_recursive_mutex)
        free(lock);
}

void
__retarget_lock_init(_LOCK_T *lock)
{
    lock_init(lock, PTHREAD_MUTEX_NORMAL);
}

void
__retarget_lock_init_recursive(_LOCK_T *lock)
{
    lock_init(lock, PTHREAD_MUTEX_RECURSIVE);
}

void
__retarget_lock_close(_LOCK_T lock)
{
    lock_close(lock);
}

void
__retarget_lock_close_recursive(_LOCK_T lock)
{
    lock_close(lock);
}

void __no_thread_safety_analysis
__retarget_lock_acquire(_LOCK_T lock)
{
    (void)pthread_mutex_lock(&lock->mutex);
}

void __no_thread_safety_analysis
__retarget_lock_acquire_recursive(_LOCK_T lock)
{
    (void)pthread_mutex_lock(&lock->mutex);
}

void __no_thread_safety_analysis
__retarget_lock_release(_LOCK_T lock)
{
    (void)pthread_mutex_unlock(&lock->mutex);
}

void __no_thread_safety_analysis
__retarget_lock_release_recursive(_LOCK_T lock)
{
    (void)pthread_mutex_unlock(&lock->mutex);
}
