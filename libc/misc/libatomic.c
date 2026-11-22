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
 * The library calls compilers make for atomic operations they don't
 * implement inline (the libatomic ABI): the generic __atomic_load,
 * __atomic_store, __atomic_exchange and __atomic_compare_exchange,
 * which take the object size, the sized _1, _2, _4, _8 and _16
 * operations, __atomic_test_and_set and __atomic_is_lock_free.
 *
 * Sizes the compiler accesses lock-free use its builtins, so they
 * work with inline code touching the same objects. Others take locks
 * from a small table hashed by address, built from the C library's
 * retargetable locks: mutexes on threaded systems, nothing on
 * single-threaded ones. An object covers each 64-byte granule it
 * touches, and the locks for those granules are taken in table order,
 * so an object spanning two slots can't deadlock against another.
 *
 * Everything is in this one file, defined weak: an application
 * linking a toolchain's libatomic as well gets one set of functions
 * (libatomic's strong ones when both are linked), never a mix
 * using two different lock tables.
 */

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

#ifndef __SINGLE_THREAD

#define ATOMIC_LOCKS        8  /* a power of two */
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

/* The slots from first through first + count - 1, wrapping */
struct atomic_slots {
    unsigned first;
    unsigned count;
};

static struct atomic_slots
atomic_slots(const volatile void *mem, size_t size)
{
    uintptr_t           start = (uintptr_t)mem >> ATOMIC_GRANULE_BITS;
    uintptr_t           end = ((uintptr_t)mem + (size ? size - 1 : 0)) >> ATOMIC_GRANULE_BITS;
    struct atomic_slots slots;

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
atomic_slot_used(struct atomic_slots slots, unsigned i)
{
    return ((i - slots.first) & (ATOMIC_LOCKS - 1)) < slots.count;
}

static struct atomic_slots __no_thread_safety_analysis
atomic_lock(const volatile void *mem, size_t size)
{
    struct atomic_slots slots = atomic_slots(mem, size);

    atomic_locks_init();
    /* Always in table order, so that two objects can't deadlock */
    for (unsigned i = 0; i < ATOMIC_LOCKS; i++)
        if (atomic_slot_used(slots, i))
            __lock_acquire(atomic_locks[i]);
    return slots;
}

static void __no_thread_safety_analysis
atomic_unlock(struct atomic_slots slots)
{
    for (unsigned i = ATOMIC_LOCKS; i-- > 0;)
        if (atomic_slot_used(slots, i))
            __lock_release(atomic_locks[i]);
}

#define ATOMIC_LOCK(mem, size) struct atomic_slots atomic_held = atomic_lock(mem, size)
#define ATOMIC_UNLOCK()        atomic_unlock(atomic_held)

#else

#define ATOMIC_LOCK(mem, size) ((void)(mem), (void)(size))
#define ATOMIC_UNLOCK()        ((void)0)

#endif /* __SINGLE_THREAD */

/* Operations on objects of each size the compiler names with _n */
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

#define ATOMIC_SIZED(n, T)                                                                     \
    ATOMIC_FUNCTION(T, __atomic_load_##n, (const volatile void *mem, int model))               \
    {                                                                                          \
        const volatile T *p = mem;                                                             \
        T                 val;                                                                 \
        if (ATOMIC_LOCK_FREE(n, mem))                                                          \
            return __atomic_load_n(p, model);                                                  \
        ATOMIC_LOCK(mem, n);                                                                   \
        val = *p;                                                                              \
        ATOMIC_UNLOCK();                                                                       \
        return val;                                                                            \
    }                                                                                          \
    ATOMIC_FUNCTION(void, __atomic_store_##n, (volatile void *mem, T val, int model))          \
    {                                                                                          \
        volatile T *p = mem;                                                                   \
        if (ATOMIC_LOCK_FREE(n, mem)) {                                                        \
            __atomic_store_n(p, val, model);                                                   \
            return;                                                                            \
        }                                                                                      \
        ATOMIC_LOCK(mem, n);                                                                   \
        *p = val;                                                                              \
        ATOMIC_UNLOCK();                                                                       \
    }                                                                                          \
    ATOMIC_FUNCTION(T, __atomic_exchange_##n, (volatile void *mem, T val, int model))          \
    {                                                                                          \
        volatile T *p = mem;                                                                   \
        T           old;                                                                       \
        if (ATOMIC_LOCK_FREE(n, mem))                                                          \
            return __atomic_exchange_n(p, val, model);                                         \
        ATOMIC_LOCK(mem, n);                                                                   \
        old = *p;                                                                              \
        *p = val;                                                                              \
        ATOMIC_UNLOCK();                                                                       \
        return old;                                                                            \
    }                                                                                          \
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
    }                                                                                          \
    ATOMIC_SIZED_FETCH(n, T, add, old + val)                                                   \
    ATOMIC_SIZED_FETCH(n, T, sub, old - val)                                                   \
    ATOMIC_SIZED_FETCH(n, T, and, old &val)                                                    \
    ATOMIC_SIZED_FETCH(n, T, or, old | val)                                                    \
    ATOMIC_SIZED_FETCH(n, T, xor, old ^ val)                                                   \
    ATOMIC_SIZED_FETCH(n, T, nand, ~(old & val))

ATOMIC_SIZED(1, uint8_t)
ATOMIC_SIZED(2, uint16_t)
ATOMIC_SIZED(4, uint32_t)
ATOMIC_SIZED(8, uint64_t)
#ifdef __SIZEOF_INT128__
ATOMIC_SIZED(16, unsigned __int128)
#endif

/*
 * The generic operations, for any size. Where the compiler accesses
 * objects of this size and alignment lock-free, use its builtins
 */
#ifdef __SIZEOF_INT128__
#define ATOMIC_CASE_16(f) f(16, unsigned __int128)
#else
#define ATOMIC_CASE_16(f)
#endif

#define ATOMIC_CASES(f) \
    f(1, uint8_t);      \
    f(2, uint16_t);     \
    f(4, uint32_t);     \
    f(8, uint64_t);     \
    ATOMIC_CASE_16(f)

#define ATOMIC_LOAD_CASE(n, T)                                     \
    case n:                                                        \
        if (ATOMIC_LOCK_FREE(n, mem)) {                            \
            T v = __atomic_load_n((const volatile T *)mem, model); \
            memcpy(ret, &v, n);                                    \
            return;                                                \
        }                                                          \
        break

ATOMIC_FUNCTION(void, __atomic_load, (size_t size, const volatile void *mem, void *ret, int model))
{
    switch (size) {
        ATOMIC_CASES(ATOMIC_LOAD_CASE);
    default:
        break;
    }
    ATOMIC_LOCK(mem, size);
    memcpy(ret, (const void *)mem, size);
    ATOMIC_UNLOCK();
}

#define ATOMIC_STORE_CASE(n, T)                            \
    case n:                                                \
        if (ATOMIC_LOCK_FREE(n, mem)) {                    \
            T v;                                           \
            memcpy(&v, val, n);                            \
            __atomic_store_n((volatile T *)mem, v, model); \
            return;                                        \
        }                                                  \
        break

ATOMIC_FUNCTION(void, __atomic_store, (size_t size, volatile void *mem, void *val, int model))
{
    switch (size) {
        ATOMIC_CASES(ATOMIC_STORE_CASE);
    default:
        break;
    }
    ATOMIC_LOCK(mem, size);
    memcpy((void *)mem, val, size);
    ATOMIC_UNLOCK();
}

#define ATOMIC_EXCHANGE_CASE(n, T)                                \
    case n:                                                       \
        if (ATOMIC_LOCK_FREE(n, mem)) {                           \
            T v;                                                  \
            memcpy(&v, val, n);                                   \
            v = __atomic_exchange_n((volatile T *)mem, v, model); \
            memcpy(ret, &v, n);                                   \
            return;                                               \
        }                                                         \
        break

ATOMIC_FUNCTION(void, __atomic_exchange,
                (size_t size, volatile void *mem, void *val, void *ret, int model))
{
    switch (size) {
        ATOMIC_CASES(ATOMIC_EXCHANGE_CASE);
    default:
        break;
    }
    volatile unsigned char *m = mem;
    unsigned char          *v = val;
    unsigned char          *r = ret;
    ATOMIC_LOCK(mem, size);
    /* Byte by byte, as val and ret may be the same */
    for (size_t i = 0; i < size; i++) {
        unsigned char old = m[i];
        m[i] = v[i];
        r[i] = old;
    }
    ATOMIC_UNLOCK();
}

#define ATOMIC_COMPARE_EXCHANGE_CASE(n, T)                                                       \
    case n:                                                                                      \
        if (ATOMIC_LOCK_FREE(n, mem)) {                                                          \
            T    e, d;                                                                           \
            bool ok;                                                                             \
            memcpy(&e, expected, n);                                                             \
            memcpy(&d, desired, n);                                                              \
            ok = __atomic_compare_exchange_n((volatile T *)mem, &e, d, false, success, failure); \
            if (!ok)                                                                             \
                memcpy(expected, &e, n);                                                         \
            return ok;                                                                           \
        }                                                                                        \
        break

ATOMIC_FUNCTION(bool, __atomic_compare_exchange,
                (size_t size, volatile void *mem, void *expected, void *desired, int success,
                 int failure))
{
    bool ok;

    switch (size) {
        ATOMIC_CASES(ATOMIC_COMPARE_EXCHANGE_CASE);
    default:
        break;
    }
    ATOMIC_LOCK(mem, size);
    ok = memcmp((void *)mem, expected, size) == 0;
    if (ok)
        memcpy((void *)mem, desired, size);
    else
        memcpy(expected, (void *)mem, size);
    ATOMIC_UNLOCK();
    return ok;
}

#ifndef __GCC_ATOMIC_TEST_AND_SET_TRUEVAL
#define __GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1
#endif

/* GCC calls this where it has no inline byte exchange */
ATOMIC_FUNCTION(bool, __atomic_test_and_set, (volatile void *mem, int model))
{
    volatile unsigned char *p = mem;
    unsigned char           old;

    if (ATOMIC_LOCK_FREE(1, mem))
        return __atomic_test_and_set(p, model);
    ATOMIC_LOCK(mem, 1);
    old = *p;
    *p = __GCC_ATOMIC_TEST_AND_SET_TRUEVAL;
    ATOMIC_UNLOCK();
    return old != 0;
}

#define ATOMIC_IS_LOCK_FREE_CASE(n, T)  \
    case n:                             \
        return ATOMIC_LOCK_FREE(n, mem)

/* Agrees with the compiler: only what it accesses inline is lock-free */
ATOMIC_FUNCTION(bool, __atomic_is_lock_free, (size_t size, const volatile void *mem))
{
    switch (size) {
        ATOMIC_CASES(ATOMIC_IS_LOCK_FREE_CASE);
    default:
        return false;
    }
}
