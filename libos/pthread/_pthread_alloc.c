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

#include <picotls.h>
#include <stdalign.h>
#include <stddef.h>

/* All pages sizes used by the supported machines are multiples of this */
#define PTHREAD_PAGE_SIZE 4096

#define round_up(v, a)    (((v) + (a) - 1) & ~((uintptr_t)(a) - 1))
#define round_down(v, a)  ((v) & ~((uintptr_t)(a) - 1))

/*
 * Each thread gets one anonymous mapping holding, from low to high
 * addresses, a guard region, the stack (unless the application
 * provided one), the TLS block and the thread descriptor followed by
 * its thread-specific data values.
 */
int
_pthread_os_alloc(const pthread_attr_t *attr, struct __pthread **tp)
{
    size_t            align, tls_size, desc_size, stack_size, guard_size, map_size;
    char             *map;
    uintptr_t         top, stack_top;
    struct __pthread *t;
    char             *tls;

    align = _tls_align();
    if (align < alignof(max_align_t))
        align = alignof(max_align_t);
    tls_size = round_up(_tls_size(), align);
    desc_size = round_up(
        sizeof(struct __pthread) + __PTHREAD_KEYS_MAX * sizeof(struct __pthread_specific), align);
    stack_size = 0;
    guard_size = 0;
    if (!attr->__stackaddr) {
        stack_size = attr->__stacksize;
        guard_size = attr->__guardsize;
    }

    /*
     * Sizes too large to add up, with room for rounding each to pages,
     * can't be mapped; don't let them wrap around to small ones
     */
    if (__picolibc_add_overflow(guard_size, stack_size, &map_size)
        || __picolibc_add_overflow(map_size, tls_size + desc_size + align + 3 * PTHREAD_PAGE_SIZE,
                                   &map_size))
        return ENOMEM;

    stack_size = round_up(stack_size, PTHREAD_PAGE_SIZE);
    guard_size = round_up(guard_size, PTHREAD_PAGE_SIZE);
    map_size = round_up(guard_size + stack_size + tls_size + desc_size + align, PTHREAD_PAGE_SIZE);

    /* Fresh anonymous memory reads as zero */
    map = _pthread_map(map_size);
    if (!map)
        return ENOMEM;
    /* The guard region is best effort */
    if (guard_size)
        _pthread_guard(map, guard_size);

    top = (uintptr_t)map + map_size;
    t = (struct __pthread *)round_down(top - desc_size, align);
    t->specific = (struct __pthread_specific *)(t + 1);
    tls = (char *)round_down((uintptr_t)t - tls_size, align);

    _init_tls(tls);

    if (attr->__stackaddr)
        stack_top = (uintptr_t)attr->__stackaddr;
    else
        stack_top = (uintptr_t)tls;
    stack_top = round_down(stack_top, 16);

    t->map = map;
    t->map_size = map_size;
    t->stack_top = (void *)stack_top;
    t->tls = tls;
    *tp = t;
    return 0;
}

void
_pthread_os_free(struct __pthread *t)
{
    _pthread_unmap(t->map, t->map_size);
}

#endif /* _PTHREAD_HAVE_MAP */
