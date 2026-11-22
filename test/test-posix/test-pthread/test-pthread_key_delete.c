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
#if TEST_POSIX_THREADS && TEST_POSIX_SEMAPHORES && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/* Thread-specific data: deleting keys, destructors and key exhaustion */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static sem_t         to_thread, to_main;
static pthread_key_t key, key2;
static volatile int  calls, bad_calls;
static void * volatile seen;

static void
bad_destructor(void *value)
{
    (void)value;
    bad_calls++;
}

/* Sets a value, then checks the next key main makes */
static void *
hold_value(void *arg)
{
    (void)arg;
    if (pthread_setspecific(key, (void *)1) != 0)
        return (void *)1;
    sem_post(&to_main);
    while (sem_wait(&to_thread) != 0)
        ;
    /* main deleted key and made key2, possibly in the same slot */
    if (pthread_getspecific(key2) != NULL)
        return (void *)2;
    return NULL;
}

/* In a destructor, the key being destroyed reads as NULL */
static void
null_destructor(void *value)
{
    (void)value;
    seen = pthread_getspecific(key);
    calls++;
}

static void *
set_key(void *arg)
{
    pthread_setspecific(key, arg);
    return NULL;
}

/* A destructor which always stores a new value */
static void
again_destructor(void *value)
{
    calls++;
    pthread_setspecific(key, value);
}

/* A destructor which deletes another key */
static volatile int delete_ret = -1;

static void
delete_destructor(void *value)
{
    (void)value;
    delete_ret = pthread_key_delete(key2);
    calls++;
}

/* The initial thread's destructors run when it calls pthread_exit */
static volatile int initial_destructor_ran;

static void
initial_destructor(void *value)
{
    initial_destructor_ran = value == (void *)9;
}

static void *
join_initial(void *arg)
{
    if (pthread_join(*(pthread_t *)arg, NULL) != 0)
        _exit(2);
    _exit(initial_destructor_ran ? 0 : 3);
}

static int
initial_child(void)
{
    static pthread_t initial;
    pthread_t        thread;
    pthread_key_t    k;

    if (pthread_key_create(&k, initial_destructor) != 0 || pthread_setspecific(k, (void *)9) != 0)
        return 1;
    initial = pthread_self();
    if (pthread_create(&thread, NULL, join_initial, &initial) != 0)
        return 1;
    pthread_exit(NULL);
}

int
main(void)
{
    pthread_t      thread;
    void          *result;
    pid_t          pid;
    int            status, ret;
    int            nkeys;
    pthread_key_t *keys;

    check(sem_init(&to_thread, 0, 0) == 0, "sem_init");
    check(sem_init(&to_main, 0, 0) == 0, "sem_init");

    /*
     * A deleted key's destructor isn't called at thread exit, and a
     * new key reads as NULL in threads which already exist
     */
    check(pthread_key_create(&key, bad_destructor) == 0, "pthread_key_create");
    check(pthread_create(&thread, NULL, hold_value, NULL) == 0, "pthread_create");
    check(sem_wait(&to_main) == 0, "sem_wait");
    check(pthread_key_delete(key) == 0, "pthread_key_delete with a value set");
    check(pthread_key_create(&key2, bad_destructor) == 0, "pthread_key_create");
    check(sem_post(&to_thread) == 0, "sem_post");
    check(pthread_join(thread, &result) == 0, "pthread_join");
    check(result != (void *)1, "pthread_setspecific");
    check(result == NULL, "new key is NULL in an existing thread");
    check(bad_calls == 0, "destructor of a deleted key called");
    check(pthread_key_delete(key2) == 0, "pthread_key_delete");

    /* The key being destroyed reads as NULL in its destructor */
    calls = 0;
    seen = (void *)1;
    check(pthread_key_create(&key, null_destructor) == 0, "pthread_key_create");
    check(pthread_create(&thread, NULL, set_key, (void *)5) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(calls == 1, "destructor called");
    check(seen == NULL, "pthread_getspecific in the destructor");

    /* No destructor call for a NULL value */
    calls = 0;
    check(pthread_create(&thread, NULL, set_key, NULL) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(calls == 0, "destructor called for NULL");
    check(pthread_key_delete(key) == 0, "pthread_key_delete");

    /* Destructors are repeated, at least PTHREAD_DESTRUCTOR_ITERATIONS times */
    calls = 0;
    check(pthread_key_create(&key, again_destructor) == 0, "pthread_key_create");
    check(pthread_create(&thread, NULL, set_key, (void *)6) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(calls >= PTHREAD_DESTRUCTOR_ITERATIONS, "destructor iterations");
    check(pthread_key_delete(key) == 0, "pthread_key_delete");

    /* pthread_key_delete can be called from a destructor */
    calls = 0;
    check(pthread_key_create(&key, delete_destructor) == 0, "pthread_key_create");
    check(pthread_key_create(&key2, NULL) == 0, "pthread_key_create");
    check(pthread_create(&thread, NULL, set_key, (void *)7) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(calls == 1 && delete_ret == 0, "pthread_key_delete in a destructor");
    check(pthread_key_delete(key) == 0, "pthread_key_delete");

    /* The initial thread runs destructors in pthread_exit */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(initial_child());
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "initial thread destructors");

    /* Running out of keys is EAGAIN */
    keys = calloc(PTHREAD_KEYS_MAX + 1, sizeof(*keys));
    check(keys != NULL, "calloc");
    for (nkeys = 0; nkeys <= PTHREAD_KEYS_MAX; nkeys++) {
        ret = pthread_key_create(&keys[nkeys], NULL);
        if (ret != 0)
            break;
    }
    check(nkeys >= _POSIX_THREAD_KEYS_MAX, "number of keys");
    check(nkeys <= PTHREAD_KEYS_MAX, "more than PTHREAD_KEYS_MAX keys");
    check(ret == EAGAIN, "pthread_key_create EAGAIN");
    while (nkeys-- > 0)
        check(pthread_key_delete(keys[nkeys]) == 0, "pthread_key_delete");
    free(keys);

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
