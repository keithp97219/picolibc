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

/* The legacy pthread_attr_getstackaddr and pthread_attr_setstackaddr */

#define _DEFAULT_SOURCE
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Allow testing functions that are deprecated (for good reason) */
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 2) || __GNUC__ > 4)
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Where the thread's stack was, from the address of a local */
static volatile uintptr_t seen;

static void *
where(void *arg)
{
    volatile char local;

    (void)arg;
    seen = (uintptr_t)&local;
    return NULL;
}

/* Run a thread on [base, base + size) using the legacy interface */
static void
run(char *base, size_t size, int address_first)
{
    pthread_attr_t attr;
    pthread_t      thread;
    void          *addr;
    size_t         got;

    check(pthread_attr_init(&attr) == 0, "pthread_attr_init");
    if (address_first) {
        check(pthread_attr_setstackaddr(&attr, base + size) == 0, "setstackaddr");
        check(pthread_attr_setstacksize(&attr, size) == 0, "setstacksize");
    } else {
        check(pthread_attr_setstacksize(&attr, size) == 0, "setstacksize");
        check(pthread_attr_setstackaddr(&attr, base + size) == 0, "setstackaddr");
    }
    check(pthread_attr_getstackaddr(&attr, &addr) == 0 && addr == base + size,
          "getstackaddr returns the top");
    check(pthread_attr_getstack(&attr, &addr, &got) == 0 && addr == base && got == size,
          "getstack returns the base");
    check(pthread_create(&thread, &attr, where, NULL) == 0, "pthread_create");
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(seen >= (uintptr_t)base && seen < (uintptr_t)(base + size), "thread ran on that stack");
    check(pthread_attr_destroy(&attr) == 0, "pthread_attr_destroy");
}

int
main(void)
{
    size_t size = PTHREAD_STACK_MIN + 65536;
    char  *base = aligned_alloc(4096, size);

    check(base != NULL, "allocate a stack");
    run(base, size, 0);
    run(base, size, 1);
    free(base);
    printf("success\n");
    return 0;
}
