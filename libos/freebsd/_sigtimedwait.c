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
#include <freebsd/freebsd-timespec-struct.h>

int
_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout, int *sig)
{
    struct __freebsd_sigset   kset;
    struct __freebsd_siginfo  kinfo;
    struct __freebsd_timespec kts, *pkts = NULL;
    long                      ret;
    int                       bad_timeout = 0;

    if (timeout) {
        /*
         * POSIX says to check the timeout only when the call would
         * wait: poll with a zero timeout instead, and report EINVAL if
         * nothing is pending. A negative interval has already expired.
         */
        bad_timeout = timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000L;
        if (bad_timeout || timeout->tv_sec < 0) {
            kts.tv_sec = 0;
            kts.tv_nsec = 0;
        } else {
            kts.tv_sec = timeout->tv_sec;
            kts.tv_nsec = timeout->tv_nsec;
        }
        pkts = &kts;
    }
    _sigmask_to_freebsd(&kset, set);

    ret = _syscall_raw(FREEBSD_SYS_sigtimedwait, &kset, &kinfo, pkts);
    if (ret < 0) {
        ret = _syscall_errno(ret);
        if (ret == EAGAIN && bad_timeout)
            ret = EINVAL;
        return (int)ret;
    }
    if (info)
        _siginfo_from_freebsd(info, &kinfo);
    *sig = _signal_from_freebsd((int)ret);
    return 0;
}
