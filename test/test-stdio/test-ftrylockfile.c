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
#include <stdio.h>
#include <stdlib.h>

/* ftrylockfile fails, instead of blocking, while another thread holds the lock */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int start_thread(void *(*func)(void *), void *arg);

int stop_thread(void);

#ifdef NO_NEWLIB
#include <pthread.h>

static pthread_t thread;

int
start_thread(void *(*func)(void *), void *arg)
{
    return pthread_create(&thread, NULL, func, arg);
}

int
stop_thread(void)
{
    return pthread_join(thread, NULL);
}
#endif

/*
 * A static stream, so the test doesn't call picolibc's malloc, which
 * the host's pthread_create would then use as well
 */
#ifdef __PICOLIBC__
static int
myput(char c, FILE *file)
{
    (void)file;
    return (unsigned char)c;
}

static FILE myfile = FDEV_SETUP_STREAM(myput, NULL, NULL, _FDEV_SETUP_WRITE);
#define TEST_FILE (&myfile)
#else
#define TEST_FILE stdout
#endif

static FILE *f;
static int   try_result;

static void *
try_func(void *arg)
{
    (void)arg;
    try_result = ftrylockfile(f);
    if (try_result == 0)
        funlockfile(f);
    return NULL;
}

int
main(void)
{
#ifdef __SINGLE_THREAD
    printf("Single thread mode, test skipped\n");
    return 77;
#endif

    f = TEST_FILE;

    /* Held by this thread: another thread's ftrylockfile fails */
    flockfile(f);
    check(start_thread(try_func, NULL) == 0, "start thread");
    check(stop_thread() == 0, "stop thread");
    funlockfile(f);
    check(try_result != 0, "ftrylockfile fails while another thread holds the lock");

    /* Not held: ftrylockfile succeeds */
    check(start_thread(try_func, NULL) == 0, "start thread");
    check(stop_thread() == 0, "stop thread");
    check(try_result == 0, "ftrylockfile takes a free lock");

    printf("success\n");
    return 0;
}
