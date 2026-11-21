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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* pthread_kill sends signals to a particular thread */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_t    target;
static volatile int caught_by_target;

static void
handler(int sig)
{
    (void)sig;
    if (pthread_equal(pthread_self(), target))
        caught_by_target = 1;
}

static void *
wait_for_signal(void *arg)
{
    (void)arg;
    while (!caught_by_target)
        sched_yield();
    return NULL;
}

static void *
exit_now(void *arg)
{
    return arg;
}

int
main(void)
{
    pthread_t        zombie;
    struct timespec  pause = { .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 };
    struct sigaction sa = { .sa_handler = handler };

    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
    check(pthread_create(&target, NULL, wait_for_signal, NULL) == 0, "pthread_create");
    check(pthread_kill(target, SIGUSR1) == 0, "pthread_kill");
    check(pthread_join(target, NULL) == 0, "pthread_join");
    check(caught_by_target, "handled in the target thread");
    check(pthread_kill(pthread_self(), 0) == 0, "signal 0 checks the thread");
    check(pthread_kill(pthread_self(), 12345) == EINVAL, "EINVAL");
    check(pthread_kill(pthread_self(), -1) == EINVAL, "EINVAL for a negative signal");

    /*
     * A thread that has exited but not been joined is not an error. The
     * pause gives it time to exit; if it hasn't, the calls still succeed.
     */
    check(pthread_create(&zombie, NULL, exit_now, NULL) == 0, "pthread_create");
    nanosleep(&pause, NULL);
    check(pthread_kill(zombie, 0) == 0, "signal 0 to an exited thread");
    check(pthread_kill(zombie, SIGUSR1) == 0, "signal to an exited thread");
#ifndef __GLIBC__
    /* glibc returns 0 for an exited thread without checking the signal */
    check(pthread_kill(zombie, 12345) == EINVAL, "EINVAL for an exited thread");
#endif
    check(pthread_join(zombie, NULL) == 0, "pthread_join");
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
