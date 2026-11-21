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

#include "local-time.h"
#include <freebsd/freebsd-timespec-struct.h>

_Static_assert(sizeof(struct timespec) == sizeof(struct __freebsd_timespec),
               "timespec matches the kernel's");

/* Translate a clock id, returning -1 for one FreeBSD lacks */
int
_clock_to_freebsd(clockid_t clock_id)
{
    switch (clock_id) {
    case CLOCK_REALTIME:
        return FREEBSD_CLOCK_REALTIME;
    case CLOCK_REALTIME_COARSE:
        return FREEBSD_CLOCK_REALTIME_FAST;
    case CLOCK_MONOTONIC:
        return FREEBSD_CLOCK_MONOTONIC;
    case CLOCK_MONOTONIC_RAW:
        return FREEBSD_CLOCK_MONOTONIC_PRECISE;
    case CLOCK_MONOTONIC_COARSE:
        return FREEBSD_CLOCK_MONOTONIC_FAST;
    case CLOCK_BOOTTIME:
        return FREEBSD_CLOCK_BOOTTIME;
    case CLOCK_PROCESS_CPUTIME_ID:
        return FREEBSD_CLOCK_PROCESS_CPUTIME_ID;
    case CLOCK_THREAD_CPUTIME_ID:
        return FREEBSD_CLOCK_THREAD_CPUTIME_ID;
    default:
        return -1;
    }
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    int kclock = _clock_to_freebsd(clock_id);

    if (kclock < 0) {
        errno = EINVAL;
        return -1;
    }
    return _syscall(FREEBSD_SYS_clock_gettime, kclock, tp);
}
