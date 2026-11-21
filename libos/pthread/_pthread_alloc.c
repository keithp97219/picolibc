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

#ifdef _PTHREAD_HAVE_MAP

/* All pages sizes used by the supported machines are multiples of this */
#define PTHREAD_PAGE_SIZE 4096

#define round_up(v, a)    (((v) + (a) - 1) & ~((size_t)(a) - 1))

/*
 * Each stack picolibc provides is an anonymous mapping, with the guard
 * region, if any, at its low end.
 */
void *
_pthread_os_stack_alloc(size_t size, size_t guard, void **map, size_t *map_size)
{
    size_t total;
    char  *m;

    /* Sizes too large to round up to pages can't be mapped; don't let them wrap */
    if (__picolibc_add_overflow(size, guard, &total)
        || __picolibc_add_overflow(total, 2 * PTHREAD_PAGE_SIZE, &total))
        return NULL;
    size = round_up(size, PTHREAD_PAGE_SIZE);
    guard = round_up(guard, PTHREAD_PAGE_SIZE);
    total = size + guard;

    /* Fresh anonymous memory reads as zero */
    m = _pthread_map(total);
    if (!m)
        return NULL;
    /* The guard region is best effort */
    if (guard)
        _pthread_guard(m, guard);
    *map = m;
    *map_size = total;
    return m + total;
}

void
_pthread_os_stack_free(void *map, size_t map_size)
{
    _pthread_unmap(map, map_size);
}

#endif /* _PTHREAD_HAVE_MAP */
