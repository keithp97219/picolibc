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

#include "local-freebsd.h"
#include <sys/times.h>
#include <time.h>
#include <freebsd/freebsd-resource.h>
#include <freebsd/freebsd-timeval-struct.h>
#include <freebsd/freebsd-rusage-struct.h>

/*
 * FreeBSD has no times system call: the process times come from
 * getrusage, in clock() ticks (CLOCKS_PER_SEC), and the elapsed time
 * from the monotonic clock.
 */
static clock_t
_ticks(const struct __freebsd_timeval *tv)
{
    return (clock_t)tv->tv_sec * CLOCKS_PER_SEC + (clock_t)tv->tv_usec * CLOCKS_PER_SEC / 1000000;
}

clock_t
times(struct tms *buf)
{
    struct __freebsd_rusage self, children;
    struct timespec         now;

    if (_syscall(FREEBSD_SYS_getrusage, FREEBSD_RUSAGE_SELF, &self) < 0
        || _syscall(FREEBSD_SYS_getrusage, FREEBSD_RUSAGE_CHILDREN, &children) < 0
        || clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return (clock_t)-1;
    buf->tms_utime = _ticks(&self.ru_utime);
    buf->tms_stime = _ticks(&self.ru_stime);
    buf->tms_cutime = _ticks(&children.ru_utime);
    buf->tms_cstime = _ticks(&children.ru_stime);
    return (clock_t)now.tv_sec * CLOCKS_PER_SEC
        + (clock_t)now.tv_nsec / (1000000000 / CLOCKS_PER_SEC);
}
