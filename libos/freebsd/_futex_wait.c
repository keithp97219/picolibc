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

#include "local-pthread.h"
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-umtx_time-struct.h>

/*
 * Wait for *addr to change from val, until abstime (measured on clock)
 * if abstime is not NULL. The kernel takes the clock and the absolute
 * time directly.
 */
int
_futex_wait(unsigned int *addr, unsigned int val, bool pshared, clockid_t clock,
            const struct timespec *abstime)
{
    struct __freebsd_umtx_time ut;
    void                      *size = NULL, *put = NULL;
    int  op = pshared ? FREEBSD_UMTX_OP_WAIT_UINT : FREEBSD_UMTX_OP_WAIT_UINT_PRIVATE;
    long ret;

    if (abstime) {
        if (clock != CLOCK_REALTIME && clock != CLOCK_MONOTONIC)
            return EINVAL;
        if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000L)
            return EINVAL;
        /*
         * The kernel waits forever for an absolute time of zero on
         * clocks other than CLOCK_REALTIME, but every clock has passed
         * that already
         */
        if (abstime->tv_sec < 0 || (abstime->tv_sec == 0 && abstime->tv_nsec == 0))
            return ETIMEDOUT;
        memset(&ut, 0, sizeof(ut));
        ut._timeout.tv_sec = abstime->tv_sec;
        ut._timeout.tv_nsec = abstime->tv_nsec;
        ut._flags = FREEBSD_UMTX_ABSTIME;
        ut._clockid = (__uint32_t)_clock_to_freebsd(clock);
        /* The size of the timeout structure goes in uaddr */
        size = (void *)(uintptr_t)sizeof(ut);
        put = &ut;
    }
    ret = _syscall_raw(FREEBSD_SYS__umtx_op, addr, op, (unsigned long)val, size, put);
    if (ret < 0)
        return _syscall_errno(ret);
    return 0;
}
