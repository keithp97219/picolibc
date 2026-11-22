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
 * The one table of locks the libatomic functions share, hashed by
 * address and built from the C library's retargetable locks: mutexes
 * on threaded systems, nothing on single-threaded ones. An object
 * covers each 64-byte granule it touches, and the locks for those
 * granules are taken in table order, so an object spanning two slots
 * can't deadlock against another.
 */

#include "local-atomic.h"

#ifndef __SINGLE_THREAD

#define ATOMIC_LOCKS        8 /* a power of two */
#define ATOMIC_GRANULE_BITS 6

static _LOCK_T atomic_locks[ATOMIC_LOCKS];
static int     atomic_locks_ready;

static void
atomic_locks_init(void)
{
#if __GCC_ATOMIC_INT_LOCK_FREE == 2
    if (__atomic_load_n(&atomic_locks_ready, __ATOMIC_ACQUIRE))
        return;
#endif
    __LIBC_LOCK();
    if (!atomic_locks_ready) {
        for (unsigned i = 0; i < ATOMIC_LOCKS; i++)
            __lock_init(atomic_locks[i]);
#if __GCC_ATOMIC_INT_LOCK_FREE == 2
        __atomic_store_n(&atomic_locks_ready, 1, __ATOMIC_RELEASE);
#else
        atomic_locks_ready = 1;
#endif
    }
    __LIBC_UNLOCK();
}

static struct __atomic_slots
atomic_slots(const volatile void *mem, size_t size)
{
    uintptr_t             start = (uintptr_t)mem >> ATOMIC_GRANULE_BITS;
    uintptr_t             end = ((uintptr_t)mem + (size ? size - 1 : 0)) >> ATOMIC_GRANULE_BITS;
    struct __atomic_slots slots;

    if (end - start >= ATOMIC_LOCKS - 1) {
        slots.first = 0;
        slots.count = ATOMIC_LOCKS;
    } else {
        slots.first = (unsigned)(start & (ATOMIC_LOCKS - 1));
        slots.count = (unsigned)(end - start + 1);
    }
    return slots;
}

static bool
atomic_slot_used(struct __atomic_slots slots, unsigned i)
{
    return ((i - slots.first) & (ATOMIC_LOCKS - 1)) < slots.count;
}

struct __atomic_slots __no_thread_safety_analysis
__picolibc_atomic_lock(const volatile void *mem, size_t size)
{
    struct __atomic_slots slots = atomic_slots(mem, size);

    atomic_locks_init();
    /* Always in table order, so that two objects can't deadlock */
    for (unsigned i = 0; i < ATOMIC_LOCKS; i++)
        if (atomic_slot_used(slots, i))
            __lock_acquire(atomic_locks[i]);
    return slots;
}

void __no_thread_safety_analysis
__picolibc_atomic_unlock(struct __atomic_slots slots)
{
    for (unsigned i = ATOMIC_LOCKS; i-- > 0;)
        if (atomic_slot_used(slots, i))
            __lock_release(atomic_locks[i]);
}

#endif /* __SINGLE_THREAD */
