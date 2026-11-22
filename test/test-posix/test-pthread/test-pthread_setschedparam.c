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
/* pthread_getconcurrency and pthread_setconcurrency are XSI interfaces */
#define _XOPEN_SOURCE 800
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_THREADS && TEST_POSIX_THREAD_PRIORITY_SCHEDULING

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Thread scheduling attributes and parameters */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void *
get_policy(void *arg)
{
    struct sched_param param;
    int                policy;

    (void)arg;
    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0)
        return (void *)-1;
    return (void *)(intptr_t)policy;
}

int
main(void)
{
    pthread_attr_t     attr;
    struct sched_param param;
    pthread_t          thread;
    void              *result;
    int                value, policy, ret;

    /* Attribute defaults and validation */
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_getinheritsched(&attr, &value) == 0 && value == PTHREAD_INHERIT_SCHED,
          "default inheritsched");
    check(pthread_attr_getscope(&attr, &value) == 0 && value == PTHREAD_SCOPE_SYSTEM,
          "default scope");
    check(pthread_attr_getschedpolicy(&attr, &value) == 0 && value == SCHED_OTHER,
          "default policy");
    check(pthread_attr_setinheritsched(&attr, -1) == EINVAL, "bad inheritsched");
    check(pthread_attr_setschedpolicy(&attr, -1) == EINVAL, "bad policy");
    check(pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS) == ENOTSUP, "process scope");
    check(pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) == 0, "system scope");

    check(pthread_attr_setschedpolicy(&attr, SCHED_RR) == 0, "setschedpolicy");
    check(pthread_attr_getschedpolicy(&attr, &value) == 0 && value == SCHED_RR, "getschedpolicy");
    param.sched_priority = 3;
    check(pthread_attr_setschedparam(&attr, &param) == 0, "setschedparam");
    param.sched_priority = 0;
    check(pthread_attr_getschedparam(&attr, &param) == 0 && param.sched_priority == 3,
          "getschedparam");
    check(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0, "setinheritsched");
    check(pthread_attr_getinheritsched(&attr, &value) == 0 && value == PTHREAD_EXPLICIT_SCHED,
          "getinheritsched");

    /* Explicit real-time scheduling may need privileges */
    ret = pthread_create(&thread, &attr, get_policy, NULL);
    check(ret == 0 || ret == EPERM, "pthread_create explicit SCHED_RR");
    if (ret == 0) {
        check(pthread_join(thread, &result) == 0, "pthread_join");
        check(result == (void *)(intptr_t)SCHED_RR, "thread runs with SCHED_RR");
    }

    /* Explicit SCHED_OTHER is always allowed */
    check(pthread_attr_setschedpolicy(&attr, SCHED_OTHER) == 0, "setschedpolicy other");
    param.sched_priority = 0;
    check(pthread_attr_setschedparam(&attr, &param) == 0, "setschedparam 0");
    check(pthread_create(&thread, &attr, get_policy, NULL) == 0, "pthread_create explicit other");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == (void *)(intptr_t)SCHED_OTHER, "thread runs with SCHED_OTHER");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");

    /* The running thread's parameters */
    check(pthread_getschedparam(pthread_self(), &policy, &param) == 0, "pthread_getschedparam");
    check(policy == SCHED_OTHER && param.sched_priority == 0, "main thread scheduling");
    check(pthread_setschedparam(pthread_self(), SCHED_OTHER, &param) == 0, "pthread_setschedparam");
    check(pthread_setschedparam(pthread_self(), -1, &param) == EINVAL, "bad policy");
    check(pthread_setschedprio(pthread_self(), 0) == 0, "pthread_setschedprio");
    check(pthread_setschedprio(pthread_self(), 1000) == EINVAL, "bad priority");

    /* Inherited scheduling */
    check(pthread_create(&thread, NULL, get_policy, NULL) == 0, "pthread_create");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result == (void *)(intptr_t)SCHED_OTHER, "inherited policy");

    /* Concurrency level hint */
    check(pthread_getconcurrency() == 0, "default concurrency");
    check(pthread_setconcurrency(-1) == EINVAL, "bad concurrency");
    check(pthread_setconcurrency(4) == 0, "pthread_setconcurrency");
    check(pthread_getconcurrency() == 4, "pthread_getconcurrency");
    check(pthread_setconcurrency(0) == 0, "reset concurrency");

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
