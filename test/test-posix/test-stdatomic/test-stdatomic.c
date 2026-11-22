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
 * The libatomic functions compilers call for atomic objects they
 * can't access lock-free: every operation on integers of each size
 * and on structures too large to be lock-free, including ones which
 * span two of the library's lock slots, with threads contending for
 * them where the system has POSIX threads.
 */

#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _POSIX_THREADS > 0
#include <pthread.h>
#endif

#ifdef __clang__
/* clang warns about each operation which isn't lock-free */
#pragma clang diagnostic ignored "-Watomic-alignment"
#endif

static int failures;

#define check(condition, message)                                                 \
    do {                                                                          \
        if (!(condition)) {                                                       \
            printf("%s:%d: %s: %s\n", __FILE__, __LINE__, (message), #condition); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

/* Every operation on an integer type */
#define TEST_INTEGER(T, name)                                                                      \
    static _Atomic T name;                                                                         \
    static T         name##_plain;                                                                 \
    static void      test_##name(void)                                                             \
    {                                                                                              \
        T e;                                                                                       \
        atomic_init(&name, (T)5);                                                                  \
        check(atomic_load(&name) == 5, #T " init/load");                                           \
        atomic_store(&name, (T)7);                                                                 \
        check(atomic_load_explicit(&name, memory_order_acquire) == 7, #T " store");                \
        atomic_store_explicit(&name, (T)8, memory_order_release);                                  \
        check(atomic_exchange(&name, (T)9) == 8, #T " exchange");                                  \
        check(atomic_exchange_explicit(&name, (T)10, memory_order_acq_rel) == 9,                   \
              #T " exchange_explicit");                                                            \
        e = 3;                                                                                     \
        check(!atomic_compare_exchange_strong(&name, &e, (T)11), #T " failing cas");               \
        check(e == 10, #T " failing cas expected");                                                \
        check(atomic_compare_exchange_strong(&name, &e, (T)11), #T " cas");                        \
        e = 11;                                                                                    \
        while (!atomic_compare_exchange_weak(&name, &e, (T)12))                                    \
            ;                                                                                      \
        check(atomic_load(&name) == 12, #T " weak cas");                                           \
        e = 12;                                                                                    \
        check(atomic_compare_exchange_strong_explicit(&name, &e, (T)13, memory_order_seq_cst,      \
                                                      memory_order_relaxed),                       \
              #T " cas_explicit");                                                                 \
        e = 0;                                                                                     \
        while (!atomic_compare_exchange_weak_explicit(&name, &e, (T)0x30, memory_order_acq_rel,    \
                                                      memory_order_acquire))                       \
            ;                                                                                      \
        check(atomic_fetch_add(&name, (T)3) == 0x30, #T " fetch_add");                             \
        check(atomic_fetch_add_explicit(&name, (T)1, memory_order_relaxed) == 0x33,                \
              #T " fetch_add_explicit");                                                           \
        check(atomic_fetch_sub(&name, (T)4) == 0x34, #T " fetch_sub");                             \
        check(atomic_fetch_sub_explicit(&name, (T)1, memory_order_relaxed) == 0x30,                \
              #T " fetch_sub_explicit");                                                           \
        check(atomic_fetch_or(&name, (T)0x0f) == 0x2f, #T " fetch_or");                            \
        check(atomic_fetch_or_explicit(&name, (T)0x40, memory_order_relaxed) == 0x2f,              \
              #T " fetch_or_explicit");                                                            \
        check(atomic_fetch_and(&name, (T)0x3c) == 0x6f, #T " fetch_and");                          \
        check(atomic_fetch_and_explicit(&name, (T)0x1c, memory_order_relaxed) == 0x2c,             \
              #T " fetch_and_explicit");                                                           \
        check(atomic_fetch_xor(&name, (T)0x11) == 0x0c, #T " fetch_xor");                          \
        check(atomic_fetch_xor_explicit(&name, (T)0x11, memory_order_relaxed) == 0x1d,             \
              #T " fetch_xor_explicit");                                                           \
        check(atomic_load(&name) == 0x0c, #T " after fetch ops");                                  \
        name += 2;                                                                                 \
        name -= 1;                                                                                 \
        name |= 0x20;                                                                              \
        name &= 0x2f;                                                                              \
        name ^= 0x01;                                                                              \
        check(name == 0x2c, #T " compound assignment");                                            \
        check(name++ == 0x2c && ++name == 0x2e, #T " increment");                                  \
        check(name-- == 0x2e && --name == 0x2c, #T " decrement");                                  \
        /* The rest only exist as GCC builtins, on non-atomic types */                             \
        name##_plain = 0x0c;                                                                       \
        check(__atomic_add_fetch(&name##_plain, 1, __ATOMIC_SEQ_CST) == 0x0d, #T " add_fetch");    \
        check(__atomic_sub_fetch(&name##_plain, 2, __ATOMIC_SEQ_CST) == 0x0b, #T " sub_fetch");    \
        check(__atomic_and_fetch(&name##_plain, 0x0e, __ATOMIC_SEQ_CST) == 0x0a, #T " and_fetch"); \
        check(__atomic_or_fetch(&name##_plain, 0x30, __ATOMIC_SEQ_CST) == 0x3a, #T " or_fetch");   \
        check(__atomic_xor_fetch(&name##_plain, 0x0f, __ATOMIC_SEQ_CST) == 0x35, #T " xor_fetch"); \
        check(__atomic_fetch_nand(&name##_plain, 0x0f, __ATOMIC_SEQ_CST) == 0x35,                  \
              #T " fetch_nand");                                                                   \
        check(name##_plain == (T) ~(T)0x05, #T " after fetch_nand");                               \
        name##_plain = 0x35;                                                                       \
        check(__atomic_nand_fetch(&name##_plain, 0x0f, __ATOMIC_SEQ_CST) == (T) ~(T)0x05,          \
              #T " nand_fetch");                                                                   \
        /* Values using every byte */                                                              \
        atomic_store(&name, (T) ~(T)0);                                                            \
        check(atomic_fetch_add(&name, (T)1) == (T) ~(T)0, #T " wide fetch_add");                   \
        check(atomic_load(&name) == 0, #T " wide wrap");                                           \
        check(atomic_is_lock_free(&name) == __atomic_always_lock_free(sizeof(T), 0),               \
              #T " is_lock_free");                                                                 \
    }

TEST_INTEGER(uint8_t, atomic_u8)
TEST_INTEGER(uint16_t, atomic_u16)
TEST_INTEGER(uint32_t, atomic_u32)
TEST_INTEGER(uint64_t, atomic_u64)
#ifdef __SIZEOF_INT128__
TEST_INTEGER(unsigned __int128, atomic_u128)
#endif

/* Structures which are never lock-free */
#define BIG_WORDS 10

struct big {
    uint32_t w[BIG_WORDS];
};

struct odd {
    unsigned char c[3];
};

static struct big
big_of(uint32_t v)
{
    struct big b;

    for (int i = 0; i < BIG_WORDS; i++)
        b.w[i] = v;
    return b;
}

static bool
big_is(const struct big *b, uint32_t v)
{
    for (int i = 0; i < BIG_WORDS; i++)
        if (b->w[i] != v)
            return false;
    return true;
}

/*
 * Objects starting 40 bytes into a 64-byte block cross into the next
 * one, so each takes two of the library's lock slots. With the blocks
 * three apart, the spans cover every pair of adjacent slots, including
 * the one which wraps around the table.
 */
#define SPANS 16

struct span {
    _Alignas(64) char pad[40];
    _Atomic struct big v;
    char               tail[192 - 40 - sizeof(struct big)];
};

static struct span        spans[SPANS];
static _Atomic struct big big;
static _Atomic struct odd odd;
static const struct odd   odd456 = {
    { 4, 5, 6 }
};

static void
test_structs(void)
{
    struct big b, e;
    struct odd o, oe;

    atomic_init(&big, big_of(1));
    b = atomic_load(&big);
    check(big_is(&b, 1), "big init/load");
    atomic_store(&big, big_of(2));
    b = atomic_load_explicit(&big, memory_order_acquire);
    check(big_is(&b, 2), "big store");
    atomic_store_explicit(&big, big_of(3), memory_order_release);
    b = atomic_exchange(&big, big_of(4));
    check(big_is(&b, 3), "big exchange");
    b = atomic_exchange_explicit(&big, big_of(5), memory_order_acq_rel);
    check(big_is(&b, 4), "big exchange_explicit");
    e = big_of(1);
    check(!atomic_compare_exchange_strong(&big, &e, big_of(6)), "big failing cas");
    check(big_is(&e, 5), "big failing cas expected");
    check(atomic_compare_exchange_strong(&big, &e, big_of(6)), "big cas");
    e = big_of(6);
    while (!atomic_compare_exchange_weak(&big, &e, big_of(7)))
        ;
    e = big_of(7);
    check(atomic_compare_exchange_strong_explicit(&big, &e, big_of(8), memory_order_seq_cst,
                                                  memory_order_relaxed),
          "big cas_explicit");
    e = big_of(8);
    while (!atomic_compare_exchange_weak_explicit(&big, &e, big_of(9), memory_order_acq_rel,
                                                  memory_order_acquire))
        ;
    b = atomic_load(&big);
    check(big_is(&b, 9), "big weak cas_explicit");
    check(!atomic_is_lock_free(&big), "big is_lock_free");

    memset(&o, 0, sizeof(o));
    atomic_init(&odd, o);
    o.c[0] = 1;
    o.c[2] = 3;
    atomic_store(&odd, o);
    oe = atomic_exchange(&odd, odd456);
    check(oe.c[0] == 1 && oe.c[1] == 0 && oe.c[2] == 3, "odd exchange");
    oe = odd456;
    check(atomic_compare_exchange_strong(&odd, &oe, o), "odd cas");
    o = atomic_load(&odd);
    check(o.c[0] == 1 && o.c[1] == 0 && o.c[2] == 3, "odd load");
#ifdef __PICOLIBC__
    /* libatomic may find room for it in a lock-free word */
    check(atomic_is_lock_free(&odd) == __atomic_always_lock_free(sizeof(odd), 0),
          "odd is_lock_free");
#endif

    for (int i = 0; i < SPANS; i++) {
        atomic_init(&spans[i].v, big_of(0));
        b = atomic_exchange(&spans[i].v, big_of((uint32_t)i));
        check(big_is(&b, 0), "span exchange");
    }
    for (int i = 0; i < SPANS; i++) {
        b = atomic_load(&spans[i].v);
        check(big_is(&b, (uint32_t)i), "span load");
        atomic_store(&spans[i].v, big_of(0));
    }
}

static atomic_flag flag = ATOMIC_FLAG_INIT;

static void
test_flag(void)
{
    check(!atomic_flag_test_and_set(&flag), "first test_and_set");
    check(atomic_flag_test_and_set(&flag), "second test_and_set");
    atomic_flag_clear(&flag);
    check(!atomic_flag_test_and_set_explicit(&flag, memory_order_acquire),
          "test_and_set_explicit after clear");
    atomic_flag_clear_explicit(&flag, memory_order_release);
    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_seq_cst);
    check(kill_dependency(1) == 1, "kill_dependency");
}

/*
 * Contention: each worker adds one to every word of the big objects
 * with compare-and-exchange loops, checking each value it sees is
 * whole, and counts with the integer fetch operations.
 */
#define WORKERS    4
#define ITERATIONS 2000

static _Atomic uint64_t count_u64;
#ifdef __SIZEOF_INT128__
static _Atomic unsigned __int128 count_u128;
#endif

static void
big_increment(_Atomic struct big *a, const char *what)
{
    struct big old = atomic_load(a);
    struct big new;

    do {
        if (!big_is(&old, old.w[0])) {
            printf("%s: torn value\n", what);
            failures++;
            return;
        }
        new = big_of(old.w[0] + 1);
    } while (!atomic_compare_exchange_weak(a, &old, new));
}

static void *
worker(void *arg)
{
    unsigned id = (unsigned)(uintptr_t)arg;

    for (unsigned i = 0; i < ITERATIONS; i++) {
        /* Workers walk the spans in different orders */
        unsigned s = (id & 1) ? (i % SPANS) : (SPANS - 1 - i % SPANS);

        big_increment(&big, "big");
        big_increment(&spans[s].v, "span");
        atomic_fetch_add(&count_u64, 1);
#ifdef __SIZEOF_INT128__
        atomic_fetch_add(&count_u128, (unsigned __int128)1 << 64);
#endif
    }
    return NULL;
}

static void
test_contention(void)
{
    struct big b;
    uint64_t   total = 0;
    unsigned   workers;

    atomic_store(&big, big_of(0));
#if _POSIX_THREADS > 0
    pthread_t threads[WORKERS];

    workers = WORKERS;
    for (unsigned t = 0; t < WORKERS; t++)
        if (pthread_create(&threads[t], NULL, worker, (void *)(uintptr_t)t) != 0) {
            printf("pthread_create failed\n");
            exit(1);
        }
    for (unsigned t = 0; t < WORKERS; t++)
        pthread_join(threads[t], NULL);
#else
    workers = 1;
    worker((void *)(uintptr_t)0);
#endif
    b = atomic_load(&big);
    check(big_is(&b, workers * ITERATIONS), "big total");
    for (int i = 0; i < SPANS; i++) {
        b = atomic_load(&spans[i].v);
        check(big_is(&b, b.w[0]), "span whole");
        total += b.w[0];
    }
    check(total == (uint64_t)workers * ITERATIONS, "span total");
    check(atomic_load(&count_u64) == (uint64_t)workers * ITERATIONS, "u64 total");
#ifdef __SIZEOF_INT128__
    check(atomic_load(&count_u128) == (unsigned __int128)workers * ITERATIONS << 64, "u128 total");
#endif
}

int
main(void)
{
    test_atomic_u8();
    test_atomic_u16();
    test_atomic_u32();
    test_atomic_u64();
#ifdef __SIZEOF_INT128__
    test_atomic_u128();
#endif
    test_structs();
    test_flag();
    test_contention();
    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("success\n");
    return 0;
}
