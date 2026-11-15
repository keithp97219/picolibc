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

#include "local-timer.h"

static int
timespec_to_linux(struct __kernel_timespec *kts, const struct timespec *ts)
{
    if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }
    /* Clamp to the end of 32-bit time on kernels without 64-bit time */
    if (sizeof(kts->tv_sec) == 4 && ts->tv_sec > INT32_MAX)
        kts->tv_sec = INT32_MAX;
    else
        kts->tv_sec = ts->tv_sec;
    kts->tv_nsec = ts->tv_nsec;
    return 0;
}

int
timer_settime(timer_t timerid, int flags, const struct itimerspec * __restrict value,
              struct itimerspec * __restrict ovalue)
{
    struct __kernel_itimerspec kvalue, kovalue;
    int                        kflags = 0;

    if (flags & ~TIMER_ABSTIME) {
        errno = EINVAL;
        return -1;
    }
    if (flags & TIMER_ABSTIME)
        kflags = LINUX_TIMER_ABSTIME;
    if (timespec_to_linux(&kvalue.it_interval, &value->it_interval) < 0
        || timespec_to_linux(&kvalue.it_value, &value->it_value) < 0)
        return -1;
    if (_syscall(LINUX_SYS_timer_settime, _timer_id(timerid), kflags, &kvalue,
                 ovalue ? &kovalue : NULL)
        < 0)
        return -1;
    if (ovalue) {
        ovalue->it_interval.tv_sec = kovalue.it_interval.tv_sec;
        ovalue->it_interval.tv_nsec = kovalue.it_interval.tv_nsec;
        ovalue->it_value.tv_sec = kovalue.it_value.tv_sec;
        ovalue->it_value.tv_nsec = kovalue.it_value.tv_nsec;
    }
    return 0;
}
