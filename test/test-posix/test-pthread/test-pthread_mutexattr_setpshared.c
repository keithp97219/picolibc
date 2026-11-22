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
#if TEST_POSIX_THREADS && TEST_POSIX_THREAD_PROCESS_SHARED

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The process-shared attribute of each kind of synchronization object:
 * private by default, set and read back, and EINVAL for other values
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Not PTHREAD_PROCESS_PRIVATE or PTHREAD_PROCESS_SHARED */
#define BAD_PSHARED (PTHREAD_PROCESS_PRIVATE + PTHREAD_PROCESS_SHARED + 7)

int
main(void)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;
    int                 pshared;

    check(pthread_mutexattr_init(&mattr) == 0, "pthread_mutexattr_init");
    check(pthread_mutexattr_getpshared(&mattr, &pshared) == 0 && pshared == PTHREAD_PROCESS_PRIVATE,
          "mutex private by default");
    check(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED) == 0,
          "pthread_mutexattr_setpshared");
    check(pthread_mutexattr_getpshared(&mattr, &pshared) == 0 && pshared == PTHREAD_PROCESS_SHARED,
          "mutex shared");
    check(pthread_mutexattr_setpshared(&mattr, BAD_PSHARED) == EINVAL, "mutex EINVAL");
    check(pthread_mutexattr_destroy(&mattr) == 0, "pthread_mutexattr_destroy");

    check(pthread_condattr_init(&cattr) == 0, "pthread_condattr_init");
    check(pthread_condattr_getpshared(&cattr, &pshared) == 0 && pshared == PTHREAD_PROCESS_PRIVATE,
          "condition variable private by default");
    check(pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED) == 0,
          "pthread_condattr_setpshared");
    check(pthread_condattr_getpshared(&cattr, &pshared) == 0 && pshared == PTHREAD_PROCESS_SHARED,
          "condition variable shared");
    check(pthread_condattr_setpshared(&cattr, BAD_PSHARED) == EINVAL, "condition variable EINVAL");
    check(pthread_condattr_destroy(&cattr) == 0, "pthread_condattr_destroy");

#if TEST_POSIX_READER_WRITER_LOCKS
    {
        pthread_rwlockattr_t rattr;

        check(pthread_rwlockattr_init(&rattr) == 0, "pthread_rwlockattr_init");
        check(pthread_rwlockattr_getpshared(&rattr, &pshared) == 0
                  && pshared == PTHREAD_PROCESS_PRIVATE,
              "rwlock private by default");
        check(pthread_rwlockattr_setpshared(&rattr, PTHREAD_PROCESS_SHARED) == 0,
              "pthread_rwlockattr_setpshared");
        check(pthread_rwlockattr_getpshared(&rattr, &pshared) == 0
                  && pshared == PTHREAD_PROCESS_SHARED,
              "rwlock shared");
        check(pthread_rwlockattr_setpshared(&rattr, BAD_PSHARED) == EINVAL, "rwlock EINVAL");
        check(pthread_rwlockattr_destroy(&rattr) == 0, "pthread_rwlockattr_destroy");
    }
#endif

#if TEST_POSIX_BARRIERS
    {
        pthread_barrierattr_t battr;

        check(pthread_barrierattr_init(&battr) == 0, "pthread_barrierattr_init");
        check(pthread_barrierattr_getpshared(&battr, &pshared) == 0
                  && pshared == PTHREAD_PROCESS_PRIVATE,
              "barrier private by default");
        check(pthread_barrierattr_setpshared(&battr, PTHREAD_PROCESS_SHARED) == 0,
              "pthread_barrierattr_setpshared");
        check(pthread_barrierattr_getpshared(&battr, &pshared) == 0
                  && pshared == PTHREAD_PROCESS_SHARED,
              "barrier shared");
        check(pthread_barrierattr_setpshared(&battr, BAD_PSHARED) == EINVAL, "barrier EINVAL");
        check(pthread_barrierattr_destroy(&battr) == 0, "pthread_barrierattr_destroy");
    }
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
