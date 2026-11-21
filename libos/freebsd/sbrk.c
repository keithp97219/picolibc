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

#include "local-freebsd.h"
#include <stdint.h>
#include <sys/mman.h>

/*
 * FreeBSD has dropped the break on newer architectures; take memory
 * from anonymous mappings. Each mapping reserves address space for the
 * next few requests so malloc's arena stays contiguous while it can.
 */

#define SBRK_CHUNK (1024 * 1024)
#define SBRK_MASK  (SBRK_CHUNK - 1)

static uintptr_t brk_cur;
static uintptr_t brk_end;

void * __disable_sanitizer
sbrk(intptr_t increment)
{
    uintptr_t old = brk_cur;

    if (increment < 0)
        return (void *)(uintptr_t)-1;
    if (brk_cur == 0 || brk_end - brk_cur < (uintptr_t)increment) {
        size_t size = ((size_t)increment + SBRK_MASK) & ~(size_t)SBRK_MASK;
        void  *map;

        /* Try to extend the current region in place first */
        map = mmap(brk_cur ? (void *)brk_end : NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (map == MAP_FAILED)
            return (void *)(uintptr_t)-1;
        if ((uintptr_t)map != brk_end) {
            /* Somewhere else: start a new region; malloc copes with the gap */
            old = (uintptr_t)map;
            brk_cur = old;
        }
        brk_end = (uintptr_t)map + size;
    }
    brk_cur = old + (uintptr_t)increment;
    return (void *)old;
}
