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
#include <sys/time.h>
#include <unistd.h>

/* The types, macros and prototypes in <sys/time.h> */

#include <stdio.h>
#include <stdlib.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static fd_set         fds;
static struct timeval tv;
static time_t         tt;
static suseconds_t    us;

_Static_assert(_Generic(tv.tv_sec, time_t: 1, default: 0), "tv_sec is a time_t");
/* 32-bit glibc with a 64-bit time_t uses a wider type for tv_usec */
#ifndef __GLIBC__
_Static_assert(_Generic(tv.tv_usec, suseconds_t: 1, default: 0), "tv_usec is a suseconds_t");
#endif
_Static_assert((suseconds_t)-1 < 0, "suseconds_t is signed");
_Static_assert(FD_SETSIZE > 0, "FD_SETSIZE");

/* The functions need system support */
#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)
#define CHECK_PROTOTYPES
static int (* const p_select)(int, fd_set * restrict, fd_set * restrict, fd_set * restrict,
                              struct timeval * restrict)
    = select;
static int (* const p_utimes)(const char *, const struct timeval[2]) = utimes;
#endif

int
main(void)
{
    (void)tt;
    (void)us;

#ifdef CHECK_PROTOTYPES
    check(p_select != NULL && p_utimes != NULL, "functions");
#endif

    FD_ZERO(&fds);
    check(!FD_ISSET(0, &fds) && !FD_ISSET(FD_SETSIZE - 1, &fds), "FD_ZERO");
    FD_SET(3, &fds);
    FD_SET(FD_SETSIZE - 1, &fds);
    check(FD_ISSET(3, &fds) && FD_ISSET(FD_SETSIZE - 1, &fds) && !FD_ISSET(4, &fds), "FD_SET");
    FD_CLR(3, &fds);
    check(!FD_ISSET(3, &fds) && FD_ISSET(FD_SETSIZE - 1, &fds), "FD_CLR");

    printf("success\n");
    return 0;
}
