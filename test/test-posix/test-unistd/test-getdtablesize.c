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

/* getdtablesize is a BSD extension, as in glibc */
#define _DEFAULT_SOURCE
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

/* getdtablesize reports the RLIMIT_NOFILE soft limit, which bounds descriptors */

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
    struct rlimit rl, low;
    int           size;

    check(getrlimit(RLIMIT_NOFILE, &rl) == 0, "getrlimit");
    size = getdtablesize();
    if (rl.rlim_cur == RLIM_INFINITY || rl.rlim_cur > INT_MAX)
        check(size == INT_MAX, "no limit");
    else
        check(size == (int)rl.rlim_cur, "the soft limit");

    /* A lower soft limit shows, and descriptors stop just below it */
    low = rl;
    low.rlim_cur = 64;
    check(setrlimit(RLIMIT_NOFILE, &low) == 0, "setrlimit");
    check(getdtablesize() == 64, "the lowered limit");
    check(dup2(1, 63) == 63, "the highest descriptor");
    check(close(63) == 0, "close");
    errno = 0;
    check(dup2(1, 64) == -1 && errno == EBADF, "past the limit");
    check(setrlimit(RLIMIT_NOFILE, &rl) == 0, "restore the limit");

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
