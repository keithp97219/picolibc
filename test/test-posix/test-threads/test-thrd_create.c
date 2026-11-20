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

/* thrd_create starts threads that thrd_join, thrd_exit and thrd_detach manage */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static thrd_t main_thread;

static int
child(void *arg)
{
    if (thrd_equal(thrd_current(), main_thread))
        return -1;
    thrd_yield();
    return *(int *)arg * 2;
}

static int
exiter(void *arg)
{
    (void)arg;
    thrd_exit(42);
}

int
main(void)
{
    struct timespec ts = { 0, 1000000 };
    thrd_t          thr;
    int             value = 21, res = 0;

    main_thread = thrd_current();
    check(thrd_equal(main_thread, thrd_current()), "thrd_equal");
    check(thrd_create(&thr, child, &value) == thrd_success, "thrd_create");
    check(!thrd_equal(thr, main_thread), "distinct threads");
    check(thrd_join(thr, &res) == thrd_success && res == 42, "thrd_join result");

    check(thrd_create(&thr, exiter, NULL) == thrd_success, "thrd_create exiter");
    check(thrd_join(thr, &res) == thrd_success && res == 42, "thrd_exit result");

    check(thrd_create(&thr, child, &value) == thrd_success, "thrd_create detached");
    check(thrd_detach(thr) == thrd_success, "thrd_detach");

    check(thrd_sleep(&ts, NULL) == 0, "thrd_sleep");
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
