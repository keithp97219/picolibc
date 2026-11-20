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
/* sigaltstack and SA_ONSTACK are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || ((defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* an SA_SIGINFO handler's third argument is the ucontext_t of the context the signal interrupted */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t usr2_blocked, usr1_blocked, on_alt_stack, done;
static char                  alt_stack[1 << 16];

static void
handler(int sig, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;

    (void)sig;
    (void)info;
    usr2_blocked = sigismember(&uc->uc_sigmask, SIGUSR2);
    usr1_blocked = sigismember(&uc->uc_sigmask, SIGUSR1);
    on_alt_stack = uc->uc_stack.ss_sp == alt_stack && uc->uc_stack.ss_size == sizeof(alt_stack);
    /* Linux restores the mask from the context when the handler returns */
    sigaddset(&uc->uc_sigmask, SIGTERM);
    done = 1;
}

int
main(void)
{
    struct sigaction act;
    stack_t          ss;
    sigset_t         set, cur;
    mcontext_t      *mc;

    /* The types and members exist */
    mc = &((ucontext_t *)0)->uc_mcontext;
    (void)mc;

    memset(&ss, 0, sizeof(ss));
    ss.ss_sp = alt_stack;
    ss.ss_size = sizeof(alt_stack);
    check(sigaltstack(&ss, NULL) == 0, "sigaltstack");

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");

    /* The handler sees the mask of the interrupted context */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    check(sigprocmask(SIG_SETMASK, &set, NULL) == 0, "block SIGUSR2");
    check(raise(SIGUSR1) == 0 && done, "raise");
    check(usr2_blocked, "uc_sigmask holds the blocked signal");
    check(!usr1_blocked, "uc_sigmask is the interrupted mask, not the handler's");
    check(on_alt_stack, "uc_stack describes the alternate stack");

    /* A change to uc_sigmask takes effect when the handler returns */
    check(sigprocmask(SIG_SETMASK, NULL, &cur) == 0, "query mask");
    check(sigismember(&cur, SIGTERM) && sigismember(&cur, SIGUSR2), "mask from uc_sigmask");

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
