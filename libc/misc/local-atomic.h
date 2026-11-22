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
 * Shared by the libatomic functions in libc/misc/libatomic_*.c: the
 * library calls compilers make for atomic operations they don't
 * implement inline. Sizes the compiler accesses lock-free use its
 * builtins, so they work with inline code touching the same objects.
 * Others take locks from the one table in libatomic_lock.c.
 *
 * Each function is in its own small object so a program links only
 * what it calls, and is weak so that a toolchain's libatomic linked as
 * well replaces it.
 */

#ifndef _LOCAL_ATOMIC_H_
#define _LOCAL_ATOMIC_H_

#include <sys/cdefs.h>
#include <sys/lock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __GNUCLIKE_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
/* The builtins for sizes which aren't lock-free are never reached */
#pragma GCC diagnostic ignored "-Watomic-alignment"
#endif

#define ATOMIC_SYMBOL(name) __XSTRING(__USER_LABEL_PREFIX__) #name

/* Declare and define a libatomic function under its reserved name */
#define ATOMIC_FUNCTION(ret, name, args)                            \
    ret __picolibc_##name args __asm__(ATOMIC_SYMBOL(name)) __weak; \
    ret __picolibc_##name args

/* Whether the compiler accesses an object of size n at mem lock-free */
#define ATOMIC_LOCK_FREE(n, mem)                                             \
    (__atomic_always_lock_free(n, 0) && ((uintptr_t)(mem) & ((n) - 1)) == 0)

/*
 * Without atomic read-modify-write instructions, compilers access
 * small objects with plain loads and stores and call these functions
 * for the rest, so no lock can exclude those accesses. These targets
 * are taken to be single-core, and the functions mask interrupts
 * instead, through hooks an RTOS can replace. That makes each
 * operation indivisible against both threads and interrupt handlers.
 * Only architectures with a default for the hooks do this; the others
 * keep the lock table.
 */
#if __GCC_ATOMIC_INT_LOCK_FREE < 2 && (defined(__arm__) || defined(__riscv))
#define ATOMIC_IRQ_LOCK
#endif

#ifdef ATOMIC_IRQ_LOCK

#define ATOMIC_LOCK(mem, size)                                                           \
    unsigned long atomic_irq = ((void)(mem), (void)(size), __retarget_atomic_irq_save())
#define ATOMIC_UNLOCK() __retarget_atomic_irq_restore(atomic_irq)

#elif !defined(__SINGLE_THREAD)

/* The table slots an object covers, from first through first + count - 1, wrapping */
struct __atomic_slots {
    unsigned first;
    unsigned count;
};

struct __atomic_slots __picolibc_atomic_lock(const volatile void *mem, size_t size);
void                  __picolibc_atomic_unlock(struct __atomic_slots slots);

#define ATOMIC_LOCK(mem, size) struct __atomic_slots atomic_held = __picolibc_atomic_lock(mem, size)
#define ATOMIC_UNLOCK()        __picolibc_atomic_unlock(atomic_held)

#else

#define ATOMIC_LOCK(mem, size) ((void)(mem), (void)(size))
#define ATOMIC_UNLOCK()        ((void)0)

#endif /* ATOMIC_IRQ_LOCK */

