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
 * Application macros with plain names, defined before the header, must
 * not break it: POSIX reserves only sem_ names for <semaphore.h>.
 * ("value" would break <time.h>'s timer_gettime prototype, which
 * <semaphore.h> may include.)
 */
#define waiters         +
#define pshared         +
#define count           +
#define lock            +

#define _POSIX_C_SOURCE 202405L
/* glibc declares sem_clockwait only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include <semaphore.h>
#include <unistd.h>

#undef waiters
#undef pshared
#undef count
#undef lock

/* Without the system support this test needs, it is skipped */
#if _POSIX_SEMAPHORES > 0

/* The <semaphore.h> header: types, constants and prototypes */

#define same_type(a, T) _Generic((a), T: 1, default: 0)

static sem_t           sem;
static struct timespec ts;
_Static_assert(same_type(SEM_FAILED, sem_t *), "SEM_FAILED is a sem_t *");
_Static_assert(same_type(ts.tv_sec, time_t) && same_type(ts.tv_nsec, long), "timespec");

/* glibc 2.43 leaves O_CREAT and O_EXCL to <fcntl.h> */
#ifndef __GLIBC__
_Static_assert(O_CREAT != O_EXCL && O_CREAT && O_EXCL, "O_CREAT and O_EXCL");
#endif

static int (* const p_sem_clockwait)(sem_t * restrict, clockid_t, const struct timespec * restrict)
    = sem_clockwait;
static int    (   *const p_sem_close)(sem_t *) = sem_close;
static int    (   *const p_sem_destroy)(sem_t *) = sem_destroy;
static int    (   *const p_sem_getvalue)(sem_t    *restrict, int    *restrict) = sem_getvalue;
static int    (   *const p_sem_init)(sem_t *, int, unsigned) = sem_init;
static sem_t *(* const p_sem_open)(const char *, int, ...) = sem_open;
static int    (   *const p_sem_post)(sem_t *) = sem_post;
static int    (   *const p_sem_timedwait)(sem_t    *restrict, const struct timespec    *restrict)
    = sem_timedwait;
static int (* const p_sem_trywait)(sem_t *) = sem_trywait;
static int (* const p_sem_unlink)(const char *) = sem_unlink;
static int (* const p_sem_wait)(sem_t *) = sem_wait;

#include <stdio.h>

int
main(void)
{
    int v;

    if (!p_sem_clockwait || !p_sem_close || !p_sem_destroy || !p_sem_getvalue || !p_sem_init
        || !p_sem_open || !p_sem_post || !p_sem_timedwait || !p_sem_trywait || !p_sem_unlink
        || !p_sem_wait) {
        printf("functions\n");
        return 1;
    }
    /* An unnamed semaphore works through the pointers too */
    if (p_sem_init(&sem, 0, 1) != 0 || p_sem_getvalue(&sem, &v) != 0 || v != 1
        || p_sem_trywait(&sem) != 0 || p_sem_post(&sem) != 0 || p_sem_destroy(&sem) != 0) {
        printf("semaphore\n");
        return 1;
    }
    (void)ts;
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
