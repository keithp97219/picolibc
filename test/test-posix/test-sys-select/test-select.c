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

#define _GNU_SOURCE
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                      \
    || ((defined(__PICOLIBC_POSIX_DEVICE_IO) && __PICOLIBC_POSIX_DEVICE_IO > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0))

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

/* select times out, reports ready descriptors and rejects a bad count */

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
    int            fds[2];
    fd_set         rset, wset;
    struct timeval tv;

    check(pipe(fds) == 0, "pipe");

    /* Nothing to read: select times out and clears the set */
    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    tv.tv_sec = 0;
    tv.tv_usec = 20000;
    check(select(fds[0] + 1, &rset, NULL, NULL, &tv) == 0, "select times out");
    check(!FD_ISSET(fds[0], &rset), "select clears the set");

    /* The write end is ready for writing */
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(fds[0], &rset);
    FD_SET(fds[1], &wset);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    check(select(fds[1] + 1, &rset, &wset, NULL, &tv) == 1, "select write ready");
    check(!FD_ISSET(fds[0], &rset) && FD_ISSET(fds[1], &wset), "select ready set");

    check(write(fds[1], "x", 1) == 1, "write");

    FD_ZERO(&rset);
    FD_SET(fds[0], &rset);
    check(select(fds[0] + 1, &rset, NULL, NULL, NULL) == 1, "select read ready");
    check(FD_ISSET(fds[0], &rset), "select read set");

    errno = 0;
    check(select(-1, NULL, NULL, NULL, NULL) == -1 && errno == EINVAL, "select bad nfds");

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
