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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                        \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0 \
        && defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* fopen takes the e and x mode flags */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NAME "test-fopen.txt"

int
main(void)
{
    FILE *f;

    (void)unlink(NAME);
    f = fopen(NAME, "wxe");
    check(f != NULL, "wx creates");
    check(fcntl(fileno(f), F_GETFD) & FD_CLOEXEC, "e sets close on exec");
    check(fputs("data", f) >= 0 && fclose(f) == 0, "write");
    check(fopen(NAME, "wx") == NULL && errno == EEXIST, "wx fails when the file exists");
    check(fopen(NAME, "w+x") == NULL && errno == EEXIST, "w+x");
    f = fopen(NAME, "r");
    check(f != NULL && !(fcntl(fileno(f), F_GETFD) & FD_CLOEXEC), "no e");
    check(fclose(f) == 0, "close");
    f = fopen(NAME, "re");
    check(f != NULL && (fcntl(fileno(f), F_GETFD) & FD_CLOEXEC), "re");
    check(fclose(f) == 0 && unlink(NAME) == 0, "cleanup");
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
