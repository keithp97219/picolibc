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

int
_pthread_getsched(struct __pthread *thread, int *policy, int *priority)
{
    struct __freebsd_rtprio rtp;
    int                     tid = __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE);
    long                    ret;

    if (tid == 0)
        return ESRCH;
    ret = _syscall_raw(FREEBSD_SYS_rtprio_thread, FREEBSD_RTP_LOOKUP, (long)tid, &rtp);
    if (ret < 0)
        return _syscall_errno(ret);
    switch (rtp.type) {
    case FREEBSD_RTP_PRIO_FIFO:
        *policy = SCHED_FIFO;
        *priority = FREEBSD_RTP_PRIO_MAX - rtp.prio;
        break;
    case FREEBSD_RTP_PRIO_REALTIME:
        *policy = SCHED_RR;
        *priority = FREEBSD_RTP_PRIO_MAX - rtp.prio;
        break;
    default:
        *policy = SCHED_OTHER;
        *priority = 0;
        break;
    }
    return 0;
}
