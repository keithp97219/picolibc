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

#include "local-sigaction.h"
#include "local-cancel.h"
#include <sys/wait.h>
#include <string.h>

int
waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
    struct __kernel_siginfo kinfo;
    int                     koptions = 0;
    void                   *cancel;
    long                    ret;

    if (options & WEXITED)
        koptions |= LINUX_WEXITED;
    if (options & WSTOPPED)
        koptions |= LINUX_WSTOPPED;
    if (options & WCONTINUED)
        koptions |= LINUX_WCONTINUED;
    if (options & WNOHANG)
        koptions |= LINUX_WNOHANG;
    if (options & WNOWAIT)
        koptions |= LINUX_WNOWAIT;

    /* With WNOHANG and no child ready, the kernel leaves this zero */
    memset(&kinfo, 0, sizeof(kinfo));
    cancel = _cancel_enter();
    ret = _syscall(LINUX_SYS_waitid, idtype, id, &kinfo, koptions, NULL);
    _cancel_leave(cancel);
    if (ret < 0)
        return -1;
    memset(infop, 0, sizeof(*infop));
    if (kinfo.si_signo)
        _siginfo_from_linux(infop, &kinfo);
    return 0;
}
