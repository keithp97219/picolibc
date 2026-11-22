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

/* What <aio.h> alone provides, and the aio limits */

#define _POSIX_C_SOURCE 202405L
#include <aio.h>

/* off_t, size_t, ssize_t and struct timespec; pthread_attr_t is below */
_Static_assert(sizeof(off_t) > 0 && sizeof(size_t) > 0 && sizeof(ssize_t) > 0
                   && sizeof(struct timespec) > 0,
               "<aio.h> types");

/* struct sigevent and union sigval, as in <signal.h> */
_Static_assert(sizeof(struct sigevent) > 0 && sizeof(union sigval) > 0, "sigevent and sigval");
#define sigevent_is(member, type) _Generic(((struct sigevent *)0)->member, type: 1, default: 0)
_Static_assert(sigevent_is(sigev_notify, int) && sigevent_is(sigev_signo, int)
                   && sigevent_is(sigev_value, union sigval)
                   && sigevent_is(sigev_notify_function, void (*)(union sigval)),
               "struct sigevent members");
#define sigval_is(member, type) _Generic(((union sigval *)0)->member, type: 1, default: 0)
_Static_assert(sigval_is(sival_int, int) && sigval_is(sival_ptr, void *), "union sigval members");

/* struct aiocb has every member, with its type */
#define aiocb_is(member, type) _Generic(((struct aiocb *)0)->member, type: 1, default: 0)
_Static_assert(aiocb_is(aio_fildes, int) && aiocb_is(aio_offset, off_t)
                   && aiocb_is(aio_buf, volatile void *) && aiocb_is(aio_nbytes, size_t)
                   && aiocb_is(aio_reqprio, int) && aiocb_is(aio_sigevent, struct sigevent)
                   && aiocb_is(aio_lio_opcode, int),
               "struct aiocb members");

/* The constants exist, and the values in each group differ */
_Static_assert(AIO_ALLDONE != AIO_CANCELED && AIO_ALLDONE != AIO_NOTCANCELED
                   && AIO_CANCELED != AIO_NOTCANCELED,
               "aio_cancel results");
_Static_assert(LIO_NOP != LIO_READ && LIO_NOP != LIO_WRITE && LIO_READ != LIO_WRITE,
               "lio_listio operations");
_Static_assert(LIO_WAIT != LIO_NOWAIT, "lio_listio modes");

#ifdef __PICOLIBC__
/* picolibc's <aio.h> includes <signal.h>, which POSIX allows */
_Static_assert(SIGEV_NONE != SIGEV_SIGNAL && SIGEV_NONE != SIGEV_THREAD
                   && SIGEV_SIGNAL != SIGEV_THREAD,
               "SIGEV_ values");
#endif

#include "../test-flags.h"
#include <unistd.h>

/* pthread_attr_t, from <aio.h> and in struct sigevent, exists with threads */
#if TEST_POSIX_THREADS
_Static_assert(sizeof(pthread_attr_t) > 0, "<aio.h> pthread_attr_t");
_Static_assert(sigevent_is(sigev_notify_attributes, pthread_attr_t *), "sigev_notify_attributes");
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_ASYNCHRONOUS_IO

/* Each function has the specified prototype */
static int (* const p_aio_cancel)(int, struct aiocb *) = aio_cancel;
static int (* const p_aio_error)(const struct aiocb *) = aio_error;
#if TEST_POSIX_FSYNC || TEST_POSIX_SYNCHRONIZED_IO
static int (* const p_aio_fsync)(int, struct aiocb *) = aio_fsync;
#endif
static int     (    *const p_aio_read)(struct aiocb *) = aio_read;
static ssize_t (* const p_aio_return)(struct aiocb *) = aio_return;
static int     (    *const p_aio_suspend)(const struct aiocb     *const[], int, const struct timespec *)
    = aio_suspend;
static int (* const p_aio_write)(struct aiocb *) = aio_write;
#ifndef __GLIBC__
static int (* const p_lio_listio)(int, struct aiocb * restrict const[restrict], int,
                                  struct sigevent * restrict)
    = lio_listio;
#else
/* glibc's list elements aren't restrict */
static int (* const p_lio_listio)(int, struct aiocb * const[restrict], int,
                                  struct sigevent * restrict)
    = lio_listio;
#endif

/*
 * A limit <limits.h> defines is the one sysconf reports; otherwise
 * sysconf gives at least the POSIX minimum, or -1 for no limit
 */
static void
check_limit(const char *name, int sc, long minimum, int defined, long value)
{
    long limit;

    errno = 0;
    limit = sysconf(sc);
    if (defined) {
        if (value < minimum || limit != value) {
            printf("%s is %ld, sysconf says %ld, minimum %ld\n", name, value, limit, minimum);
            exit(1);
        }
    } else if (limit != -1 && limit < minimum) {
        printf("sysconf says %s is %ld, minimum %ld\n", name, limit, minimum);
        exit(1);
    } else if (limit == -1 && errno != 0) {
        printf("sysconf(%s) failed\n", name);
        exit(1);
    }
}

#ifdef AIO_LISTIO_MAX
#define LISTIO_DEFINED 1
#define LISTIO_VALUE   AIO_LISTIO_MAX
#else
#define LISTIO_DEFINED 0
#define LISTIO_VALUE   0
#endif
#ifdef AIO_MAX
#define MAX_DEFINED 1
#define MAX_VALUE   AIO_MAX
#else
#define MAX_DEFINED 0
#define MAX_VALUE   0
#endif
#ifdef AIO_PRIO_DELTA_MAX
#define PRIO_DEFINED 1
#define PRIO_VALUE   AIO_PRIO_DELTA_MAX
#else
#define PRIO_DEFINED 0
#define PRIO_VALUE   0
#endif

int
main(void)
{
    (void)p_aio_cancel;
    (void)p_aio_error;
#if TEST_POSIX_FSYNC || TEST_POSIX_SYNCHRONIZED_IO
    (void)p_aio_fsync;
#endif
    (void)p_aio_read;
    (void)p_aio_return;
    (void)p_aio_suspend;
    (void)p_aio_write;
    (void)p_lio_listio;

    check(sysconf(_SC_ASYNCHRONOUS_IO) > 0, "_SC_ASYNCHRONOUS_IO");
    check(_POSIX_AIO_LISTIO_MAX == 2 && _POSIX_AIO_MAX == 1, "POSIX minimums");
    check_limit("AIO_LISTIO_MAX", _SC_AIO_LISTIO_MAX, _POSIX_AIO_LISTIO_MAX, LISTIO_DEFINED,
                LISTIO_VALUE);
    check_limit("AIO_MAX", _SC_AIO_MAX, _POSIX_AIO_MAX, MAX_DEFINED, MAX_VALUE);
    /* The priority offset can be 0, so -1 isn't a limit here */
    check_limit("AIO_PRIO_DELTA_MAX", _SC_AIO_PRIO_DELTA_MAX, 0, PRIO_DEFINED, PRIO_VALUE);
    check(sysconf(_SC_AIO_PRIO_DELTA_MAX) >= 0, "AIO_PRIO_DELTA_MAX");
    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
