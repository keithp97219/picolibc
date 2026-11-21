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
#include <string.h>

/* FreeBSD's sigset_t holds signal n in bit n - 1 of 32-bit words */
void
_sigmask_to_freebsd(struct __freebsd_sigset *kset, const sigset_t *set)
{
    int sig;

    memset(kset, 0, sizeof(*kset));
    for (sig = 1; sig <= SIGRTMAX; sig++) {
        if (sigismember(set, sig) == 1) {
            int ksig = _signal_to_freebsd(sig) - 1;

            kset->__bits[ksig / 32] |= (__uint32_t)1 << (ksig % 32);
        }
    }
}

void
_sigmask_from_freebsd(sigset_t *set, const struct __freebsd_sigset *kset)
{
    int sig;

    sigemptyset(set);
    for (sig = 1; sig <= SIGRTMAX; sig++) {
        int ksig = _signal_to_freebsd(sig) - 1;

        if (kset->__bits[ksig / 32] & ((__uint32_t)1 << (ksig % 32)))
            sigaddset(set, sig);
    }
}
