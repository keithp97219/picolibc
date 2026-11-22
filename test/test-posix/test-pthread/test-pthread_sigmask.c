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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Per-thread signal masks and pthread_kill */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static pthread_t             target;
static volatile pthread_t    caught_by;

static void
handler(int sig)
{
    (void)sig;
    caught_by = pthread_self();
    caught = 1;
}

static void *
wait_signal(void *arg)
{
    sigset_t set;

    (void)arg;
    /* New threads inherit the creator's mask */
    check(pthread_sigmask(SIG_BLOCK, NULL, &set) == 0, "pthread_sigmask query in thread");
    if (!sigismember(&set, SIGUSR2))
        return (void *)1;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(pthread_sigmask(SIG_UNBLOCK, &set, NULL) == 0, "pthread_sigmask unblock");
    while (!caught)
        sched_yield();
    return NULL;
}

int
main(void)
{
    struct sigaction act;
    sigset_t         set, old;
    void            *result;

    /* sigaction keeps the handler installed, where signal may reset it on delivery */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");

    /* Block SIGUSR1 and SIGUSR2 in the main thread */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    check(pthread_sigmask(SIG_BLOCK, &set, &old) == 0, "pthread_sigmask block");
    check(!sigismember(&old, SIGUSR1), "SIGUSR1 initially unblocked");
    check(pthread_sigmask(SIG_BLOCK, NULL, &old) == 0, "pthread_sigmask query");
    check(sigismember(&old, SIGUSR1) && sigismember(&old, SIGUSR2), "signals blocked");
    check(pthread_sigmask(-1, &set, NULL) == EINVAL, "bad how");

    /* A signal sent to a thread reaches that thread */
    check(pthread_create(&target, NULL, wait_signal, NULL) == 0, "pthread_create");
    check(pthread_kill(target, 0) == 0, "pthread_kill with no signal");
    check(pthread_kill(target, SIGUSR1) == 0, "pthread_kill");
    check(pthread_join(target, &result) == 0, "pthread_join");
    check(result == NULL, "signal mask inherited");
    check(caught, "signal caught");
    check(pthread_equal(caught_by, target), "signal caught by target thread");

    check(pthread_kill(pthread_self(), SIGRTMAX + 1) == EINVAL, "bad signal");

    /* A signal sent to the main thread waits until it is unblocked */
    caught = 0;
    check(pthread_kill(pthread_self(), SIGUSR1) == 0, "pthread_kill self");
    check(!caught, "blocked signal pending");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(pthread_sigmask(SIG_UNBLOCK, &set, NULL) == 0, "pthread_sigmask unblock");
    check(caught, "signal delivered once unblocked");
    check(pthread_equal(caught_by, pthread_self()), "signal caught by main thread");

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
