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
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

/*
 * The ISO C wide functions with POSIX's added requirements: errno
 * from the wcsto* conversions (EINVAL for an unsupported base, ERANGE
 * for values out of range), wcstok's state, wcsftime's overflow, and
 * the basic wcs and wmem functions leaving errno alone.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SENTINEL EDOM

static void
integers(void)
{
    const wchar_t *str = L"  -123xyz";
    wchar_t       *end;

    errno = SENTINEL;
    check(wcstol(str, &end, 10) == -123 && end == str + 6, "wcstol");
    check(wcstoul(L"0x1f", &end, 0) == 0x1f && *end == L'\0', "wcstoul base 0");
    check(wcstoll(L"777", &end, 8) == 0777, "wcstoll base 8");
    check(wcstoull(L"zz", &end, 36) == 35 * 36 + 35, "wcstoull base 36");
    check(errno == SENTINEL, "errno after the conversions");

    /* No conversion: 0, and end is the start */
    check(wcstol(L"  xyz", &end, 10) == 0 && wcscmp(end, L"  xyz") == 0, "no conversion");

    /* The value of base is not supported */
    errno = 0;
    check(wcstol(L"12", &end, 1) == 0 && errno == EINVAL, "wcstol base 1");
    errno = 0;
    check(wcstoul(L"12", &end, 37) == 0 && errno == EINVAL, "wcstoul base 37");
    errno = 0;
    check(wcstoll(L"12", &end, -1) == 0 && errno == EINVAL, "wcstoll base -1");

    /* Out of range: the limit with the value's sign, and ERANGE */
    errno = 0;
    check(wcstol(L"999999999999999999999999", &end, 10) == LONG_MAX && errno == ERANGE,
          "wcstol overflow");
    check(*end == L'\0', "wcstol consumes the digits");
    errno = 0;
    check(wcstol(L"-999999999999999999999999", &end, 10) == LONG_MIN && errno == ERANGE,
          "wcstol underflow");
    errno = 0;
    check(wcstoll(L"-999999999999999999999999", &end, 10) == LLONG_MIN && errno == ERANGE,
          "wcstoll underflow");
    errno = 0;
    check(wcstoul(L"999999999999999999999999", &end, 10) == ULONG_MAX && errno == ERANGE,
          "wcstoul overflow");
    errno = 0;
    check(wcstoull(L"999999999999999999999999", &end, 10) == ULLONG_MAX && errno == ERANGE,
          "wcstoull overflow");
}

static void
floats(void)
{
    wchar_t *end;

    errno = SENTINEL;
    check(wcstod(L" 1.5e2x", &end) == 150.0 && *end == L'x', "wcstod");
    check(wcstof(L"0.25", &end) == 0.25f, "wcstof");
    check(errno == SENTINEL, "errno after wcstod");

    errno = 0;
    check(wcstod(L"1e999999", &end) == HUGE_VAL && errno == ERANGE, "wcstod overflow");
    errno = 0;
    check(wcstod(L"-1e999999", &end) == -HUGE_VAL && errno == ERANGE, "wcstod negative overflow");
    errno = 0;
    check(wcstof(L"1e999", &end) == HUGE_VALF && errno == ERANGE, "wcstof overflow");
    errno = 0;
    check(fabs(wcstod(L"1e-999999", &end)) <= DBL_MIN && errno == ERANGE, "wcstod underflow");
}

static void
strings(void)
{
    wchar_t   buf[32] = L"a,b;;c";
    wchar_t  *state = NULL, *tok;
    wchar_t   out[32];
    struct tm tm = { .tm_year = 124, .tm_mon = 0, .tm_mday = 2, .tm_hour = 3 };

    errno = SENTINEL;

    /* wcstok keeps its place in the third argument */
    tok = wcstok(buf, L",;", &state);
    check(tok != NULL && wcscmp(tok, L"a") == 0, "wcstok first");
    tok = wcstok(NULL, L",;", &state);
    check(tok != NULL && wcscmp(tok, L"b") == 0, "wcstok second");
    tok = wcstok(NULL, L",;", &state);
    check(tok != NULL && wcscmp(tok, L"c") == 0, "wcstok skips empty fields");
    check(wcstok(NULL, L",;", &state) == NULL, "wcstok at the end");
    check(wcstok(NULL, L",;", &state) == NULL, "wcstok stays at the end");

    /* wcsftime: 0 when the result and its null don't fit */
    check(wcsftime(out, 32, L"%Y-%m-%d", &tm) == 10 && wcscmp(out, L"2024-01-02") == 0, "wcsftime");
    check(wcsftime(out, 10, L"%Y-%m-%d", &tm) == 0, "wcsftime too long");
    check(wcsftime(out, 11, L"%Y-%m-%d", &tm) == 10, "wcsftime just fits");

    /* The rest of the family, briefly; these leave errno alone */
    errno = SENTINEL;
    wcscpy(out, L"ab");
    check(wcscat(out, L"cd") == out && wcscmp(out, L"abcd") == 0, "wcscat");
    check(wcsncat(out, L"efg", 2) == out && wcscmp(out, L"abcdef") == 0, "wcsncat");
    check(wcsncmp(L"abc", L"abd", 2) == 0 && wcsncmp(L"abc", L"abd", 3) < 0, "wcsncmp");
    check(wcsncpy(out, L"xy", 4) == out && out[2] == 0 && out[3] == 0, "wcsncpy pads");
    check(wcsspn(L"aabc", L"a") == 2 && wcscspn(L"aabc", L"c") == 3, "wcsspn, wcscspn");
    check(wcschr(L"abc", L'\0') != NULL, "wcschr finds the terminator");
    check(wcsrchr(L"abca", L'a') != NULL && *(wcsrchr(L"abca", L'a') + 1) == L'\0', "wcsrchr");
    check(wcsstr(L"abc", L"") != NULL, "wcsstr of an empty string");
    check(wmemcmp(L"ab", L"ac", 2) < 0 && wmemcmp(L"ab", L"ac", 1) == 0, "wmemcmp");
    wmemcpy(out, L"abcdef", 7);
    check(wmemmove(out + 1, out, 3) == out + 1 && wcscmp(out, L"aabcef") == 0, "wmemmove");
    check(wmemset(out, L'z', 2) == out && out[0] == L'z' && out[1] == L'z', "wmemset");
    check(wmemchr(L"abc", L'c', 2) == NULL, "wmemchr stops at n");
    check(errno == SENTINEL, "errno after the string functions");
}

int
main(void)
{
    integers();
    floats();
    strings();
    printf("success\n");
    return 0;
}
