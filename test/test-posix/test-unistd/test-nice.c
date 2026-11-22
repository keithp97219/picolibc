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
/* nice is an XSI interface */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

/* nice adds to the nice value and returns the result */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    int prio = getpriority(PRIO_PROCESS, 0), ret;
    int privileged = 0;

    errno = 0;
    ret = nice(0);
    check(errno == 0 && ret == prio, "nice(0)");

    /* Lowering the nice value needs privileges; a failure leaves it alone */
    errno = 0;
    ret = nice(-1);
    if (ret == -1 && errno != 0) {
        check(errno == EPERM, "EPERM");
        check(getpriority(PRIO_PROCESS, 0) == prio, "unchanged after EPERM");
    } else {
        check(ret == (prio > -20 ? prio - 1 : -20), "nice(-1)");
        prio = ret;
        privileged = 1;
    }

    if (prio < 19) {
        errno = 0;
        ret = nice(1);
        check(errno == 0 && ret == prio + 1, "nice(1)");
        check(getpriority(PRIO_PROCESS, 0) == prio + 1, "applied");
    }
    errno = 0;
    ret = nice(100);
    check(errno == 0 && ret == 19, "clamped");
    check(getpriority(PRIO_PROCESS, 0) == 19, "clamped value applied");

    /* glibc adds incr to the nice value without a limit, so the sum overflows */
#ifndef __GLIBC__
    /* However large the increment, the value stops at the limit */
    errno = 0;
    ret = nice(INT_MAX);
    check(errno == 0 && ret == 19, "nice(INT_MAX)");
#endif
    errno = 0;
    ret = nice(INT_MIN);
    if (privileged)
        check(ret == -20, "nice(INT_MIN)");
    else
        check(ret == -1 && errno == EPERM && getpriority(PRIO_PROCESS, 0) == 19,
              "nice(INT_MIN) without privileges");

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
