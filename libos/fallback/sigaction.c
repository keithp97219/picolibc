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

/*
 * sigaction over the in-process signal table signal and raise use.
 * Only the handler is kept: a handler runs with its own signal
 * blocked, whatever sa_mask says, and the flags have no effect, so
 * oldact reports an empty mask and no flags. SA_SIGINFO handlers
 * aren't supported, as raise doesn't build a siginfo.
 */
#include "local-signal.h"

#ifndef __weak_reference
#define __fallback_sigaction sigaction
#endif

int
__fallback_sigaction(int sig, const struct sigaction * restrict act,
                     struct sigaction * restrict oldact)
{
    /* SIGKILL and SIGSTOP may be asked about, but not changed */
    if (sig <= 0 || sig >= _SIG_FUNC_COUNT
        || (act != NULL && (sig == SIGKILL || sig == SIGSTOP || (act->sa_flags & SA_SIGINFO)))) {
        errno = EINVAL;
        return -1;
    }
    if (oldact != NULL) {
        oldact->sa_handler = _sig_func[sig];
        sigemptyset(&oldact->sa_mask);
        oldact->sa_flags = 0;
    }
    if (act != NULL)
        _sig_func[sig] = act->sa_handler;
    return 0;
}

#ifdef __weak_reference
__weak_reference(__fallback_sigaction, sigaction);
#endif
