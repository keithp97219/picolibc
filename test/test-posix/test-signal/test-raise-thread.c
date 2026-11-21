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
#if defined(__PICOLIBC_POSIX_SIGNALS) && _POSIX_THREADS > 0

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* raise sends the signal to the calling thread, like pthread_kill(pthread_self(), sig) */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;
static pthread_t             caught_by;

static void
handler(int sig)
{
    caught_by = pthread_self();
    caught = sig;
}

static void *
raise_in_thread(void *arg)
{
    sigset_t set;

    (void)arg;
    /* Unblock SIGUSR1 here only, so no other thread could take it */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0)
        return (void *)"pthread_sigmask";
    if (raise(SIGUSR1) != 0)
        return (void *)"raise";
    /* The handler has run before raise returns */
    if (caught != SIGUSR1)
        return (void *)"handler not run before raise returned";
    if (!pthread_equal(caught_by, pthread_self()))
        return (void *)"handler ran in another thread";
    return NULL;
}

int
main(void)
{
    struct sigaction act;
    sigset_t         set, pending;
    pthread_t        thread;
    void            *result;

    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");

    /* In the main thread */
    check(raise(SIGUSR1) == 0 && caught == SIGUSR1, "handler ran before raise returned");
    check(pthread_equal(caught_by, pthread_self()), "handler ran in the calling thread");

    /* In another thread, with the signal blocked in the main thread */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(pthread_sigmask(SIG_BLOCK, &set, NULL) == 0, "block in main");
    caught = 0;
    check(pthread_create(&thread, NULL, raise_in_thread, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    if (result)
        printf("%s\n", (char *)result);
    check(result == NULL, "raise in a thread");
    check(sigpending(&pending) == 0 && !sigismember(&pending, SIGUSR1),
          "nothing left pending in the main thread");
    check(pthread_sigmask(SIG_UNBLOCK, &set, NULL) == 0, "unblock in main");

    /* A blocked signal stays pending for the thread */
    check(pthread_sigmask(SIG_BLOCK, &set, NULL) == 0, "block in main");
    caught = 0;
    check(raise(SIGUSR1) == 0 && caught == 0, "blocked");
    check(sigpending(&pending) == 0 && sigismember(&pending, SIGUSR1), "pending");
    check(pthread_sigmask(SIG_UNBLOCK, &set, NULL) == 0 && caught == SIGUSR1,
          "delivered when unblocked");

    /* Errors */
    errno = 0;
    check(raise(-1) != 0 && errno == EINVAL, "negative signal");
    errno = 0;
    check(raise(SIGRTMAX + 1) != 0 && errno == EINVAL, "past SIGRTMAX");

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
