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
/*
 * Whether delivery resets a handler installed with signal() is
 * implementation-defined. This test expects BSD semantics, where it
 * stays installed, as picolibc provides; glibc does that only with
 * _DEFAULT_SOURCE. Build for BSD signal semantics with other libraries.
 */
#ifdef TEST_GLIBC
#define _DEFAULT_SOURCE
#endif
#include <signal.h>
#include <assert.h>
#include "../test-flags.h"
#include <errno.h>
#include <stdio.h>

static int caught_usr1;

static void
handle_usr1(int sig)
{
    caught_usr1 = sig;
}

int
main(void)
{
    void (*old_func)(int);
    old_func = signal(SIGUSR1, handle_usr1);
    if (old_func == (void (*)(int))-1 && errno == EPERM) {
        printf("signal returned EPERM, assuming github CI\n");
        return 77;
    }
    if (old_func != SIG_DFL) {
        printf("signal returned %p instead of SIG_DFL (%p)\n", old_func, SIG_DFL);
        return 1;
    }
    kill(getpid(), SIGUSR1);
    if (caught_usr1 != SIGUSR1) {
        printf("caught %d instead of SIGUSR1(%d)\n", caught_usr1, SIGUSR1);
        return 1;
    }

    /* signal returns the func of the most recent call, without touching errno */
    errno = 12345;
    if (signal(SIGUSR1, SIG_IGN) != handle_usr1 || errno != 12345) {
        printf("signal didn't return the previous handler\n");
        return 1;
    }
    caught_usr1 = 0;
    raise(SIGUSR1);
    if (caught_usr1 != 0) {
        printf("SIG_IGN didn't ignore the signal\n");
        return 1;
    }
    if (signal(SIGUSR1, SIG_DFL) != SIG_IGN) {
        printf("signal didn't return SIG_IGN\n");
        return 1;
    }

#if TEST_POSIX_SIGNALS
    /* Invalid signals, and catching or ignoring SIGKILL, are EINVAL */
    errno = 0;
    if (signal(-1, handle_usr1) != SIG_ERR || errno != EINVAL) {
        printf("signal(-1) didn't fail with EINVAL\n");
        return 1;
    }
    errno = 0;
    if (signal(SIGKILL, handle_usr1) != SIG_ERR || errno != EINVAL) {
        printf("catching SIGKILL didn't fail with EINVAL\n");
        return 1;
    }
    errno = 0;
    if (signal(SIGSTOP, SIG_IGN) != SIG_ERR || errno != EINVAL) {
        printf("ignoring SIGSTOP didn't fail with EINVAL\n");
        return 1;
    }
#endif
    return 0;
}
