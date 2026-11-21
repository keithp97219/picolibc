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

#include "local-wait.h"
#include "local-cancel.h"

/*
 * FreeBSD queues SIGCHLD with each child's status and takes it from
 * the queue when the status is collected, as POSIX asks.
 */
pid_t
waitpid(pid_t pid, int *wstatus, int options)
{
    void *cancel;
    pid_t ret;
    int   kstatus = 0;

    /* A pending cancellation acts even when the options are bad */
    cancel = _cancel_enter();
    if (options & ~(WNOHANG | WUNTRACED | WCONTINUED)) {
        _cancel_leave(cancel);
        errno = EINVAL;
        return -1;
    }
    ret = (pid_t)_syscall(FREEBSD_SYS_wait4, pid, &kstatus, _wait_options_to_freebsd(options),
                          NULL);
    _cancel_leave(cancel);
    /* With WNOHANG, 0 means no child changed state and there's no status */
    if (ret > 0 && wstatus)
        *wstatus = _wait_status_from_freebsd(kstatus);
    return ret;
}
