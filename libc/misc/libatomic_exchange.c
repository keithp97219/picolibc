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

/* The generic __atomic_exchange, for any size */

#include "local-atomic.h"

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
