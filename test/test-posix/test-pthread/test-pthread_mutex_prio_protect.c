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
#if _POSIX_THREADS > 0 && _POSIX_THREAD_PRIO_PROTECT > 0 && _POSIX_THREAD_PRIORITY_SCHEDULING > 0

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * A thread holding PTHREAD_PRIO_PROTECT mutexes runs at the highest of
 * their ceilings and its own priority, whatever order it takes and
 * releases them in. Seeing the priority needs SCHED_FIFO, which only a
 * privileged process gets; otherwise this only exercises the
 * bookkeeping.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NMUTEX 4

static pthread_mutex_t mutex[NMUTEX];
static int             realtime;

static void
expect(int priority, const char *message)
{
    struct sched_param param;
    int                policy;

    if (!realtime)
        return;
    check(pthread_getschedparam(pthread_self(), &policy, &param) == 0, "getschedparam");
    if (param.sched_priority != priority) {
        printf("%s: priority %d, expected %d\n", message, param.sched_priority, priority);
        exit(1);
    }
}

int
main(void)
{
    pthread_mutexattr_t attr;
    struct sched_param  param;
    int                 min = sched_get_priority_min(SCHED_FIFO);
    int                 max = sched_get_priority_max(SCHED_FIFO);
    int                 base, ceiling[NMUTEX], i, ret;

    check(min >= 0 && max >= min + NMUTEX + 1, "SCHED_FIFO priority range");
    base = min;
    param.sched_priority = base;
    realtime = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == 0;
    if (!realtime) {
#ifdef TEST_GLIBC
        /* glibc refuses PTHREAD_PRIO_PROTECT mutexes to SCHED_OTHER threads with EINVAL */
        printf("glibc needs SCHED_FIFO for this, which needs privilege; skipping\n");
        return 77;
#endif
        printf("no SCHED_FIFO without privilege; checking the bookkeeping only\n");
    }

    /* Ceilings above the base priority, in no particular order */
    ceiling[0] = base + 2;
    ceiling[1] = base + 4;
    ceiling[2] = base + 1;
    ceiling[3] = base + 3;
    for (i = 0; i < NMUTEX; i++) {
        check(pthread_mutexattr_init(&attr) == 0, "mutexattr_init");
        check(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT) == 0, "setprotocol");
        check(pthread_mutexattr_setprioceiling(&attr, ceiling[i]) == 0, "setprioceiling");
#if _POSIX_THREAD_ROBUST_PRIO_PROTECT > 0
        /* A robust one too, which is on the robust list instead */
        if (i == 3)
            check(pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) == 0, "setrobust");
#endif
        check(pthread_mutex_init(&mutex[i], &attr) == 0, "mutex_init");
        check(pthread_mutexattr_destroy(&attr) == 0, "mutexattr_destroy");
    }

    for (i = 0; i < NMUTEX; i++) {
        ret = pthread_mutex_lock(&mutex[i]);
        check(ret == 0, "lock");
    }
    expect(base + 4, "holding all");

    /* Not in reverse order: the highest ceiling goes in the middle */
    check(pthread_mutex_unlock(&mutex[0]) == 0, "unlock 0");
    expect(base + 4, "after the second-lowest goes");
    check(pthread_mutex_unlock(&mutex[1]) == 0, "unlock 1");
    expect(base + 3, "after the highest goes");
    check(pthread_mutex_trylock(&mutex[1]) == 0, "trylock 1 again");
    expect(base + 4, "the highest again");
    check(pthread_mutex_unlock(&mutex[3]) == 0, "unlock 3");
    expect(base + 4, "the highest still held");
    check(pthread_mutex_unlock(&mutex[1]) == 0, "unlock 1 again");
    expect(base + 1, "only the lowest left");
    check(pthread_mutex_unlock(&mutex[2]) == 0, "unlock 2");
    expect(base, "nothing held");

    /* setprioceiling takes the mutex to change it, and leaves nothing behind */
    check(pthread_mutex_setprioceiling(&mutex[2], base + 5, &ret) == 0 && ret == base + 1,
          "setprioceiling");
    expect(base, "nothing held after setprioceiling");
    check(pthread_mutex_lock(&mutex[2]) == 0, "lock 2");
    expect(base + 5, "the new ceiling");
    check(pthread_mutex_unlock(&mutex[2]) == 0, "unlock 2");
    expect(base, "back to the base");

    for (i = 0; i < NMUTEX; i++)
        check(pthread_mutex_destroy(&mutex[i]) == 0, "mutex_destroy");
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