/* Operations on objects of each size the compiler names with _n */
#define ATOMIC_SIZED_LOAD(n, T)                                                  \
    ATOMIC_FUNCTION(T, __atomic_load_##n, (const volatile void *mem, int model)) \
    {                                                                            \
        const volatile T *p = mem;                                               \
        T                 val;                                                   \
        if (ATOMIC_LOCK_FREE(n, mem))                                            \
            return __atomic_load_n(p, model);                                    \
        ATOMIC_LOCK(mem, n);                                                     \
        val = *p;                                                                \
        ATOMIC_UNLOCK();                                                         \
        return val;                                                              \
    }

#define ATOMIC_SIZED_STORE(n, T)                                                      \
    ATOMIC_FUNCTION(void, __atomic_store_##n, (volatile void *mem, T val, int model)) \
    {                                                                                 \
        volatile T *p = mem;                                                          \
        if (ATOMIC_LOCK_FREE(n, mem)) {                                               \
            __atomic_store_n(p, val, model);                                          \
            return;                                                                   \
        }                                                                             \
        ATOMIC_LOCK(mem, n);                                                          \
        *p = val;                                                                     \
        ATOMIC_UNLOCK();                                                              \
    }

#define ATOMIC_SIZED_EXCHANGE(n, T)                                                   \
    ATOMIC_FUNCTION(T, __atomic_exchange_##n, (volatile void *mem, T val, int model)) \
    {                                                                                 \
        volatile T *p = mem;                                                          \
        T           old;                                                              \
        if (ATOMIC_LOCK_FREE(n, mem))                                                 \
            return __atomic_exchange_n(p, val, model);                                \
        ATOMIC_LOCK(mem, n);                                                          \
        old = *p;                                                                     \
        *p = val;                                                                     \
        ATOMIC_UNLOCK();                                                              \
        return old;                                                                   \
    }

#define ATOMIC_SIZED_COMPARE_EXCHANGE(n, T)                                                    \
    ATOMIC_FUNCTION(bool, __atomic_compare_exchange_##n,                                       \
                    (volatile void *mem, void *expected, T desired, int success, int failure)) \
    {                                                                                          \
        volatile T *p = mem;                                                                   \
        T          *e = expected;                                                              \
        T           old;                                                                       \
        bool        ret;                                                                       \
        if (ATOMIC_LOCK_FREE(n, mem))                                                          \
            return __atomic_compare_exchange_n(p, e, desired, false, success, failure);        \
        ATOMIC_LOCK(mem, n);                                                                   \
        old = *p;                                                                              \
        ret = old == *e;                                                                       \
        if (ret)                                                                               \
            *p = desired;                                                                      \
        else                                                                                   \
            *e = old;                                                                          \
        ATOMIC_UNLOCK();                                                                       \
        return ret;                                                                            \
    }

#define ATOMIC_SIZED_FETCH(n, T, op, expr)                                                \
    ATOMIC_FUNCTION(T, __atomic_fetch_##op##_##n, (volatile void *mem, T val, int model)) \
    {                                                                                     \
        volatile T *p = mem;                                                              \
        T           old;                                                                  \
        if (ATOMIC_LOCK_FREE(n, mem))                                                     \
            return __atomic_fetch_##op(p, val, model);                                    \
        ATOMIC_LOCK(mem, n);                                                              \
        old = *p;                                                                         \
        *p = (T)(expr);                                                                   \
        ATOMIC_UNLOCK();                                                                  \
        return old;                                                                       \
    }                                                                                     \
    ATOMIC_FUNCTION(T, __atomic_##op##_fetch_##n, (volatile void *mem, T val, int model)) \
    {                                                                                     \
        volatile T *p = mem;                                                              \
        T           old, result;                                                          \
        if (ATOMIC_LOCK_FREE(n, mem))                                                     \
            return __atomic_##op##_fetch(p, val, model);                                  \
        ATOMIC_LOCK(mem, n);                                                              \
        old = *p;                                                                         \
        result = (T)(expr);                                                               \
        *p = result;                                                                      \
        ATOMIC_UNLOCK();                                                                  \
        return result;                                                                    \
    }

#ifdef __SIZEOF_INT128__
#define ATOMIC_CASE_16(f) f(16, unsigned __int128)
#else
#define ATOMIC_CASE_16(f)
#endif

/* The sizes with _n functions, for the generic ones */
#define ATOMIC_CASES(f) \
    f(1, uint8_t);      \
    f(2, uint16_t);     \
    f(4, uint32_t);     \
    f(8, uint64_t);     \
    ATOMIC_CASE_16(f)

#endif /* _LOCAL_ATOMIC_H_ */
