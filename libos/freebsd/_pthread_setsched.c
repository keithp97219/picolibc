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

/*
 * Each thread's scheduling is its rtprio: real-time FIFO, real-time
 * round robin or time sharing. rtprio numbers real-time priorities 0
 * (highest) to 31; POSIX numbers them the other way around. Time
 * sharing has the single priority 0, as in sched_get_priority_max.
 */
int
_pthread_setsched(struct __pthread *thread, int policy, int priority)
{
    struct __freebsd_rtprio rtp;
    int                     tid = __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE);
    long                    ret;

    switch (policy) {
    case SCHED_FIFO:
        rtp.type = FREEBSD_RTP_PRIO_FIFO;
        break;
    case SCHED_RR:
        rtp.type = FREEBSD_RTP_PRIO_REALTIME;
        break;
    case SCHED_OTHER:
        rtp.type = FREEBSD_RTP_PRIO_NORMAL;
        break;
    default:
        return EINVAL;
    }
    if (policy == SCHED_OTHER) {
        if (priority != 0)
            return EINVAL;
        rtp.prio = 0;
    } else {
        if (priority < _PTHREAD_PRIO_MIN || priority > _PTHREAD_PRIO_MAX)
            return EINVAL;
        rtp.prio = (__uint16_t)(FREEBSD_RTP_PRIO_MAX - priority);
    }
    if (tid == 0)
        return ESRCH;
    ret = _syscall_raw(FREEBSD_SYS_rtprio_thread, FREEBSD_RTP_SET, (long)tid, &rtp);
    if (ret < 0)
        return _syscall_errno(ret);
    return 0;
}
