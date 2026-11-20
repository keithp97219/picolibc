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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0)

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* sigpending reports blocked signals waiting for delivery */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    (void)sig;
    caught++;
}

int
main(void)
{
    sigset_t set, pending;

    check(signal(SIGUSR2, handler) != SIG_ERR, "signal");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block");
    check(sigpending(&pending) == 0 && !sigismember(&pending, SIGUSR2), "nothing pending");
    check(raise(SIGUSR2) == 0, "raise");
    check(sigpending(&pending) == 0 && sigismember(&pending, SIGUSR2), "pending");
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0 && caught == 1, "delivered");
    check(sigpending(&pending) == 0 && !sigismember(&pending, SIGUSR2), "no longer pending");

    /* Signals pending on the process are reported too, and unblocked ones aren't pending */
    /* signal may reset the handler on delivery, as glibc's does */
    check(signal(SIGUSR2, handler) != SIG_ERR, "signal");
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block");
    check(kill(getpid(), SIGUSR2) == 0, "kill");
    check(sigpending(&pending) == 0 && sigismember(&pending, SIGUSR2), "pending on the process");
    check(!sigismember(&pending, SIGUSR1), "only the blocked, sent signal");
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0 && caught == 2, "delivered");
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
