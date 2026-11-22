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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The standard streams start at their descriptor's offset, wherever
 * an earlier process (or this one) left it: after reading from stdin,
 * ftell and fflush (which moves the descriptor to the stream's
 * position) count from there. This is how "(prog; prog; cat) < file"
 * hands a file from one reader to the next.
 */

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
    char name[] = "stdin-offset-XXXXXX";
    int  fd;

    fd = mkstemp(name);
    check(fd >= 0, "mkstemp");
    check(write(fd, "0123456789\nabcdef\n", 18) == 18, "write");
    check(dup2(fd, 0) == 0, "dup2 onto stdin");
    close(fd);
    /* Leave the descriptor at 5 before stdin is used at all */
    check(lseek(0, 5, SEEK_SET) == 5, "lseek");

    check(getc(stdin) == '5', "first byte from the offset");
    check(ftell(stdin) == 6, "ftell counts from the offset");
    check(fflush(stdin) == 0, "fflush");
    check(lseek(0, 0, SEEK_CUR) == 6, "fflush leaves the descriptor at the stream position");
    check(getc(stdin) == '6', "reading resumes");

    check(unlink(name) == 0, "cleanup");
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
