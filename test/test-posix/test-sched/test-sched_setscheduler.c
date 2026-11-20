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
#if _POSIX_PRIORITY_SCHEDULING > 0

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* sched_setscheduler sets the policy and returns the former one */

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
    struct sched_param param, before;
    int                policy = sched_getscheduler(0);
    int                ret;

    check(sched_getparam(0, &param) == 0, "sched_getparam");
    check(sched_setscheduler(0, policy, &param) == policy, "returns former policy");

    /* Real-time policies may need privileges */
    param.sched_priority = sched_get_priority_min(SCHED_FIFO);
    ret = sched_setscheduler(0, SCHED_FIFO, &param);
    if (ret != -1) {
        check(ret == policy, "former policy");
        check(sched_getscheduler(0) == SCHED_FIFO, "now SCHED_FIFO");
    } else {
        check(errno == EPERM, "EPERM");
    }
    check(sched_setscheduler(0, INT_MAX, &param) == -1 && errno == EINVAL, "bad policy");

    /* Priorities outside the policy's range; nothing changes on failure */
    policy = sched_getscheduler(0);
    check(sched_getparam(0, &before) == 0, "sched_getparam");
    param.sched_priority = sched_get_priority_max(SCHED_FIFO) + 1;
    errno = 0;
    check(sched_setscheduler(0, SCHED_FIFO, &param) == -1 && errno == EINVAL,
          "SCHED_FIFO priority above the range");
    param.sched_priority = sched_get_priority_min(SCHED_RR) - 1;
    errno = 0;
    check(sched_setscheduler(0, SCHED_RR, &param) == -1 && errno == EINVAL,
          "SCHED_RR priority below the range");
    param.sched_priority = sched_get_priority_max(SCHED_OTHER) + 1;
    errno = 0;
    check(sched_setscheduler(0, SCHED_OTHER, &param) == -1 && errno == EINVAL,
          "SCHED_OTHER priority above the range");
    check(sched_getscheduler(0) == policy, "policy unchanged");
    check(sched_getparam(0, &param) == 0 && param.sched_priority == before.sched_priority,
          "priority unchanged");

    /* A process this one may not signal belongs to someone else; Linux applies the same rule */
    errno = 0;
    if (kill(1, 0) == -1 && errno == EPERM) {
        check(sched_getparam(1, &param) == 0 || errno == EPERM, "sched_getparam(1)");
        param.sched_priority = sched_get_priority_min(SCHED_OTHER);
        errno = 0;
        check(sched_setscheduler(1, SCHED_OTHER, &param) == -1 && errno == EPERM,
              "another user's process");
    } else {
        printf("may signal process 1, skipping EPERM\n");
    }

    errno = 0;
    param.sched_priority = sched_get_priority_min(SCHED_OTHER);
    check(sched_setscheduler(INT_MAX, SCHED_OTHER, &param) == -1 && errno == ESRCH,
          "no such process");
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
