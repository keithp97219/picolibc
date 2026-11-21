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

#include "local-signal.h"

/*
 * The kernel calls one handler, which translates the signal number and
 * siginfo before calling the application's. FreeBSD returns from a
 * handler through its own trampoline, so nothing else is needed.
 */
static struct sigaction _freebsd_actions[SIGRTMAX + 1];

static void
_freebsd_handler(int ksig, struct __freebsd_siginfo *kinfo, void *ucontext)
{
    int sig = _signal_from_freebsd(ksig);

    if (sig < 0)
        return;
    if (_freebsd_actions[sig].sa_flags & SA_SIGINFO) {
        siginfo_t info;

        _siginfo_from_freebsd(&info, kinfo);
        _freebsd_actions[sig].sa_sigaction(sig, &info, ucontext);
    } else {
        _freebsd_actions[sig].sa_handler(sig);
    }
}

int
sigaction(int sig, const struct sigaction * restrict act, struct sigaction * restrict oldact)
{
    struct __freebsd_sigaction kact, koldact;
    struct sigaction           saved;
    int                        ksig = _signal_to_freebsd(sig);

    if (ksig < 0) {
        errno = EINVAL;
        return -1;
    }
    saved = _freebsd_actions[sig];
    if (act) {
        kact.__sa_handler = (void *)act->sa_handler;
        if (act->sa_handler != SIG_DFL && act->sa_handler != SIG_IGN)
            kact.__sa_handler = (void *)_freebsd_handler;
        kact.sa_flags = FREEBSD_SA_SIGINFO;
        if (act->sa_flags & SA_NOCLDSTOP)
            kact.sa_flags |= FREEBSD_SA_NOCLDSTOP;
        if (act->sa_flags & SA_ONSTACK)
            kact.sa_flags |= FREEBSD_SA_ONSTACK;
        if (act->sa_flags & SA_RESETHAND)
            kact.sa_flags |= FREEBSD_SA_RESETHAND;
        if (act->sa_flags & SA_RESTART)
            kact.sa_flags |= FREEBSD_SA_RESTART;
        if (act->sa_flags & SA_NOCLDWAIT)
            kact.sa_flags |= FREEBSD_SA_NOCLDWAIT;
        if (act->sa_flags & SA_NODEFER)
            kact.sa_flags |= FREEBSD_SA_NODEFER;
        _sigmask_to_freebsd(&kact.sa_mask, &act->sa_mask);
        _freebsd_actions[sig] = *act;
    }
    if (_syscall(FREEBSD_SYS_sigaction, ksig, act ? &kact : NULL, &koldact) < 0) {
        _freebsd_actions[sig] = saved;
        return -1;
    }
    if (oldact) {
        if (koldact.__sa_handler == (void *)_freebsd_handler) {
            *oldact = saved;
        } else {
            oldact->sa_handler = (_sig_func_ptr)koldact.__sa_handler;
            oldact->sa_flags = 0;
            _sigmask_from_freebsd(&oldact->sa_mask, &koldact.sa_mask);
        }
    }
    return 0;
}
