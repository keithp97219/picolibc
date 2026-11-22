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

#ifndef _LOCAL_RUSAGE_H_
#define _LOCAL_RUSAGE_H_

#include <sys/resource.h>

/*
 * Copy the kernel's struct rusage (struct __os_rusage from
 * local-os.h, the classic BSD layout on Linux and FreeBSD alike:
 * two timevals, then fourteen longs) into picolibc's
 */
#define _RUSAGE_FROM_OS(usage, kusage)                          \
    do {                                                        \
        (usage)->ru_utime.tv_sec = (kusage)->ru_utime.tv_sec;   \
        (usage)->ru_utime.tv_usec = (kusage)->ru_utime.tv_usec; \
        (usage)->ru_stime.tv_sec = (kusage)->ru_stime.tv_sec;   \
        (usage)->ru_stime.tv_usec = (kusage)->ru_stime.tv_usec; \
        (usage)->ru_maxrss = (kusage)->ru_maxrss;               \
        (usage)->ru_ixrss = (kusage)->ru_ixrss;                 \
        (usage)->ru_idrss = (kusage)->ru_idrss;                 \
        (usage)->ru_isrss = (kusage)->ru_isrss;                 \
        (usage)->ru_minflt = (kusage)->ru_minflt;               \
        (usage)->ru_majflt = (kusage)->ru_majflt;               \
        (usage)->ru_nswap = (kusage)->ru_nswap;                 \
        (usage)->ru_inblock = (kusage)->ru_inblock;             \
        (usage)->ru_oublock = (kusage)->ru_oublock;             \
        (usage)->ru_msgsnd = (kusage)->ru_msgsnd;               \
        (usage)->ru_msgrcv = (kusage)->ru_msgrcv;               \
        (usage)->ru_nsignals = (kusage)->ru_nsignals;           \
        (usage)->ru_nvcsw = (kusage)->ru_nvcsw;                 \
        (usage)->ru_nivcsw = (kusage)->ru_nivcsw;               \
    } while (0)

#endif /* _LOCAL_RUSAGE_H_ */
