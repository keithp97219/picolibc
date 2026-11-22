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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_C_LANG_THREADS) && _POSIX_THREADS > 0                  \
    && defined(__PICOLIBC_POSIX_SIGNALS) && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <time.h>

/*
 * tss_ destructors: how often they run (TSS_DTOR_ITERATIONS), what
 * tss_get says inside them, tss_delete and tss_set running none, and
 * none running at process termination. Thread exit also runs
 * pthread_key_create destructors.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static tss_t         key_always, key_twice, key_null, key_nodtor, key_deleted, key_late;
static pthread_key_t pkey;
static int           always_calls, twice_calls, null_calls, deleted_calls, pkey_calls;
static int           bad_value, bad_get;
static int           value, other;
static sem_t         created, go;

/* Sets its value again every time, so only the iteration limit stops it */
static void
dtor_always(void *p)
{
    always_calls++;
    if (p != &value)
        bad_value++;
    if (tss_get(key_always) != NULL)
        bad_get++;
    tss_set(key_always, &value);
}

/* Sets its value again twice, so it runs three times */
static void
dtor_twice(void *p)
{
    if (p != &other)
        bad_value++;
    if (tss_get(key_twice) != NULL)
        bad_get++;
    if (++twice_calls < 3)
        tss_set(key_twice, &other);
}

static void
dtor_null(void *p)
{
    (void)p;
    null_calls++;
}

static void
dtor_deleted(void *p)
{
    (void)p;
    deleted_calls++;
}

static void
dtor_pkey(void *p)
{
    if (p == &value)
        pkey_calls++;
}

static int
user(void *arg)
{
    (void)arg;
    if (tss_set(key_always, &value) != thrd_success || tss_set(key_twice, &other) != thrd_success
        || tss_set(key_nodtor, &value) != thrd_success
        || tss_set(key_deleted, &value) != thrd_success || pthread_setspecific(pkey, &value) != 0)
        return 1;
    /* tss_set doesn't run the destructor on the value it replaces */
    if (tss_set(key_twice, &value) != thrd_success || tss_set(key_twice, &other) != thrd_success)
        return 2;
    /* key_null keeps its null value */
    if (tss_get(key_null) != NULL)
        return 3;
    /* Deleting a key with a value runs no destructor, then or at exit */
    tss_delete(key_deleted);
    thrd_exit(0);
}

/* Started before key_late exists */
static int
early(void *arg)
{
    (void)arg;
    sem_post(&created);
    while (sem_wait(&go) != 0)
        ;
    return tss_get(key_late) == NULL ? 0 : 1;
}

static tss_t key_self;

static void
dtor_self(void *p)
{
    (void)p;
    /* tss_delete can be called from a destructor */
    tss_delete(key_self);
    deleted_calls += 100;
}

static int
self_deleter(void *arg)
{
    (void)arg;
    return tss_set(key_self, &value) == thrd_success ? 0 : 1;
}

/* In a child: destructors don't run at process termination */
static int   child_wfd;
static tss_t key_child;

static void
dtor_child(void *p)
{
    char c = 'D';

    (void)p;
    if (write(child_wfd, &c, 1) != 1)
        _exit(3);
}

static void
child(int wfd)
{
    child_wfd = wfd;
    if (tss_create(&key_child, dtor_child) != thrd_success
        || tss_set(key_child, &value) != thrd_success)
        _exit(2);
    exit(0);
}

int
main(void)
{
    thrd_t thr;
    int    res = -1, fds[2], status;
    pid_t  pid;
    char   buf[4];

    alarm(60);
    check(sem_init(&created, 0, 0) == 0 && sem_init(&go, 0, 0) == 0, "sem_init");

#if TSS_DTOR_ITERATIONS < 4
#error TSS_DTOR_ITERATIONS is below _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#endif

    check(tss_create(&key_always, dtor_always) == thrd_success, "tss_create");
    check(tss_create(&key_twice, dtor_twice) == thrd_success, "tss_create");
    check(tss_create(&key_null, dtor_null) == thrd_success, "tss_create");
    check(tss_create(&key_nodtor, NULL) == thrd_success, "tss_create");
    check(tss_create(&key_deleted, dtor_deleted) == thrd_success, "tss_create");
    check(pthread_key_create(&pkey, dtor_pkey) == 0, "pthread_key_create");
    check(key_always != key_twice && key_twice != key_null && key_null != key_nodtor,
          "distinct keys");

    check(thrd_create(&thr, user, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success && res == 0, "user");
    printf("TSS_DTOR_ITERATIONS %d, destructor that always sets a value ran %d times\n",
           TSS_DTOR_ITERATIONS, always_calls);
    check(always_calls >= TSS_DTOR_ITERATIONS && always_calls <= TSS_DTOR_ITERATIONS + 1,
          "destructor rounds stop at TSS_DTOR_ITERATIONS");
    check(twice_calls == 3, "destructor rounds repeat while values remain");
    check(bad_value == 0, "destructors get the previous value");
    check(bad_get == 0, "tss_get is null in the key's destructor");
    check(null_calls == 0, "no destructor call for null values");
    check(deleted_calls == 0, "tss_delete runs no destructor");
    check(pkey_calls == 1, "thrd_exit runs pthread_key_create destructors");

    /* tss_delete called from a destructor */
    check(tss_create(&key_self, dtor_self) == thrd_success, "tss_create");
    check(thrd_create(&thr, self_deleter, NULL) == thrd_success, "thrd_create");
    check(thrd_join(thr, &res) == thrd_success && res == 0, "self_deleter");
    check(deleted_calls == 100, "destructor that deletes its key ran once");

    /* A new key is null in threads that already exist */
    check(thrd_create(&thr, early, NULL) == thrd_success, "thrd_create");
    check(sem_wait(&created) == 0, "sem_wait");
    check(tss_create(&key_late, dtor_null) == thrd_success, "tss_create");
    check(tss_set(key_late, &value) == thrd_success, "tss_set");
    check(sem_post(&go) == 0, "sem_post");
    check(thrd_join(thr, &res) == thrd_success && res == 0, "new key null in existing thread");
    check(tss_get(key_late) == &value, "main's value");

    /* Destructors don't run at process termination */
    check(pipe(fds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(fds[0]);
        child(fds[1]);
    }
    close(fds[1]);
    check(read(fds[0], buf, sizeof(buf)) == 0, "no destructor output");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child exit status");
    check(null_calls == 0, "no destructor in the parent");

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
