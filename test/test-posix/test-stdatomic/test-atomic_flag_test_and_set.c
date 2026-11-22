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

#define _POSIX_C_SOURCE 202405L
/*
 * Call the <stdatomic.h> operations that C11 also defines as
 * functions, through their function names rather than the macros.
 */
#include <unistd.h>
#include "../test-flags.h"

/* Skipped without the atomics group; not an OS group, so built for every target */
#if TEST_POSIX_C_LANG_ATOMICS

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __clang__
/* clang warns about each operation which isn't lock-free */
#pragma clang diagnostic ignored "-Watomic-alignment"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* GCC's <stdatomic.h> has only the macros */
bool(atomic_flag_test_and_set)(volatile atomic_flag *);
bool(atomic_flag_test_and_set_explicit)(volatile atomic_flag *, memory_order);
void(atomic_flag_clear)(volatile atomic_flag *);
void(atomic_flag_clear_explicit)(volatile atomic_flag *, memory_order);
void(atomic_thread_fence)(memory_order);
void(atomic_signal_fence)(memory_order);

static atomic_flag flag = ATOMIC_FLAG_INIT;

int
main(void)
{
    bool (*test_and_set)(volatile atomic_flag *) = atomic_flag_test_and_set;
    void (*fence)(memory_order) = atomic_thread_fence;

    check((atomic_flag_test_and_set)(&flag) == false, "first test_and_set");
    check((atomic_flag_test_and_set)(&flag) == true, "second test_and_set");
    (atomic_flag_clear)(&flag);
    check((atomic_flag_test_and_set_explicit)(&flag, memory_order_acquire) == false,
          "test_and_set_explicit after clear");
    (atomic_flag_clear_explicit)(&flag, memory_order_release);
    check(test_and_set(&flag) == false, "through a pointer");
    (atomic_thread_fence)(memory_order_seq_cst);
    (atomic_signal_fence)(memory_order_seq_cst);
    fence(memory_order_acq_rel);

    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
