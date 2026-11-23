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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __PICOLIBC__
#include <malloc.h>
#endif

/* dprintf doesn't leave a lock (or anything else) allocated per call */

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-dprintf-leak.txt"
#endif

#define NAME TEST_FILE_NAME

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(NAME);                        \
            exit(1);                                   \
        }                                              \
    } while (0)

#define CALLS 100

int
main(void)
{
    int fd;
    int i;

    fd = open(NAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "open");
    /* The first call may allocate things kept for later ones */
    check(dprintf(fd, "%d\n", 0) == 2, "dprintf");
    {
#ifdef __PICOLIBC__
        struct mallinfo before = mallinfo(), after;
#endif

        for (i = 0; i < CALLS; i++)
            check(dprintf(fd, "%d\n", i % 10) == 2, "dprintf");
#ifdef __PICOLIBC__
        after = mallinfo();
        check(after.uordblks <= before.uordblks, "dprintf doesn't leak");
#endif
    }
    check(close(fd) == 0 && unlink(NAME) == 0, "cleanup");
    printf("success\n");
    return 0;
}
