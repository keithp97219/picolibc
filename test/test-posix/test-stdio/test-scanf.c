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
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_FILE_SYSTEM

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "SCANF.TXT"
#endif

/* getchar, scanf and vscanf reading stdin, and vfscanf reading a stream */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);              \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
call_vscanf(const char *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vscanf(format, ap);
    va_end(ap);
    return ret;
}

static int
call_vfscanf(FILE *f, const char *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vfscanf(f, format, ap);
    va_end(ap);
    return ret;
}

int
main(void)
{
    FILE    *f;
    int      a = 0, b = 0;
    unsigned u = 0;
    double   d = 0;
    char     word[16];

    (void)unlink(TEST_FILE_NAME);
    f = fopen(TEST_FILE_NAME, "w");
    check(f != NULL, "fopen for writing");
    check(fputs("xy 12 34\nhello 0x1f 2.5\n-7 rest\n", f) >= 0, "fputs");
    check(fclose(f) == 0, "fclose");

    /* vfscanf: conversions, a matching failure, and the end of the file */
    f = fopen(TEST_FILE_NAME, "r");
    check(f != NULL, "fopen for reading");
    check(call_vfscanf(f, "xy %d %d", &a, &b) == 2 && a == 12 && b == 34, "vfscanf");
    check(call_vfscanf(f, "%15s %x %lf", word, &u, &d) == 3 && strcmp(word, "hello") == 0
              && u == 0x1f && d == 2.5,
          "vfscanf conversions");
    check(call_vfscanf(f, "%d %d", &a, &b) == 1 && a == -7, "vfscanf stops at a matching failure");
    check(call_vfscanf(f, "%15s", word) == 1 && strcmp(word, "rest") == 0,
          "vfscanf resumes at the failed input");
    check(call_vfscanf(f, "%d", &a) == EOF, "vfscanf at the end of the file");
    check(fclose(f) == 0, "fclose");

    /* The same input on stdin */
    check(freopen(TEST_FILE_NAME, "r", stdin) == stdin, "freopen stdin");
    check(getchar() == 'x' && getchar() == 'y', "getchar");
    check(scanf("%d %d", &a, &b) == 2 && a == 12 && b == 34, "scanf");
    check(getchar() == '\n', "getchar after scanf");
    check(call_vscanf("%15s %x", word, &u) == 2 && strcmp(word, "hello") == 0 && u == 0x1f,
          "vscanf");
    check(scanf("%lf %d", &d, &a) == 2 && d == 2.5 && a == -7, "scanf across lines");
    check(scanf("%d", &a) == 0, "scanf matching failure");
    check(scanf("%15s", word) == 1 && strcmp(word, "rest") == 0, "scanf after a matching failure");
    check(getchar() == '\n', "the last newline");
    check(getchar() == EOF && feof(stdin), "getchar at the end of the file");
    check(scanf("%d", &a) == EOF, "scanf at the end of the file");
    check(call_vscanf("%d", &a) == EOF, "vscanf at the end of the file");

    check(unlink(TEST_FILE_NAME) == 0, "unlink");
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
