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

/*
 * libos/pthread over a stub OS backend, the shape of an RTOS one such
 * as Zephyr's: thread creation places TLS itself
 * (_PTHREAD_HAVE_OS_TLS) and the system keeps running when the last
 * thread exits (_PTHREAD_HAVE_LAST_EXIT). This file is the OS half:
 * the hooks, and sysconf from libos/fallback's helper. The stub has
 * no scheduler; a created thread runs to completion on the caller's
 * stack when run_pending is called, and its pthread_exit returns
 * there with longjmp.
 */

#define _DEFAULT_SOURCE
#include "local-pthread.h"
#include "local-sysconf.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATTERN 0xa5

static int               errors;
static int               next_tid = 2;
static struct __pthread *pending;
static jmp_buf           pending_done;
static bool              last_exit_called;
static void             *alloc_base;
static void             *expected_base;
static size_t            alloc_size;
static int               allocs, frees;

#define check(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            errors++;                                                       \
        }                                                                   \
    } while (0)

long
sysconf(int name)
{
    return __sysconf_common(name);
}

/* One thread at a time and no blocking: a wait that would sleep can never be woken */
int
_futex_wait(__pthread_futex_t *addr, __pthread_futex_t val, bool pshared, clockid_t clock,
            const struct timespec *abstime)
{
    (void)pshared;
    (void)clock;
    (void)abstime;
    if (__atomic_load_n(addr, __ATOMIC_ACQUIRE) != val)
        return 0;
    printf("futex wait would block forever\n");
    exit(1);
}

void
_futex_wake(__pthread_futex_t *addr, int count, bool pshared)
{
    (void)addr;
    (void)count;
    (void)pshared;
}

void *
_pthread_os_stack_alloc(size_t size, size_t guard, void **map, size_t *map_size)
{
    char *base;

    (void)guard;
    base = malloc(size);
    if (!base)
        return NULL;
    memset(base, PATTERN, size);
    alloc_base = expected_base = base;
    alloc_size = size;
    allocs++;
    *map = base;
    *map_size = size;
    return base + size;
}

void
_pthread_os_stack_free(void *map, size_t map_size)
{
    check(map == alloc_base && map_size == alloc_size);
    frees++;
    free(map);
}

size_t
_pthread_os_tls_size(void)
{
    return STUB_OS_TLS_SIZE;
}

static bool
untouched(const char *from, const char *to)
{
    for (; from < to; from++)
        if (*(const unsigned char *)from != PATTERN)
            return false;
    return true;
}

/*
 * What an RTOS thread creation gets: the stack from t->stack_base to
 * t->stack_top, below the descriptor, with room for the OS's TLS and
 * nothing written there by the core
 */
int
_pthread_os_create(struct __pthread *t)
{
    char *base = t->stack_base;
    char *top = t->stack_top;

    check(t->tls == NULL);
    check(base == expected_base);
    check(base < top);
    check(top <= (char *)t);
    check(((uintptr_t)top & 15) == 0);
    check((size_t)(top - base) >= STUB_OS_TLS_SIZE + __PTHREAD_STACK_WORK_MIN);
    check(untouched(base, top));
    check(pending == NULL);

    t->tid = next_tid++;
    pending = t;
    return 0;
}

__noreturn void
_pthread_os_exit(struct __pthread *self)
{
    if (self == pending) {
        /* Done with the thread's memory: let pthread_join go */
        __atomic_store_n(_pthread_exit_word(self), _PTHREAD_EXIT_DONE, __ATOMIC_RELEASE);
        _futex_wake(_pthread_exit_word(self), INT_MAX, false);
        longjmp(pending_done, 1);
    }

    /* The initial thread, after the last pthread_exit */
    check(last_exit_called);
    printf("%d errors\n", errors);
    exit(errors ? 1 : 0);
}

void
_pthread_os_set_tid(struct __pthread *self)
{
    self->tid = 1;
}

/* There is no process to end; the thread ends alone */
void
_pthread_os_last_exit(struct __pthread *self)
{
    check(self->tid == 1);
    check(!last_exit_called);
    last_exit_called = true;
}

/* Run the thread pthread_create started, as far as its pthread_exit */
static void
run_pending(void)
{
    struct __pthread *self = _pthread_current;
    struct __pthread *t = pending;

    check(t != NULL);
    if (!t)
        return;
    t->os.started = 1;
    if (setjmp(pending_done) == 0)
        _pthread_start(t);
    pending = NULL;
    _pthread_current = self;
}

/* Exiting without _pthread_os_last_exit (the default exit(0)) is a failure */
static void
check_last_exit(void)
{
    if (!last_exit_called) {
        printf("exit without _pthread_os_last_exit\n");
        _exit(1);
    }
}

static void *
thread_main(void *arg)
{
    check(pthread_self() != NULL);
    return (char *)arg + 1;
}

static void
create_join(const pthread_attr_t *attr, char *arg)
{
    pthread_t t;
    void     *result = NULL;

    check(pthread_create(&t, attr, thread_main, arg) == 0);
    check(t->os.started == 0);
    run_pending();
    check(t->os.started == 1);
    check(pthread_join(t, &result) == 0);
    check(result == arg + 1);
}

int
main(void)
{
    static char    arg[2];
    pthread_attr_t attr;
    size_t         min = (size_t)sysconf(_SC_THREAD_STACK_MIN);
    size_t         stacksize;
    char          *stack;

    atexit(check_last_exit);

    /* The minimum covers the OS's TLS, from the core, not a guess */
    check((long)min == __pthread_stack_min());
    check(min >= STUB_OS_TLS_SIZE + __PTHREAD_STACK_WORK_MIN);

    /* A stack the core allocates: its bottom is the region's */
    create_join(NULL, arg);
    check(allocs == 1 && frees == 1);
    pthread_attr_init(&attr);
    check(pthread_attr_getstacksize(&attr, &stacksize) == 0);
    check(alloc_size >= stacksize + STUB_OS_TLS_SIZE);

    /* An application stack of the minimum size still has room for the OS's TLS */
    stack = aligned_alloc(16, min);
    check(stack != NULL);
    if (stack) {
        memset(stack, PATTERN, min);
        expected_base = stack;
        check(pthread_attr_setstack(&attr, stack, min) == 0);
        create_join(&attr, arg);
        check(allocs == 1 && frees == 1);
        free(stack);
    }
    pthread_attr_destroy(&attr);

    /* The last thread leaving reaches _pthread_os_last_exit, then _pthread_os_exit */
    pthread_exit(NULL);
}
