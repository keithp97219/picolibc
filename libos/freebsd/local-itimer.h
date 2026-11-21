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

#ifndef _LOCAL_ITIMER_H_
#define _LOCAL_ITIMER_H_

#include "local-freebsd.h"
#include <sys/time.h>
#include <freebsd/freebsd-timeval-struct.h>
#include <freebsd/freebsd-itimerval-struct.h>

/* Convert between picolibc's struct timeval and the kernel's */
static inline void
_timeval_to_freebsd(struct __freebsd_timeval *ktv, const struct timeval *tv)
{
    ktv->tv_sec = tv->tv_sec;
    ktv->tv_usec = tv->tv_usec;
}

static inline void
_timeval_from_freebsd(struct timeval *tv, const struct __freebsd_timeval *ktv)
{
    tv->tv_sec = ktv->tv_sec;
    tv->tv_usec = (suseconds_t)ktv->tv_usec;
}

/* Translate an interval timer, returning -1 for an unknown one */
static inline int
_itimer_to_freebsd(int which)
{
    switch (which) {
    case ITIMER_REAL:
        return FREEBSD_ITIMER_REAL;
    case ITIMER_VIRTUAL:
        return FREEBSD_ITIMER_VIRTUAL;
    case ITIMER_PROF:
        return FREEBSD_ITIMER_PROF;
    default:
        return -1;
    }
}

#endif /* _LOCAL_ITIMER_H_ */
