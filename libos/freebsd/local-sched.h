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

#include "local-freebsd.h"
#include <sched.h>
#include <freebsd/freebsd-sched.h>

/* FreeBSD's struct sched_param */
struct __freebsd_sched_param {
    int sched_priority;
};

/* Returns the FreeBSD policy, or -1 for policies FreeBSD doesn't support */
static inline int
_sched_policy_to_freebsd(int policy)
{
    switch (policy) {
    case SCHED_OTHER:
        return FREEBSD_SCHED_OTHER;
    case SCHED_FIFO:
        return FREEBSD_SCHED_FIFO;
    case SCHED_RR:
        return FREEBSD_SCHED_RR;
    default:
        return -1;
    }
}

static inline int
_sched_policy_from_freebsd(int policy)
{
    switch (policy) {
    case FREEBSD_SCHED_FIFO:
        return SCHED_FIFO;
    case FREEBSD_SCHED_RR:
        return SCHED_RR;
    default:
        return SCHED_OTHER;
    }
}

#endif /* _LOCAL_SCHED_H_ */
