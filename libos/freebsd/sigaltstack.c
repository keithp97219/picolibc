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

#include "local-signal.h"
#include <freebsd/freebsd-sigaltstack.h>
#include <freebsd/freebsd-sigaltstack-struct.h>

/* picolibc's stack_t has its members in another order, and other SS_ values */
int
sigaltstack(const stack_t * __restrict ss, stack_t * __restrict old)
{
    struct __freebsd_sigaltstack kss, kold;

    if (ss) {
        if (ss->ss_flags & ~SS_DISABLE) {
            errno = EINVAL;
            return -1;
        }
        kss.ss_sp = ss->ss_sp;
        kss.ss_size = ss->ss_size;
        kss.ss_flags = (ss->ss_flags & SS_DISABLE) ? FREEBSD_SS_DISABLE : 0;
    }
    if (_syscall(FREEBSD_SYS_sigaltstack, ss ? &kss : NULL, old ? &kold : NULL) < 0)
        return -1;
    if (old) {
        old->ss_sp = kold.ss_sp;
        old->ss_size = kold.ss_size;
        old->ss_flags = 0;
        if (kold.ss_flags & FREEBSD_SS_DISABLE)
            old->ss_flags |= SS_DISABLE;
        if (kold.ss_flags & FREEBSD_SS_ONSTACK)
            old->ss_flags |= SS_ONSTACK;
    }
    return 0;
}
