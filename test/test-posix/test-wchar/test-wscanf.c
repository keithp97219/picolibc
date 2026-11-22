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
#include <unistd.h>
#include <wchar.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "WSCANF.TXT"
#endif

/*
 * The wide-character stream functions: vfwprintf and putwc writing a
 * file, getwc, fwscanf and vfwscanf reading it, getwchar, wscanf and
 * vwscanf reading it as stdin, and putwchar, wprintf and vwprintf
 * writing stdout
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);              \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Once stdout is wide, report with wprintf */
#define wcheck(condition, message)                       \
    do {                                                 \
        if (!(condition)) {                              \
            wprintf(L"%s: %s\n", (message), #condition); \
            (void)unlink(TEST_FILE_NAME);                \
            exit(1);                                     \
        }                                                \
    } while (0)

static int
call_vfwprintf(FILE *f, const wchar_t *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vfwprintf(f, format, ap);
    va_end(ap);
    return ret;
}

static int
call_vwprintf(const wchar_t *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vwprintf(format, ap);
    va_end(ap);
    return ret;
}

static int
call_vfwscanf(FILE *f, const wchar_t *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vfwscanf(f, format, ap);
    va_end(ap);
    return ret;
}

static int
call_vwscanf(const wchar_t *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vwscanf(format, ap);
    va_end(ap);
    return ret;
}

int
main(void)
{
    FILE   *f;
    wchar_t wc = 0;
    wchar_t word[16];
    int     a = 0;

    (void)unlink(TEST_FILE_NAME);
    f = fopen(TEST_FILE_NAME, "w");
    check(f != NULL, "fopen for writing");
    check(call_vfwprintf(f, L"%ls %d\n", L"ab", 12) == 6, "vfwprintf");
    check(putwc(L'z', f) == L'z', "putwc");
    check(call_vfwprintf(f, L" %d word\n", 34) == 9, "vfwprintf again");
    check(fwide(f, 0) > 0, "the stream is wide");
    check(fclose(f) == 0, "fclose");

    f = fopen(TEST_FILE_NAME, "r");
    check(f != NULL, "fopen for reading");
    check(getwc(f) == L'a', "getwc");
    check(fwscanf(f, L"%lc %d", &wc, &a) == 2 && wc == L'b' && a == 12, "fwscanf");
    check(call_vfwscanf(f, L" %lc %d %15ls", &wc, &a, word) == 3 && wc == L'z' && a == 34
              && wcscmp(word, L"word") == 0,
          "vfwscanf");
    check(getwc(f) == L'\n' && getwc(f) == WEOF && feof(f), "getwc at the end of the file");
    check(call_vfwscanf(f, L"%d", &a) == EOF, "vfwscanf at the end of the file");
    check(fclose(f) == 0, "fclose");

    check(freopen(TEST_FILE_NAME, "r", stdin) == stdin, "freopen stdin");
    check(getwchar() == L'a', "getwchar");
    check(wscanf(L"%lc %d", &wc, &a) == 2 && wc == L'b' && a == 12, "wscanf");
    check(call_vwscanf(L" %lc %d %15ls", &wc, &a, word) == 3 && wc == L'z' && a == 34
              && wcscmp(word, L"word") == 0,
          "vwscanf");
    check(getwchar() == L'\n' && getwchar() == WEOF, "getwchar at the end of the file");
    check(wscanf(L"%d", &a) == EOF, "wscanf at the end of the file");
    check(unlink(TEST_FILE_NAME) == 0, "unlink");

    /* stdout, which this makes wide */
    fflush(stdout);
    wcheck(putwchar(L'[') == L'[', "putwchar");
    wcheck(wprintf(L"%d", 1) == 1, "wprintf");
    wcheck(call_vwprintf(L"%ls]\n", L"x") == 3, "vwprintf");
    wcheck(fwide(stdout, 0) > 0, "stdout is wide");

    wprintf(L"success\n");
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
