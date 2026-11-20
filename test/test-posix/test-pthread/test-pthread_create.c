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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Thread creation, termination, joining, detaching and attributes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NTHREADS 16
#define NLOOPS   10000

static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
static long            counter;

static __thread int    tls_value = 42;

static void *
return_arg(void *arg)
{
    return arg;
}

static void *
exit_arg(void *arg)
{
    pthread_exit(arg);
    return NULL;
}

static void *
count(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOPS; i++) {
        pthread_mutex_lock(&counter_lock);
        counter++;
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

/* Each thread starts with the initial TLS values and its own errno */
static void *
check_tls(void *arg)
{
    intptr_t id = (intptr_t)arg;

    if (tls_value != 42)
        return (void *)1;
    tls_value = (int)id;
    errno = (int)id;
    for (int i = 0; i < 1000; i++) {
        if (tls_value != id || errno != id)
            return (void *)2;
    }
    return NULL;
}

static void *
check_self(void *arg)
{
    pthread_t *self = arg;

    *self = pthread_self();
    return NULL;
}

static void *
join_self(void *arg)
{
    (void)arg;
    return (void *)(intptr_t)pthread_join(pthread_self(), NULL);
}

static void *
use_stack(void *arg)
{
    volatile char buf[8192];

    memset((char *)buf, 0x5a, sizeof(buf));
    return (void *)(uintptr_t)(buf[0] + buf[sizeof(buf) - 1] + (uintptr_t)arg);
}

static void *
print(void *arg)
{
    for (int i = 0; i < 100; i++)
        printf("thread %d line %d\n", (int)(intptr_t)arg, i);
    return NULL;
}

static pthread_mutex_t detach_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  detach_cond = PTHREAD_COND_INITIALIZER;
static int             detach_done;

static void *
detached(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&detach_lock);
    detach_done++;
    pthread_cond_signal(&detach_cond);
    pthread_mutex_unlock(&detach_lock);
    return NULL;
}

int
main(void)
{
    pthread_t      threads[NTHREADS];
    pthread_t      thread, other;
    pthread_attr_t attr;
    void          *result;
    int            ret;
    int            state;
    size_t         size, stack_size;
    void          *addr;

    /* Results from return and from pthread_exit */
    ret = pthread_create(&thread, NULL, return_arg, (void *)0x1234);
    check(ret == 0, "pthread_create");
    ret = pthread_join(thread, &result);
    check(ret == 0, "pthread_join");
    check(result == (void *)0x1234, "returned value");

    ret = pthread_create(&thread, NULL, exit_arg, (void *)0x5678);
    check(ret == 0, "pthread_create");
    ret = pthread_join(thread, &result);
    check(ret == 0, "pthread_join");
    check(result == (void *)0x5678, "pthread_exit value");

    /* Many threads contending for a mutex */
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, count, NULL) == 0, "pthread_create count");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join count");
    check(counter == (long)NTHREADS * NLOOPS, "mutex protected counter");

    /* Thread-local storage, including errno */
    errno = 0;
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&threads[i], NULL, check_tls, (void *)(intptr_t)(i + 1)) == 0,
              "pthread_create tls");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_join(threads[i], &result) == 0, "pthread_join tls");
        check(result == NULL, "thread-local values");
    }
    check(tls_value == 42, "main thread TLS unchanged");
    check(errno == 0, "main thread errno unchanged");

    /* pthread_self and pthread_equal */
    check(pthread_equal(pthread_self(), pthread_self()), "pthread_equal self");
    ret = pthread_create(&thread, NULL, check_self, &other);
    check(ret == 0, "pthread_create self");
    check(pthread_join(thread, NULL) == 0, "pthread_join self");
    check(pthread_equal(thread, other), "pthread_self in thread");
    check(!pthread_equal(thread, pthread_self()), "thread differs from main");

    /* Joining yourself deadlocks */
    check(pthread_join(pthread_self(), NULL) == EDEADLK, "main joins itself");
    ret = pthread_create(&thread, NULL, join_self, NULL);
    check(ret == 0, "pthread_create join_self");
    check(pthread_join(thread, &result) == 0, "pthread_join join_self");
    check(result == (void *)(intptr_t)EDEADLK, "thread joins itself");

    /* Default attributes */
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_getdetachstate(&attr, &state) == 0, "getdetachstate");
    check(state == PTHREAD_CREATE_JOINABLE, "default detach state");
    check(pthread_attr_getstacksize(&attr, &size) == 0, "getstacksize");
    check(size >= PTHREAD_STACK_MIN, "default stack size");
    check(pthread_attr_setdetachstate(&attr, -1) == EINVAL, "bad detach state");
    check(pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN - 1) == EINVAL, "small stack size");

    /*
     * Explicit stack size and guard size. PTHREAD_STACK_MIN only covers
     * starting the thread, so add room for use_stack.
     */
    stack_size = PTHREAD_STACK_MIN + 65536;
    check(pthread_attr_setstacksize(&attr, stack_size) == 0, "setstacksize");
    check(pthread_attr_getstacksize(&attr, &size) == 0 && size == stack_size, "getstacksize");
    check(pthread_attr_setguardsize(&attr, 8192) == 0, "setguardsize");
    check(pthread_attr_getguardsize(&attr, &size) == 0 && size == 8192, "getguardsize");
    ret = pthread_create(&thread, &attr, use_stack, (void *)1);
    check(ret == 0, "pthread_create with stack size");
    check(pthread_join(thread, &result) == 0, "pthread_join with stack size");
    check(result == (void *)(uintptr_t)(0x5a + 0x5a + 1), "stack size thread result");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");

    /* Application-provided stack */
    size = stack_size;
    addr = malloc(size);
    check(addr != NULL, "malloc stack");
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_setstack(&attr, addr, size) == 0, "setstack");
    {
        void  *get_addr;
        size_t get_size;
        check(pthread_attr_getstack(&attr, &get_addr, &get_size) == 0, "getstack");
        check(get_addr == addr && get_size == size, "getstack values");
    }
    ret = pthread_create(&thread, &attr, use_stack, (void *)2);
    check(ret == 0, "pthread_create with stack");
    check(pthread_join(thread, &result) == 0, "pthread_join with stack");
    check(result == (void *)(uintptr_t)(0x5a + 0x5a + 2), "application stack thread result");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");
    free(addr);

    /* Detached threads, both created detached and detached afterwards */
    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    check(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0, "setdetachstate");
    for (int i = 0; i < NTHREADS; i++)
        check(pthread_create(&thread, &attr, detached, NULL) == 0, "pthread_create detached");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");
    for (int i = 0; i < NTHREADS; i++) {
        check(pthread_create(&threads[i], NULL, detached, NULL) == 0, "pthread_create");
        check(pthread_detach(threads[i]) == 0, "pthread_detach");
    }
    pthread_mutex_lock(&detach_lock);
    while (detach_done < 2 * NTHREADS)
        pthread_cond_wait(&detach_cond, &detach_lock);
    pthread_mutex_unlock(&detach_lock);

    /* Concurrent stdio */
    for (int i = 0; i < 4; i++)
        check(pthread_create(&threads[i], NULL, print, (void *)(intptr_t)i) == 0,
              "pthread_create print");
    for (int i = 0; i < 4; i++)
        check(pthread_join(threads[i], NULL) == 0, "pthread_join print");

    printf("success\n");
    return 0;
}
