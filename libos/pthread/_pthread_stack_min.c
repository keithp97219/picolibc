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
#include <picotls.h>
#include <stdalign.h>
#include <stddef.h>

#define round_up(v, a) (((v) + (a) - 1) & ~((size_t)(a) - 1))

void
_pthread_layout(struct _pthread_layout *layout)
{
    size_t align = _tls_align();

    /* Either TLS variant puts the block next to the thread pointer; align for both */
    if (align < alignof(max_align_t))
        align = alignof(max_align_t);
    layout->align = align;
    layout->tls_size = round_up(_tls_size(), align);
    layout->desc_size = round_up(
        sizeof(struct __pthread) + __PTHREAD_KEYS_MAX * sizeof(struct __pthread_specific), align);
    /* Room to align the top of the region down */
    layout->overhead = layout->tls_size + layout->desc_size + align;
}

long
__pthread_stack_min(void)
{
    struct _pthread_layout layout;

    _pthread_layout(&layout);
    return (long)(layout.overhead + __PTHREAD_STACK_WORK_MIN);
}
