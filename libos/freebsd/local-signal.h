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

#ifndef _LOCAL_SIGNAL_H_
#define _LOCAL_SIGNAL_H_

#include "local-freebsd.h"
#include <freebsd/freebsd-signal.h>
#include <freebsd/freebsd-sigset-struct.h>
#include <freebsd/freebsd-siginfo-struct.h>
#include <freebsd/freebsd-sigaction-struct.h>

/*
 * picolibc's signals 1 to 31 are the BSD numbers, as in FreeBSD. Its
 * real-time range, SIGRTMIN to SIGRTMAX, maps to the start of
 * FreeBSD's; FreeBSD's 32 (SIGTHR) and 33 (SIGLIBRT) stay hidden.
 */
static inline int
_signal_to_freebsd(int sig)
{
    if (sig <= 0 || sig > SIGRTMAX)
        return -1;
    if (sig >= SIGRTMIN)
        return sig - SIGRTMIN + FREEBSD_SIGRTMIN;
    return sig;
}

static inline int
_signal_from_freebsd(int ksig)
{
    if (ksig >= FREEBSD_SIGRTMIN && ksig - FREEBSD_SIGRTMIN <= SIGRTMAX - SIGRTMIN)
        return ksig - FREEBSD_SIGRTMIN + SIGRTMIN;
    if (ksig > 0 && ksig < SIGRTMIN)
        return ksig;
    return -1;
}

void _sigmask_to_freebsd(struct __freebsd_sigset *kset, const sigset_t *set);
void _sigmask_from_freebsd(sigset_t *set, const struct __freebsd_sigset *kset);
void _siginfo_from_freebsd(siginfo_t *info, const struct __freebsd_siginfo *kinfo);

#endif /* _LOCAL_SIGNAL_H_ */
