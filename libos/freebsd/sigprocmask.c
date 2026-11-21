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

int
sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    struct __freebsd_sigset kset, koldset;
    int                     khow;

    switch (how) {
    case SIG_BLOCK:
        khow = FREEBSD_SIG_BLOCK;
        break;
    case SIG_UNBLOCK:
        khow = FREEBSD_SIG_UNBLOCK;
        break;
    case SIG_SETMASK:
        khow = FREEBSD_SIG_SETMASK;
        break;
    default:
        /* Without a set, how is ignored */
        if (set) {
            errno = EINVAL;
            return -1;
        }
        khow = FREEBSD_SIG_BLOCK;
        break;
    }
    if (set)
        _sigmask_to_freebsd(&kset, set);
    if (_syscall(FREEBSD_SYS_sigprocmask, khow, set ? &kset : NULL, oldset ? &koldset : NULL) < 0)
        return -1;
    if (oldset)
        _sigmask_from_freebsd(oldset, &koldset);
    return 0;
}
