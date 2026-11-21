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
#include "local-poll.h"
#include <stdint.h>

/* Use the 64-bit time call where the kernel has one */
#ifdef LINUX_SYS_ppoll_time64
#define LINUX_SYS_ppoll_time_t LINUX_SYS_ppoll_time64
#else
#define LINUX_SYS_ppoll_time_t LINUX_SYS_ppoll
#endif

/* Convert a timeout, clamping it to the end of 32-bit time if needed */
static int
timeout_to_linux(struct __kernel_timespec64 *kts, const struct timespec *ts)
{
    if (ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }
    if (sizeof(kts->tv_sec) == 4 && ts->tv_sec > INT32_MAX)
        kts->tv_sec = INT32_MAX;
    else
        kts->tv_sec = ts->tv_sec;
    kts->tv_nsec = ts->tv_nsec;
    return 0;
}

int
ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask)
{
    struct __kernel_timespec64 kts, *pkts = NULL;
    __kernel_sigset_t          kmask, *pkmask = NULL;
    void                      *cancel;
    int                        ret;

    if (timeout) {
        if (timeout_to_linux(&kts, timeout) < 0)
            return -1;
        pkts = &kts;
    }
    if (sigmask) {
        _sigmask_to_linux(&kmask, sigmask);
        pkmask = &kmask;
    }
    _poll_fds_to_linux(fds, nfds);
    cancel = _cancel_enter();
    ret = _syscall(LINUX_SYS_ppoll_time_t, fds, nfds, pkts, pkmask, __KERNEL_NSIG_BYTES);
    _cancel_leave(cancel);
    _poll_fds_from_linux(fds, nfds);
    return ret;
}
