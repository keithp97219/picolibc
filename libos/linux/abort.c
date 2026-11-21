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

#define _POSIX_C_SOURCE 202405L
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * POSIX: abort overrides blocking or ignoring SIGABRT, and the status
 * is that of a process killed by SIGABRT. A handler that returns gets
 * the signal once; then the default action is restored and the signal
 * sent again.
 */
void
abort(void)
{
    sigset_t         abrt;
    struct sigaction dfl = { .sa_handler = SIG_DFL };

    sigemptyset(&abrt);
    sigaddset(&abrt, SIGABRT);

    (void)sigprocmask(SIG_UNBLOCK, &abrt, NULL);
    (void)raise(SIGABRT);

    /* SIGABRT is ignored, or its handler returned */
    sigemptyset(&dfl.sa_mask);
    (void)sigaction(SIGABRT, &dfl, NULL);
    (void)sigprocmask(SIG_UNBLOCK, &abrt, NULL);
    (void)raise(SIGABRT);

    for (;;)
        _exit(127);
}
