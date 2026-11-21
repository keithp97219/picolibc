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

#ifndef _LOCAL_UTIME_H_
#define _LOCAL_UTIME_H_

#include "local-freebsd.h"
#include <freebsd/freebsd-timespec-struct.h>
#include <freebsd/freebsd-utime.h>

/*
 * Convert utimensat's times, with UTIME_NOW and UTIME_OMIT, for the
 * kernel. FreeBSD's UTIME_NOW and UTIME_OMIT are negative, so other
 * values out of range are caught here, returning -1 with errno set.
 */
static inline int
_utimens_to_freebsd(struct __freebsd_timespec ktimes[2], const struct timespec times[2],
                    struct __freebsd_timespec **pktimes)
{
    *pktimes = NULL;
    if (!times)
        return 0;
    for (int i = 0; i < 2; i++) {
        ktimes[i].tv_sec = times[i].tv_sec;
        if (times[i].tv_nsec == UTIME_NOW)
            ktimes[i].tv_nsec = FREEBSD_UTIME_NOW;
        else if (times[i].tv_nsec == UTIME_OMIT)
            ktimes[i].tv_nsec = FREEBSD_UTIME_OMIT;
        else if (times[i].tv_nsec < 0 || times[i].tv_nsec >= 1000000000L) {
            errno = EINVAL;
            return -1;
        } else
            ktimes[i].tv_nsec = times[i].tv_nsec;
    }
    *pktimes = ktimes;
    return 0;
}

#endif /* _LOCAL_UTIME_H_ */
