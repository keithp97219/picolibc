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

#include "local-sigaction.h"
#include "local-time.h"
#include "local-cancel.h"
#include <stdint.h>

/* Use the 64-bit time call where the kernel has one */
#ifdef LINUX_SYS_rt_sigtimedwait_time64
#define LINUX_SYS_rt_sigtimedwait_time_t LINUX_SYS_rt_sigtimedwait_time64
struct __kernel_sigtimedwait_timespec {
    __int64_t tv_sec;
    __int64_t tv_nsec;
};
#else
#define LINUX_SYS_rt_sigtimedwait_time_t LINUX_SYS_rt_sigtimedwait
#define __kernel_sigtimedwait_timespec   __kernel_timespec
#endif

int
sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout)
{
    __kernel_sigset_t                     kset;
    struct __kernel_siginfo               kinfo;
    struct __kernel_sigtimedwait_timespec kts, *pkts = NULL;
    void                                 *cancel;
    long                                  ret;

    if (timeout) {
        if (timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000L) {
            errno = EINVAL;
            return -1;
        }
        /* Clamp to the end of 32-bit time on kernels without 64-bit time */
        if (sizeof(kts.tv_sec) == 4 && timeout->tv_sec > INT32_MAX)
            kts.tv_sec = INT32_MAX;
        else
            kts.tv_sec = timeout->tv_sec;
        kts.tv_nsec = timeout->tv_nsec;
        pkts = &kts;
    }
    _sigmask_to_linux(&kset, set);

    cancel = _cancel_enter();
    ret = _syscall(LINUX_SYS_rt_sigtimedwait_time_t, &kset, &kinfo, pkts, __KERNEL_NSIG_BYTES);
    _cancel_leave(cancel);
    if (ret < 0)
        return -1;
    if (info)
        _siginfo_from_linux(info, &kinfo);
    return _signal_from_linux((int)ret);
}
