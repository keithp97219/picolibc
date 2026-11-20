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

/* mtx_ mutexes lock, time out and recurse */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static mtx_t mtx;

static int
holder(void *arg)
{
    (void)arg;
    return mtx_trylock(&mtx);
}

int
main(void)
{
    struct timespec ts;
    thrd_t          thr;
    int             res;

    check(mtx_init(&mtx, mtx_timed) == thrd_success, "mtx_init");
    check(mtx_lock(&mtx) == thrd_success, "mtx_lock");
    check(thrd_create(&thr, holder, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success && res == thrd_busy, "trylock busy");
    check(timespec_get(&ts, TIME_UTC) == TIME_UTC, "timespec_get");
    ts.tv_nsec += 20000000;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    check(mtx_timedlock(&mtx, &ts) == thrd_success, "mtx_timedlock free");
    check(mtx_unlock(&mtx) == thrd_success, "mtx_unlock");
    mtx_destroy(&mtx);

    check(mtx_init(&mtx, mtx_plain | mtx_recursive) == thrd_success, "recursive");
    check(mtx_lock(&mtx) == thrd_success && mtx_lock(&mtx) == thrd_success, "relock");
    check(mtx_unlock(&mtx) == thrd_success && mtx_unlock(&mtx) == thrd_success, "unlock");
    mtx_destroy(&mtx);

#ifdef __PICOLIBC__
    /* C leaves other types undefined; picolibc rejects them */
    check(mtx_init(&mtx, 42) == thrd_error, "bad type");
#endif
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
