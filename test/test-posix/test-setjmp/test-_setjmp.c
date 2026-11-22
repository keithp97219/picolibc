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

#define _DEFAULT_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#if TEST_POSIX_SIGNALS
#include <signal.h>
#endif

/*
 * _setjmp and _longjmp (XSI before POSIX.1-2024, still used by Lua
 * and others): longjmp's return values, and the signal mask is left
 * alone
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static jmp_buf      env;
static volatile int calls;

static void
jump(int value)
{
    calls++;
    _longjmp(env, value);
}

int
main(void)
{
    volatile int round = 0;
    int          r;

    r = _setjmp(env);
    switch (round++) {
    case 0:
        check(r == 0, "_setjmp returns 0 directly");
        jump(42);
        break;
    case 1:
        check(r == 42, "_longjmp value");
        jump(0);
        break;
    case 2:
        check(r == 1, "_longjmp(env, 0) returns 1");
        break;
    default:
        check(0, "too many returns");
    }
    check(calls == 2, "two jumps");

#if TEST_POSIX_SIGNALS
    {
        sigset_t set, now;

        sigemptyset(&set);
        check(sigprocmask(SIG_SETMASK, &set, NULL) == 0, "clear mask");
        if (_setjmp(env) == 0) {
            sigaddset(&set, SIGUSR1);
            check(sigprocmask(SIG_BLOCK, &set, NULL) == 0, "block SIGUSR1");
            jump(1);
        }
        check(sigprocmask(SIG_SETMASK, NULL, &now) == 0, "read mask");
        check(sigismember(&now, SIGUSR1) == 1, "_longjmp leaves the signal mask");
    }
#endif

    printf("success\n");
    return 0;
}
