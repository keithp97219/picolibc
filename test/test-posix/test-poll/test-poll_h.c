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

/* What <poll.h> provides */

#define _POSIX_C_SOURCE 202405L
/* glibc declares ppoll only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_PIPE

#include <poll.h>
/* glibc's <poll.h> doesn't define sigset_t */
#ifdef __GLIBC__
#include <signal.h>
#endif

#define same_type(a, T) _Generic((a), T: 1, default: 0)

static struct pollfd   pfd;
static nfds_t          nfds __attribute__((unused));
static sigset_t        set;
static struct timespec ts __attribute__((unused));
_Static_assert(same_type(pfd.fd, int) && same_type(pfd.events, short)
                   && same_type(pfd.revents, short),
               "pollfd");
_Static_assert((nfds_t)-1 > 0 && (nfds_t)0.5 == 0 && sizeof(nfds) <= sizeof(long),
               "nfds_t: an unsigned integer type no wider than long");
_Static_assert(sizeof(ts.tv_sec) > 0 && same_type(ts.tv_nsec, long), "timespec");

/* The events can be OR'ed together: non-zero and bitwise distinct */
#define EVENTS(X)                                                                       \
    X(POLLIN)                                                                           \
    X(POLLRDNORM)                                                                       \
    X(POLLRDBAND) X(POLLPRI) X(POLLOUT) X(POLLWRBAND) X(POLLERR) X(POLLHUP) X(POLLNVAL)
#define OR(e)  | (e)
#define SUM(e) +(e)
_Static_assert((0 EVENTS(OR)) == (0 EVENTS(SUM)), "events bitwise distinct");
#define NONZERO(e) &&(e) != 0
_Static_assert(1 EVENTS(NONZERO) && POLLWRNORM != 0, "events non-zero");
/* POLLWRNORM is equivalent to POLLOUT, so it may share its bit */
_Static_assert((POLLWRNORM
                & (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLWRBAND | POLLERR | POLLHUP
                   | POLLNVAL))
                   == 0,
               "POLLWRNORM");
/* They fit in short */
_Static_assert((short)(0 EVENTS(OR) | POLLWRNORM) == (0 EVENTS(OR) | POLLWRNORM), "short");

static int (* const p_poll)(struct pollfd[], nfds_t, int) = poll;
static int (* const p_ppoll)(struct pollfd[], nfds_t, const struct timespec * restrict,
                             const sigset_t * restrict)
    = ppoll;

#include <stdio.h>

int
main(void)
{
    if (!p_poll || !p_ppoll) {
        printf("functions\n");
        return 1;
    }
    (void)pfd;
    (void)set;
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
