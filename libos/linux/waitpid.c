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
#include "local-time.h"
#include <sys/wait.h>
#include <string.h>

/* Use the 64-bit time call where the kernel has one */
#ifdef LINUX_SYS_rt_sigtimedwait_time64
#define LINUX_SYS_rt_sigtimedwait_time_t LINUX_SYS_rt_sigtimedwait_time64
#else
#define LINUX_SYS_rt_sigtimedwait_time_t LINUX_SYS_rt_sigtimedwait
#endif

/*
 * Linux doesn't queue SIGCHLD, and leaves it pending after a wait.
 * POSIX: if SIGCHLD is blocked, a wait that returns a child's status
 * clears a pending SIGCHLD unless another child has status
 * available. Take the signal first and put it back if another child
 * has status, so one arriving in between isn't lost.
 */
static void
_waitpid_clear_sigchld(void)
{
    __kernel_sigset_t          kset;
    struct __kernel_siginfo    kinfo, kwinfo;
    struct __kernel_timespec64 kts = { 0 };

    if (_syscall_raw(LINUX_SYS_rt_sigprocmask, LINUX_SIG_BLOCK, NULL, &kset, __KERNEL_NSIG_BYTES)
            < 0
        || !__kernel_sigset_get_mask(&kset, __LINUX_SIGCHLD))
        return;
    if (_syscall_raw(LINUX_SYS_rt_sigpending, &kset, __KERNEL_NSIG_BYTES) < 0
        || !__kernel_sigset_get_mask(&kset, __LINUX_SIGCHLD))
        return;
    memset(&kset, 0, sizeof(kset));
    __kernel_sigset_set_mask(&kset, __LINUX_SIGCHLD);
    if (_syscall_raw(LINUX_SYS_rt_sigtimedwait_time_t, &kset, &kinfo, &kts, __KERNEL_NSIG_BYTES)
        != __LINUX_SIGCHLD)
        return;
    memset(&kwinfo, 0, sizeof(kwinfo));
    if (_syscall_raw(
            LINUX_SYS_waitid, LINUX_P_ALL, 0, &kwinfo,
            LINUX_WEXITED | LINUX_WSTOPPED | LINUX_WCONTINUED | LINUX_WNOHANG | LINUX_WNOWAIT, NULL)
            == 0
        && kwinfo.si_pid != 0)
        (void)_syscall_raw(LINUX_SYS_rt_sigqueueinfo, _syscall_raw(LINUX_SYS_getpid),
                           __LINUX_SIGCHLD, &kinfo);
}

pid_t
waitpid(pid_t pid, int *wstatus, int options)
{
    void *cancel;
    int   koptions = 0;
    pid_t ret;
    int   kstatus;

    if (options & WNOHANG)
        koptions |= LINUX_WNOHANG;
    if (options & WUNTRACED)
        koptions |= LINUX_WUNTRACED;
    if (options & WCONTINUED)
        koptions |= LINUX_WCONTINUED;
    /* A pending cancellation acts even when the options are bad */
    cancel = _cancel_enter();
    if (options & ~(WNOHANG | WUNTRACED | WCONTINUED)) {
        _cancel_leave(cancel);
        errno = EINVAL;
        return -1;
    }
    ret = _syscall(LINUX_SYS_wait4, pid, &kstatus, koptions, NULL);
    _cancel_leave(cancel);
    /* With WNOHANG, 0 means no child changed state and there's no status */
    if (ret > 0 && wstatus) {
        if (WIFSIGNALED(kstatus))
            kstatus
                = __W_EXITCODE(0, _signal_from_linux(WTERMSIG(kstatus))) | (kstatus & __W_COREFLAG);
        else if (WIFSTOPPED(kstatus))
            kstatus = __W_EXITCODE(_signal_from_linux(WSTOPSIG(kstatus)), 0x7f);
        *wstatus = kstatus;
    }
    if (ret > 0)
        _waitpid_clear_sigchld();
    return ret;
}
