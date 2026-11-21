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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if _POSIX_THREADS > 0 && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

/* nice changes the process, so every thread sees the new value, whichever thread calls it */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int go[2], done[2];
static int seen;

static void *
waiter(void *arg)
{
    char c;

    (void)arg;
    /* First report the value main set, then change it from here */
    if (read(go[0], &c, 1) != 1)
        return NULL;
    seen = getpriority(PRIO_PROCESS, 0);
    errno = 0;
    if (nice(1) == -1 && errno != 0)
        seen = -100;
    (void)write(done[1], "x", 1);
    return NULL;
}

int
main(void)
{
    pthread_t t;
    int       prio, ret;
    char      c;

    errno = 0;
    prio = nice(0);
    check(prio != -1 || errno == 0, "nice(0)");
    if (prio > 17) {
        printf("nice value %d leaves no room, skipping\n", prio);
        return 77;
    }
    check(pipe(go) == 0 && pipe(done) == 0, "pipe");
    check(pthread_create(&t, NULL, waiter, NULL) == 0, "pthread_create");

    errno = 0;
    ret = nice(1);
    check(ret == prio + 1 && errno == 0, "nice(1)");
    check(write(go[1], "x", 1) == 1 && read(done[0], &c, 1) == 1, "handshake");
    check(pthread_join(t, NULL) == 0, "pthread_join");
    check(seen != -100, "nice in the thread");
    /* glibc's nice changes only the calling thread */
#ifndef __GLIBC__
    check(seen == prio + 1, "the other thread has the new value");
    check(getpriority(PRIO_PROCESS, 0) == prio + 2, "the main thread has the thread's value");
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
