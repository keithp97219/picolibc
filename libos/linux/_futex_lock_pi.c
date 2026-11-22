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
#include "local-pthread-mutex.h"

/*
 * FUTEX_LOCK_PI measures its timeout on CLOCK_REALTIME. Newer kernels
 * add FUTEX_LOCK_PI2, which uses CLOCK_MONOTONIC; on older ones,
 * convert a monotonic deadline to realtime.
 */
static void
monotonic_to_realtime(struct timespec *rt, const struct timespec *abstime)
{
    struct timespec now_rt, now_mono;

    clock_gettime(CLOCK_REALTIME, &now_rt);
    clock_gettime(CLOCK_MONOTONIC, &now_mono);
    rt->tv_sec = now_rt.tv_sec + (abstime->tv_sec - now_mono.tv_sec);
    rt->tv_nsec = now_rt.tv_nsec + (abstime->tv_nsec - now_mono.tv_nsec);
    if (rt->tv_nsec < 0) {
        rt->tv_nsec += 1000000000L;
        rt->tv_sec--;
    } else if (rt->tv_nsec >= 1000000000L) {
        rt->tv_nsec -= 1000000000L;
        rt->tv_sec++;
    }
}

int
_futex_lock_pi(__pthread_futex_t *addr, bool pshared, clockid_t clock,
               const struct timespec *abstime)
{
    struct __kernel_timespec64 kts, *pkts = NULL;
    struct timespec            rt;
    int                        op = LINUX_FUTEX_LOCK_PI;
    int                        ret;

    if (abstime) {
        if (clock == CLOCK_MONOTONIC)
            op = LINUX_FUTEX_LOCK_PI2;
        else if (clock != CLOCK_REALTIME)
            return EINVAL;
        ret = _futex_timespec(&kts, abstime);
        if (ret)
            return ret;
        pkts = &kts;
    }
    if (!pshared)
        op |= LINUX_FUTEX_PRIVATE_FLAG;

    for (;;) {
        ret = (int)_syscall_raw(LINUX_SYS_futex_time_t, addr, op, 0, pkts, NULL, 0);
        if (ret == 0)
            break;
        ret = _syscall_errno(ret);
        if (ret == ENOSYS && (op & ~LINUX_FUTEX_PRIVATE_FLAG) == LINUX_FUTEX_LOCK_PI2) {
            monotonic_to_realtime(&rt, abstime);
            ret = _futex_timespec(&kts, &rt);
            if (ret)
                break;
            op = (op & LINUX_FUTEX_PRIVATE_FLAG) | LINUX_FUTEX_LOCK_PI;
            continue;
        }
        /* The owner may be exiting, or a signal arrived */
        if (ret != EAGAIN && ret != EINTR)
            break;
    }
    return ret;
}
