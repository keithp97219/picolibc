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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 * wait, waitid and waitpid are cancellation points, and a pending
 * cancellation acts even when the call would fail with EINVAL
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

enum which { WAIT, WAITPID, WAITID, WAITPID_BAD, WAITID_BAD };

static volatile int returned;

static void *
waiter(void *arg)
{
    enum which which = (enum which)(intptr_t)arg;
    siginfo_t  info;
    int        old;

    /* Make the cancellation pending before the call */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    pthread_cancel(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
    switch (which) {
    case WAIT:
        (void)wait(NULL);
        break;
    case WAITPID:
        (void)waitpid(-1, NULL, WNOHANG);
        break;
    case WAITID:
        (void)waitid(P_ALL, 0, &info, WEXITED | WNOHANG);
        break;
    case WAITPID_BAD:
        (void)waitpid(-1, NULL, 0x100);
        break;
    case WAITID_BAD:
        (void)waitid(P_ALL, 0, &info, WEXITED | 0x100);
        break;
    }
    returned = 1;
    return NULL;
}

int
main(void)
{
    static const char * const names[]
        = { "wait", "waitpid", "waitid", "waitpid with bad options", "waitid with bad options" };
    pthread_t t;
    void     *result;
    int       i;

    for (i = WAIT; i <= WAITID_BAD; i++) {
        returned = 0;
        check(pthread_create(&t, NULL, waiter, (void *)(intptr_t)i) == 0, "pthread_create");
        check(pthread_join(t, &result) == 0, "pthread_join");
        if (result != PTHREAD_CANCELED || returned) {
            printf("%s didn't act on the pending cancellation\n", names[i]);
            return 1;
        }
    }
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
