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
#if !defined(__PICOLIBC__) || (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

/* cnd_ condition variables wake waiters and time out */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static mtx_t mtx;
static cnd_t cnd;
static int   ready;

static int
waiter(void *arg)
{
    (void)arg;
    mtx_lock(&mtx);
    while (!ready)
        cnd_wait(&cnd, &mtx);
    mtx_unlock(&mtx);
    return 7;
}

int
main(void)
{
    struct timespec ts;
    thrd_t          thr[2];
    int             res, i;

    check(mtx_init(&mtx, mtx_plain) == thrd_success, "mtx_init");
    check(cnd_init(&cnd) == thrd_success, "cnd_init");

    /* Nobody signals, so this times out */
    check(timespec_get(&ts, TIME_UTC) == TIME_UTC, "timespec_get");
    ts.tv_nsec += 20000000;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    check(mtx_lock(&mtx) == thrd_success, "lock");
    check(cnd_timedwait(&cnd, &mtx, &ts) == thrd_timedout, "cnd_timedwait");
    check(mtx_unlock(&mtx) == thrd_success, "unlock");

    for (i = 0; i < 2; i++)
        check(thrd_create(&thr[i], waiter, NULL) == thrd_success, "thrd_create");
    mtx_lock(&mtx);
    ready = 1;
    check(cnd_broadcast(&cnd) == thrd_success, "cnd_broadcast");
    check(cnd_signal(&cnd) == thrd_success, "cnd_signal");
    mtx_unlock(&mtx);
    for (i = 0; i < 2; i++)
        check(thrd_join(thr[i], &res) == thrd_success && res == 7, "waiter");
    cnd_destroy(&cnd);
    mtx_destroy(&mtx);
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
