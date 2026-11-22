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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/* wcspbrk and vswscanf */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
call_vswscanf(const wchar_t *s, const wchar_t *format, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, format);
    ret = vswscanf(s, format, ap);
    va_end(ap);
    return ret;
}

int
main(void)
{
    static const wchar_t s[] = L"hello, world";
    wchar_t              word[8];
    int                  a = 0;

    check(wcspbrk(s, L"ow") == s + 4, "wcspbrk finds the first of the set");
    check(wcspbrk(s, L",") == s + 5, "wcspbrk punctuation");
    check(wcspbrk(s, L"xyz") == NULL, "wcspbrk without a match");
    check(wcspbrk(s, L"") == NULL, "wcspbrk with an empty set");
    check(wcspbrk(L"", L"abc") == NULL, "wcspbrk of an empty string");
    check(wcspbrk(L"\x263a smile", L"\x263a") != NULL, "wcspbrk beyond ASCII");

    check(call_vswscanf(L"12 word", L"%d %7ls", &a, word) == 2 && a == 12
              && wcscmp(word, L"word") == 0,
          "vswscanf");
    check(call_vswscanf(L"x", L"%d", &a) == 0, "vswscanf matching failure");
    check(call_vswscanf(L"", L"%d", &a) == EOF, "vswscanf of an empty string");

    printf("success\n");
    return 0;
}
