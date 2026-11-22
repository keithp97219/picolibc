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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_PIPE && TEST_POSIX_SIGNALS

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * getline and getdelim: EINVAL for null pointers, the bytes stored, and -1
 * with errno when a read fails part way
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
on_alarm(int sig)
{
    (void)sig;
}

int
main(void)
{
    char            *line = NULL;
    size_t           n = 0;
    int              p[2];
    FILE            *f;
    struct sigaction sa;
    static char      data[] = "one\ntwo\0three:four";

    /* EINVAL for null pointers */
    f = fmemopen(data, sizeof(data) - 1, "r");
    check(f != NULL, "fmemopen");
    errno = 0;
    check(getline(NULL, &n, f) == -1 && errno == EINVAL, "getline null lineptr");
#ifndef __GLIBC__
    /* glibc doesn't check every pointer */
    errno = 0;
    check(getline(&line, NULL, f) == -1 && errno == EINVAL, "getline null n");
    errno = 0;
    check(getdelim(NULL, &n, ':', f) == -1 && errno == EINVAL, "getdelim null lineptr");
    errno = 0;
    check(getdelim(&line, NULL, ':', f) == -1 && errno == EINVAL, "getdelim null n");
#endif
    check(fclose(f) == 0, "fclose");

    /* Lines, with the delimiter, the null bytes counted and *n large enough */
    f = fmemopen(data, sizeof(data) - 1, "r");
    check(f != NULL, "fmemopen");
    check(getline(&line, &n, f) == 4 && strcmp(line, "one\n") == 0 && n >= 5, "first line");
    check(getdelim(&line, &n, ':', f) == 10 && memcmp(line, "two\0three:", 11) == 0,
          "getdelim with a null in the data");
    check(getline(&line, &n, f) == 4 && strcmp(line, "four") == 0, "last line without delimiter");
    check(getline(&line, &n, f) == -1 && feof(f), "end of file");
    check(fclose(f) == 0, "fclose");

    /* A read error part way through a line: -1, errno and the error indicator */
    check(pipe(p) == 0, "pipe");
    f = fdopen(p[0], "r");
    check(f != NULL, "fdopen");
    check(write(p[1], "ab", 2) == 2, "write part of a line");
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART: the blocked read fails with EINTR */
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    alarm(1);
    errno = 0;
    {
        ssize_t len = getline(&line, &n, f);
        int     err = errno;

#ifdef __GLIBC__
        /* glibc returns the bytes read before the error */
        check(len == -1 || len == 2, "getline after a read error (glibc)");
#else
        check(len == -1, "getline returns -1 after a read error");
#endif
        check(err == EINTR && ferror(f), "errno and the error indicator");
    }
    check(fclose(f) == 0 && close(p[1]) == 0, "close");

    free(line);
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
