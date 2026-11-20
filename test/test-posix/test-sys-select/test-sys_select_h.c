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

/* What <sys/select.h> provides */

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                      \
    || ((defined(__PICOLIBC_POSIX_DEVICE_IO) && __PICOLIBC_POSIX_DEVICE_IO > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <sys/select.h>

#define same_type(a, T) _Generic((a), T: 1, default: 0)

static struct timeval  tv;
static struct timespec ts;
static sigset_t        set;
static time_t          t;
static suseconds_t     us;
_Static_assert(same_type(tv.tv_sec, time_t), "timeval tv_sec");
/* With 64-bit time on 32-bit targets, glibc's tv_usec isn't a suseconds_t */
#if !defined(__GLIBC__) || __SIZEOF_POINTER__ == 8
_Static_assert(same_type(tv.tv_usec, suseconds_t), "timeval tv_usec");
#endif
_Static_assert((suseconds_t)-1 < 0 && (suseconds_t)0.5 == 0, "suseconds_t: a signed integer type");
_Static_assert(same_type(ts.tv_nsec, long) && same_type(ts.tv_sec, time_t), "timespec");

/* FD_SETSIZE works in #if */
#if !defined(FD_SETSIZE) || FD_SETSIZE < 1
#error FD_SETSIZE
#endif

static int (* const p_pselect)(int, fd_set * restrict, fd_set * restrict, fd_set * restrict,
                               const struct timespec * restrict, const sigset_t * restrict)
    = pselect;
static int (* const p_select)(int, fd_set * restrict, fd_set * restrict, fd_set * restrict,
                              struct timeval * restrict)
    = select;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    fd_set a, b;
    int    fd, n;

    /* fd_set is a structure, so it can be assigned */
    FD_ZERO(&b);
    a = b;
    for (fd = 0; fd < FD_SETSIZE; fd++)
        check(!FD_ISSET(fd, &a), "empty");
    /* Adding twice, or removing a non-member, changes nothing */
    FD_SET(0, &a);
    FD_SET(FD_SETSIZE - 1, &a);
    FD_SET(FD_SETSIZE - 1, &a);
    FD_CLR(1, &a);
    for (fd = 0, n = 0; fd < FD_SETSIZE; fd++)
        n += FD_ISSET(fd, &a) != 0;
    check(n == 2 && FD_ISSET(0, &a) && FD_ISSET(FD_SETSIZE - 1, &a), "members");
    /* FD_ISSET takes a const set */
    {
        const fd_set *c = &a;
        check(FD_ISSET(0, c), "const");
    }
    FD_CLR(0, &a);
    check(!FD_ISSET(0, &a) && FD_ISSET(FD_SETSIZE - 1, &a), "removed");
    /* FD_ZERO empties a set that isn't empty */
    FD_ZERO(&a);
    check(!FD_ISSET(FD_SETSIZE - 1, &a), "zeroed");
    check(p_pselect && p_select, "functions");
    (void)tv;
    (void)set;
    (void)t;
    (void)us;
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
