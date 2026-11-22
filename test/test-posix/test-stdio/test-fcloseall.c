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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_DEVICE_IO

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * fcloseall writes out what the open streams hold. picolibc also
 * closes them (glibc only flushes), leaving stdout open, including
 * streams already flushed by fflush(NULL)
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("a");
    (void)unlink("b");
    if (chdir("..") == 0)
        (void)rmdir(dir);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
contains(const char *name, const char *want)
{
    char    buf[32];
    ssize_t n;
    int     fd = open(name, O_RDONLY);

    check(fd >= 0, "open");
    n = read(fd, buf, sizeof(buf));
    check(n == (ssize_t)strlen(want) && memcmp(buf, want, (size_t)n) == 0, name);
    close(fd);
}

int
main(void)
{
    FILE *a, *b;
    int   fda, fdb;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    a = fopen("a", "w");
    b = fopen("b", "w");
    check(a != NULL && b != NULL, "fopen");
    fda = fileno(a);
    fdb = fileno(b);
    check(fputs("hel", a) >= 0 && fputs("wor", b) >= 0, "fputs");
    /* Streams fflush(NULL) has flushed are still open, and still closed */
    check(fflush(NULL) == 0, "fflush(NULL)");
    check(fputs("lo", a) >= 0 && fputs("ld", b) >= 0, "fputs");

    check(fcloseall() == 0, "fcloseall");
    contains("a", "hello");
    contains("b", "world");
#ifdef __PICOLIBC__
    errno = 0;
    check(fcntl(fda, F_GETFD) == -1 && errno == EBADF, "a closed");
    errno = 0;
    check(fcntl(fdb, F_GETFD) == -1 && errno == EBADF, "b closed");
#else
    (void)fda;
    (void)fdb;
#endif

    cleanup();
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
