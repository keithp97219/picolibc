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

/*
 * Wait for *addr to change from val, until abstime (measured on clock)
 * if abstime is not NULL.
 */
/*
 * Linux futexes are 32 bits, and the kernel writes the word for
 * priority inheritance, robust mutexes and the exit word (the tid)
 */
_Static_assert(sizeof(__pthread_futex_t) == 4, "the futex word is 32 bits");

int
_futex_wait(__pthread_futex_t *addr, __pthread_futex_t val, bool pshared, clockid_t clock,
            const struct timespec *abstime)
{
    struct __kernel_timespec64 kts, *pkts = NULL;
    int                        op;
    int                        ret;

    op = pshared ? 0 : LINUX_FUTEX_PRIVATE_FLAG;
    if (abstime) {
        if (clock == CLOCK_REALTIME)
            op |= LINUX_FUTEX_CLOCK_REALTIME;
        else if (clock != CLOCK_MONOTONIC)
            return EINVAL;
        ret = _futex_timespec(&kts, abstime);
        if (ret)
            return ret;
        pkts = &kts;
        op |= LINUX_FUTEX_WAIT_BITSET;
    } else {
        op |= LINUX_FUTEX_WAIT;
    }

    ret = (int)_syscall_raw(LINUX_SYS_futex_time_t, addr, op, val, pkts, NULL,
                            LINUX_FUTEX_BITSET_MATCH_ANY);
    if (ret < 0)
        return _syscall_errno(ret);
    return 0;
}
