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
/* sigaltstack is an XSI interface */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* sigaltstack runs handlers on an alternate stack */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char        *stack_base;
static size_t       stack_size;
static volatile int on_stack;

static volatile int onstack_flag;
static volatile int modify_result;
static volatile int modify_errno;

static void
handler(int sig)
{
    char    here;
    stack_t cur, ss;

    (void)sig;
    on_stack = (uintptr_t)&here >= (uintptr_t)stack_base
        && (uintptr_t)&here < (uintptr_t)stack_base + stack_size;

    /* While running on the alternate stack, it reports SS_ONSTACK and can't be changed */
    onstack_flag = sigaltstack(NULL, &cur) == 0 && (cur.ss_flags & SS_ONSTACK);
    ss.ss_sp = stack_base;
    ss.ss_size = stack_size;
    ss.ss_flags = SS_DISABLE;
    errno = 0;
    modify_result = sigaltstack(&ss, NULL);
    modify_errno = errno;
}

int
main(void)
{
    struct sigaction sa;
    stack_t          ss, old;

    stack_size = SIGSTKSZ * 4;
    stack_base = malloc(stack_size);
    check(stack_base != NULL, "malloc");
    check(sigaltstack(NULL, &old) == 0 && (old.ss_flags & SS_DISABLE), "none yet");
    ss.ss_sp = stack_base;
    ss.ss_size = stack_size;
    ss.ss_flags = 0;
    check(sigaltstack(&ss, NULL) == 0, "sigaltstack");
    check(sigaltstack(NULL, &old) == 0 && old.ss_sp == stack_base && old.ss_size == stack_size,
          "installed");

    sa.sa_handler = handler;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    check(raise(SIGUSR1) == 0 && on_stack, "handler on the alternate stack");
    check(onstack_flag, "SS_ONSTACK while on the alternate stack");
    check(modify_result == -1 && modify_errno == EPERM, "EPERM changing the active stack");
    check(sigaltstack(NULL, &old) == 0 && !(old.ss_flags & SS_ONSTACK) && old.ss_sp == stack_base,
          "not on the stack after the handler");

    ss.ss_flags = SS_DISABLE;
    check(sigaltstack(&ss, NULL) == 0, "disable");
    check(raise(SIGUSR1) == 0 && !on_stack, "handler on the normal stack");
    ss.ss_flags = 0;
    ss.ss_size = 1;
    check(sigaltstack(&ss, NULL) == -1 && errno == ENOMEM, "too small");
    check(sigaltstack(NULL, &old) == 0 && (old.ss_flags & SS_DISABLE),
          "failed call leaves the stack alone");

    /* MINSIGSTKSZ is enough for sigaltstack */
    ss.ss_size = MINSIGSTKSZ;
    check(sigaltstack(&ss, NULL) == 0, "MINSIGSTKSZ accepted");
    check(sigaltstack(NULL, &old) == 0 && old.ss_size == MINSIGSTKSZ && old.ss_flags == 0,
          "MINSIGSTKSZ stack installed");

    /* SS_DISABLE ignores ss_sp and ss_size */
    ss.ss_sp = NULL;
    ss.ss_size = 0;
    ss.ss_flags = SS_DISABLE;
    check(sigaltstack(&ss, &old) == 0 && old.ss_sp == stack_base, "SS_DISABLE with no stack");
    check(sigaltstack(NULL, &old) == 0 && (old.ss_flags & SS_DISABLE), "disabled");

    /* SS_ONSTACK and unknown flags are EINVAL */
    ss.ss_sp = stack_base;
    ss.ss_size = stack_size;
    ss.ss_flags = SS_ONSTACK | SS_DISABLE;
    while (ss.ss_flags & (SS_ONSTACK | SS_DISABLE))
        ss.ss_flags <<= 1;
    errno = 0;
    check(sigaltstack(&ss, NULL) == -1 && errno == EINVAL, "unknown flag");
#ifndef __GLIBC__
    /* glibc passes SS_ONSTACK to Linux, which accepts it */
    ss.ss_flags = SS_ONSTACK;
    errno = 0;
    check(sigaltstack(&ss, NULL) == -1 && errno == EINVAL, "SS_ONSTACK");
#endif
    check(sigaltstack(NULL, &old) == 0 && (old.ss_flags & SS_DISABLE),
          "invalid flags leave the stack alone");
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
