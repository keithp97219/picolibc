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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* sigprocmask blocks signals, which stay pending until unblocked */

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
    sigset_t set, old, cur;

    check(signal(SIGUSR1, handler) != SIG_ERR, "signal");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &set, &old) == 0, "SIG_BLOCK");
    check(!sigismember(&old, SIGUSR1), "was unblocked");
    check(raise(SIGUSR1) == 0 && caught == 0, "blocked");
    check(sigprocmask(SIG_SETMASK, NULL, &cur) == 0 && sigismember(&cur, SIGUSR1), "query");
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0 && caught == 1, "delivered on unblock");
    /* SIGKILL and SIGSTOP can't be blocked */
    sigfillset(&set);
    check(sigprocmask(SIG_SETMASK, &set, NULL) == 0, "block everything");
    check(sigprocmask(SIG_SETMASK, NULL, &cur) == 0, "query");
    check(!sigismember(&cur, SIGKILL) && !sigismember(&cur, SIGSTOP), "SIGKILL and SIGSTOP");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "restore");
    check(sigprocmask(42, &set, NULL) == -1 && errno == EINVAL, "EINVAL");
    check(sigprocmask(SIG_SETMASK, NULL, &cur) == 0 && !sigismember(&cur, SIGUSR2),
          "failed call leaves the mask alone");

    /* SIG_BLOCK forms the union, SIG_UNBLOCK removes, SIG_SETMASK replaces */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(sigprocmask(SIG_SETMASK, &set, NULL) == 0, "set USR1");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR2);
    check(sigprocmask(SIG_BLOCK, &set, &cur) == 0 && sigismember(&cur, SIGUSR1)
              && !sigismember(&cur, SIGUSR2),
          "oset holds the previous mask");
    check(sigprocmask(SIG_BLOCK, NULL, &cur) == 0 && sigismember(&cur, SIGUSR1)
              && sigismember(&cur, SIGUSR2),
          "SIG_BLOCK forms the union");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(sigprocmask(SIG_UNBLOCK, &set, NULL) == 0, "unblock USR1");
    check(sigprocmask(SIG_BLOCK, NULL, &cur) == 0 && !sigismember(&cur, SIGUSR1)
              && sigismember(&cur, SIGUSR2),
          "SIG_UNBLOCK removes only those signals");
    check(sigprocmask(SIG_SETMASK, &set, NULL) == 0, "set USR1");
    check(sigprocmask(SIG_BLOCK, NULL, &cur) == 0 && sigismember(&cur, SIGUSR1)
              && !sigismember(&cur, SIGUSR2),
          "SIG_SETMASK replaces the mask");

    /* With a null set, how doesn't matter and the mask is unchanged */
    check(sigprocmask(42, NULL, &cur) == 0, "how ignored without a set");
    check(sigismember(&cur, SIGUSR1) && !sigismember(&cur, SIGUSR2),
          "query with any how reports the mask");
    check(sigprocmask(SIG_UNBLOCK, NULL, &cur) == 0 && sigismember(&cur, SIGUSR1),
          "SIG_UNBLOCK without a set changes nothing");
#if !defined(__PICOLIBC__) || (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)
    /* pthread_sigmask returns the error number */
    check(pthread_sigmask(42, NULL, &cur) == 0, "pthread_sigmask: how ignored without a set");
    check(pthread_sigmask(42, &set, NULL) == EINVAL, "pthread_sigmask EINVAL");
#endif
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "restore");
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
