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

#ifndef _LOCAL_SCHED_H_
#define _LOCAL_SCHED_H_

#include "local-linux.h"
#include <sched.h>

/* Linux scheduling policies, the same on every architecture */
#define LINUX_SCHED_NORMAL 0
#define LINUX_SCHED_FIFO   1
#define LINUX_SCHED_RR     2
#define LINUX_SCHED_BATCH  3
#define LINUX_SCHED_IDLE   5

struct __kernel_sched_param {
    int sched_priority;
};

/* Returns the Linux policy, or -1 for policies Linux doesn't support */
static inline int
_sched_policy_to_linux(int policy)
{
    switch (policy) {
    case SCHED_OTHER:
        return LINUX_SCHED_NORMAL;
    case SCHED_FIFO:
        return LINUX_SCHED_FIFO;
    case SCHED_RR:
        return LINUX_SCHED_RR;
#ifdef SCHED_BATCH
    case SCHED_BATCH:
        return LINUX_SCHED_BATCH;
#endif
#ifdef SCHED_IDLE
    case SCHED_IDLE:
        return LINUX_SCHED_IDLE;
#endif
    default:
        return -1;
    }
}

static inline int
_sched_policy_from_linux(int policy)
{
    switch (policy & ~0x40000000) { /* SCHED_RESET_ON_FORK */
    case LINUX_SCHED_FIFO:
        return SCHED_FIFO;
    case LINUX_SCHED_RR:
        return SCHED_RR;
#ifdef SCHED_BATCH
    case LINUX_SCHED_BATCH:
        return SCHED_BATCH;
#endif
#ifdef SCHED_IDLE
    case LINUX_SCHED_IDLE:
        return SCHED_IDLE;
#endif
    default:
        return SCHED_OTHER;
    }
}

#endif /* _LOCAL_SCHED_H_ */
