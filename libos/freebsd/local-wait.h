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

#include "local-signal.h"
#include <sys/wait.h>
#include <freebsd/freebsd-wait.h>

/* FreeBSD reports a continued child with the status SIGCONT, 0x13 */
#define FREEBSD_W_CONTINUED 0x13

/* Translate the options of waitpid and wait3 */
static inline int
_wait_options_to_freebsd(int options)
{
    int koptions = 0;

    if (options & WNOHANG)
        koptions |= FREEBSD_WNOHANG;
    if (options & WUNTRACED)
        koptions |= FREEBSD_WUNTRACED;
    if (options & WCONTINUED)
        koptions |= FREEBSD_WCONTINUED;
    return koptions;
}

/*
 * Translate a wait status. The layout matches picolibc's, but the
 * signal numbers and the continued status differ.
 */
static inline int
_wait_status_from_freebsd(int kstatus)
{
    if (kstatus == FREEBSD_W_CONTINUED)
        return 0xffff;
    if ((kstatus & 0x7f) == 0x7f)
        return __W_EXITCODE(_signal_from_freebsd((kstatus >> 8) & 0xff), 0x7f);
    if ((kstatus & 0x7f) != 0)
        return __W_EXITCODE(0, _signal_from_freebsd(kstatus & 0x7f)) | (kstatus & __W_COREFLAG);
    return kstatus;
}

#endif /* _LOCAL_WAIT_H_ */
