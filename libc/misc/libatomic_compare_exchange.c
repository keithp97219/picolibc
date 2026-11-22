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

/* The generic __atomic_compare_exchange, for any size */

#include "local-atomic.h"

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
