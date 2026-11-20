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

#include "local-linux.h"
#include "local-time.h"
#include <sys/stat.h>

/* utimensat with 64-bit times where the kernel has it */
#ifdef LINUX_SYS_utimensat_time64
#define LINUX_SYS_utimensat_time_t LINUX_SYS_utimensat_time64
#else
#define LINUX_SYS_utimensat_time_t LINUX_SYS_utimensat
#endif

/* picolibc's UTIME_NOW and UTIME_OMIT match Linux's, so tv_nsec passes through */
int
futimens(int fd, const struct timespec times[2])
{
    struct __kernel_timespec_time_t ktimes[2], *pktimes = NULL;

    if (times) {
        for (int i = 0; i < 2; i++) {
            ktimes[i].tv_sec = times[i].tv_sec;
            ktimes[i].tv_nsec = times[i].tv_nsec;
        }
        pktimes = ktimes;
    }
    return (int)_syscall(LINUX_SYS_utimensat_time_t, fd, NULL, pktimes, 0);
}
