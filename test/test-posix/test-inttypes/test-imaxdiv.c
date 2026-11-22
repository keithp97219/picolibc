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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/* imaxabs, imaxdiv, strtoimax, strtoumax, wcstoimax and wcstoumax */

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
    imaxdiv_t         d;
    char             *end;
    wchar_t          *wend;
    /* Not constants, so the compiler calls the functions */
    volatile intmax_t n = -7, k = 2;

    check(imaxabs(n) == 7 && imaxabs(INTMAX_MAX) == INTMAX_MAX
              && imaxabs(-INTMAX_MAX) == INTMAX_MAX,
          "imaxabs");
    d = imaxdiv(n, k);
    check(d.quot == -3 && d.rem == -1, "imaxdiv truncates");
    d = imaxdiv(INTMAX_MIN, -INTMAX_MAX);
    check(d.quot == 1 && d.rem == -1, "imaxdiv of INTMAX_MIN");

    errno = 0;
    check(strtoimax(" -0x1fz", &end, 16) == -0x1f && *end == 'z' && errno == 0, "strtoimax");
    check(strtoimax("077", NULL, 0) == 077, "strtoimax octal");
    errno = 0;
    check(strtoimax("-999999999999999999999999999999", &end, 10) == INTMAX_MIN && errno == ERANGE
              && *end == '\0',
          "strtoimax underflow");
    errno = 0;
    check(strtoumax("18446744073709551615", &end, 10) == (uintmax_t)18446744073709551615ULL
              && *end == '\0',
          "strtoumax");
    errno = 0;
    check(strtoumax("999999999999999999999999999999", NULL, 10) == UINTMAX_MAX && errno == ERANGE,
          "strtoumax overflow");
    check(strtoumax("xyz", &end, 10) == 0 && *end == 'x', "strtoumax without digits");

    errno = 0;
    check(wcstoimax(L" -0x1fz", &wend, 16) == -0x1f && *wend == L'z' && errno == 0, "wcstoimax");
    errno = 0;
    check(wcstoimax(L"999999999999999999999999999999", NULL, 10) == INTMAX_MAX && errno == ERANGE,
          "wcstoimax overflow");
    check(wcstoumax(L"42 ", &wend, 10) == 42 && *wend == L' ', "wcstoumax");
    errno = 0;
    check(wcstoumax(L"999999999999999999999999999999", NULL, 10) == UINTMAX_MAX && errno == ERANGE,
          "wcstoumax overflow");

    printf("success\n");
    return 0;
}
