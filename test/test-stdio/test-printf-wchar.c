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

/*
 * Check %lc and %ls in the C locale. Where printf supports them (the
 * 'd' and 'f' levels), ASCII values convert, padding and precision
 * count bytes, and anything else fails with EILSEQ rather than being
 * misprinted. The 'l', 'i' and 'm' levels don't support them, but
 * still consume their arguments, so the rest of the output is right.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

/* The 'd' and 'f' levels always support them */
#if !defined(__PICOLIBC__) || _PICOLIBC_PRINTF == __IO_VARIANT_DOUBLE \
    || _PICOLIBC_PRINTF == __IO_VARIANT_FLOAT
#define HAVE_WCHAR 1
#else
#define HAVE_WCHAR 0
#endif

static int errors;

#if HAVE_WCHAR
static void
check(int line, const char *expect, int ret, const char *buf)
{
    if (ret != (int)strlen(expect) || strcmp(buf, expect) != 0) {
        printf("%d: expected \"%s\" (%d) got \"%s\" (%d)\n", line, expect, (int)strlen(expect), buf,
               ret);
        errors++;
    }
}

static void
check_fail(int line, int ret)
{
    if (ret >= 0 || errno != EILSEQ) {
        printf("%d: expected -1/EILSEQ got %d/%d\n", line, ret, errno);
        errors++;
    }
}
#else
/* An unsupported conversion still consumes its argument: buf ends with tail */
static void
check_tail(int line, const char *tail, int ret, const char *buf)
{
    size_t len = strlen(buf), tlen = strlen(tail);

    if (ret < 0 || (size_t)ret != len || len < tlen || strcmp(buf + len - tlen, tail) != 0) {
        printf("%d: expected ...\"%s\" got \"%s\" (%d)\n", line, tail, buf, ret);
        errors++;
    }
}
#endif

int
main(void)
{
    char buf[64];
    int  ret;

#define T(expect, ...)                                 \
    do {                                               \
        memset(buf, 'x', sizeof(buf));                 \
        ret = snprintf(buf, sizeof(buf), __VA_ARGS__); \
        check(__LINE__, expect, ret, buf);             \
    } while (0)

#define F(...)                                         \
    do {                                               \
        errno = 0;                                     \
        ret = snprintf(buf, sizeof(buf), __VA_ARGS__); \
        check_fail(__LINE__, ret);                     \
    } while (0)

#if HAVE_WCHAR
    T("wide", "%ls", L"wide");
    T("[  wide]", "[%6ls]", L"wide");
    T("[wide  ]", "[%-6ls]", L"wide");
    T("[wi]", "[%.2ls]", L"wide");
    T("[   wi]", "[%5.2ls]", L"wide");
    T("[]", "[%ls]", L"");
    T("a wide b", "%c %ls %c", 'a', L"wide", 'b');
    T("[c]", "[%lc]", (wint_t)L'c');
    T("[  c]", "[%3lc]", (wint_t)L'c');
    T("[c  ]", "[%-3lc]", (wint_t)L'c');
    T("c wide 7", "%lc %ls %d", (wint_t)L'c', L"wide", 7);

    /* Only ASCII is representable in the C locale */
    F("%ls", L"café");
    F("%lc", (wint_t)0xe9);
    F("%lc", (wint_t)0x20ac);
    /* precision stops before the bad character */
    T("caf", "%.3ls", L"café");
#else
#define U(tail, ...)                                   \
    do {                                               \
        memset(buf, 'x', sizeof(buf));                 \
        ret = snprintf(buf, sizeof(buf), __VA_ARGS__); \
        check_tail(__LINE__, tail, ret, buf);          \
    } while (0)

    U("|7|8", "%ls|%d|%d", L"wide", 7, 8);
    U("|7|8", "%lc|%d|%d", (wint_t)L'c', 7, 8);
    U("|7|wide|8", "%lc|%d|%s|%d", (wint_t)0x20ac, 7, "wide", 8);
    (void)ret;
#endif

    return errors ? 1 : 0;
}
