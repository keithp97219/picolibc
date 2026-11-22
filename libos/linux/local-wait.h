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

#ifndef _LOCAL_WAIT_H_
#define _LOCAL_WAIT_H_

#include "local-sigaction.h"
#include "local-time.h"
#include <sys/wait.h>

/* Translate the options of waitpid and wait3 */
static inline int
_wait_options_to_linux(int options)
{
    int koptions = 0;

    if (options & WNOHANG)
        koptions |= LINUX_WNOHANG;
    if (options & WUNTRACED)
        koptions |= LINUX_WUNTRACED;
    if (options & WCONTINUED)
        koptions |= LINUX_WCONTINUED;
    return koptions;
}

/* Translate a wait status. The layout matches picolibc's; the signal numbers may not */
static inline int
_wait_status_from_linux(int kstatus)
{
    if (WIFSIGNALED(kstatus))
        return __W_EXITCODE(0, _signal_from_linux(WTERMSIG(kstatus))) | (kstatus & __W_COREFLAG);
    if (WIFSTOPPED(kstatus))
        return __W_EXITCODE(_signal_from_linux(WSTOPSIG(kstatus)), 0x7f);
    return kstatus;
}

/*
 * The kernel's struct rusage: two timevals in the kernel's layout,
 * which differs from picolibc's on 32-bit architectures, then
 * fourteen longs picolibc doesn't report.
 */
struct __kernel_wait_rusage {
    struct __kernel_timeval ru_utime;
    struct __kernel_timeval ru_stime;
    long                    ru_other[14];
};

/* Take a SIGCHLD a collected status leaves pending */
void _wait_reaped(void);

#endif /* _LOCAL_WAIT_H_ */
