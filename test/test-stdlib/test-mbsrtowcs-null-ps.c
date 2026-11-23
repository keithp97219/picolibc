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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * mbsrtowcs, mbsnrtowcs, wcsrtombs and wcsnrtombs with a null ps use
 * an internal state, in every configuration, including when they
 * fail with EILSEQ.
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
    static const char    abc[] = "abc";
    static const wchar_t wabc[] = L"abc";
    const char          *src;
    const wchar_t       *wsrc;
    wchar_t              wbuf[8];
    char                 buf[8];

    src = abc;
    check(mbsrtowcs(wbuf, &src, 8, NULL) == 3, "mbsrtowcs with a null ps");
    src = abc;
    check(mbsnrtowcs(wbuf, &src, 8, 8, NULL) == 3, "mbsnrtowcs with a null ps");
    wsrc = wabc;
    check(wcsrtombs(buf, &wsrc, 8, NULL) == 3, "wcsrtombs with a null ps");
    wsrc = wabc;
    check(wcsnrtombs(buf, &wsrc, 8, 8, NULL) == 3, "wcsnrtombs with a null ps");

#if WCHAR_MAX > 0x10000
    {
        /* A wide character the C locale lacks is an encoding error */
        static const wchar_t bad[] = { L'a', (wchar_t)0x10000, L'b', L'\0' };

        wsrc = bad;
        errno = 0;
        check(wcsrtombs(buf, &wsrc, 8, NULL) == (size_t)-1 && errno == EILSEQ,
              "wcsrtombs EILSEQ with a null ps");
        wsrc = bad;
        errno = 0;
        check(wcsnrtombs(NULL, &wsrc, 8, 0, NULL) == (size_t)-1 && errno == EILSEQ,
              "wcsnrtombs EILSEQ with a null dst and ps");
    }
#endif

    printf("success\n");
    return 0;
}
