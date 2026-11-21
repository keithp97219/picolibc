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
#if _POSIX_THREADS > 0

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NTHREADS 8

/* Thread-specific values are per thread, destructors run at exit, and keys can be reused */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pthread_key_t   key, key2;
static pthread_mutex_t destructor_lock = PTHREAD_MUTEX_INITIALIZER;
static intptr_t        destructor_sum;
static int             destructor_calls;

static void
destructor(void *value)
{
    pthread_mutex_lock(&destructor_lock);
    destructor_sum += (intptr_t)value;
    destructor_calls++;
    pthread_mutex_unlock(&destructor_lock);
}

/* Sets a new value the first time it runs, so it runs twice */
static void
destructor_again(void *value)
{
    destructor(value);
    if ((intptr_t)value == 1000)
        pthread_setspecific(key2, (void *)2000);
}

static void *
use_key(void *arg)
{
    if (pthread_getspecific(key) != NULL)
        return (void *)1;
    if (pthread_setspecific(key, arg) != 0)
        return (void *)2;
    for (int i = 0; i < 1000; i++)
        if (pthread_getspecific(key) != arg)
            return (void *)3;
    if (pthread_setspecific(key2, (void *)1000) != 0)
        return (void *)4;
    return NULL;
}

int
main(void)
{
    pthread_t     threads[NTHREADS];
    void         *result;
    pthread_key_t keys[PTHREAD_KEYS_MAX];
    int           nkeys;

    /* Values are per-thread and destructors run at thread exit */
    check(pthread_key_create(&key, destructor) == 0, "pthread_key_create");
    check(pthread_key_create(&key2, destructor_again) == 0, "pthread_key_create 2");
    check(key != key2, "distinct keys");
    check(pthread_getspecific(key) == NULL, "initial value");
    check(pthread_setspecific(key, (void *)7) == 0, "pthread_setspecific main");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, use_key, (void *)(intptr_t)(i + 1)) == 0,
              "pthread_create key");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join key");
        check(result == NULL, "per-thread values");
    }
    check(pthread_getspecific(key) == (void *)7, "main thread value");
    check(destructor_calls == 3 * NTHREADS, "destructor calls");
    check(destructor_sum == NTHREADS * (NTHREADS + 1) / 2 + NTHREADS * 3000, "destructor values");

    /* Deleting a key; a new key in its place starts out NULL */
    check(pthread_key_delete(key) == 0, "pthread_key_delete");
    check(pthread_key_delete(key2) == 0, "pthread_key_delete 2");
    check(pthread_key_create(&key, NULL) == 0, "pthread_key_create again");
    check(pthread_getspecific(key) == NULL, "new key value");
    check(pthread_key_delete(key) == 0, "pthread_key_delete again");

    /* At least PTHREAD_KEYS_MAX keys are available */
    for (nkeys = 0; nkeys < PTHREAD_KEYS_MAX; nkeys++)
        if (pthread_key_create(&keys[nkeys], NULL) != 0)
            break;
    check(nkeys >= _POSIX_THREAD_KEYS_MAX, "number of keys");
    while (nkeys-- > 0)
        check(pthread_key_delete(keys[nkeys]) == 0, "pthread_key_delete");

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
