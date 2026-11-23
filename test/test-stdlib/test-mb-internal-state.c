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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * With a null ps, each of mbrlen, mbrtowc, mbsrtowcs and mbsnrtowcs
 * has its own internal state: a character one of them left unfinished
 * isn't continued by another. This needs a UTF-8 locale.
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
    const char *src;
    wchar_t     wc;
    wchar_t     wbuf[8];

    if (setlocale(LC_CTYPE, "C.UTF-8") == NULL) {
        printf("no C.UTF-8 locale, skipping\n");
        return 77;
    }

    /* mbrtowc and mbrlen */
    check(mbrtowc(&wc, "\xc3", 1, NULL) == (size_t)-2, "mbrtowc's internal state");
    errno = 0;
    check(mbrlen("\xa9", 1, NULL) == (size_t)-1 && errno == EILSEQ, "mbrlen's own state");
    check(mbrtowc(&wc, "\xa9", 1, NULL) == 1 && wc == 0xe9, "mbrtowc's state kept");
    check(mbrlen("\xc3", 1, NULL) == (size_t)-2, "mbrlen's internal state");
    errno = 0;
    check(mbrtowc(&wc, "\xa9", 1, NULL) == (size_t)-1 && errno == EILSEQ, "mbrtowc's own state");
    check(mbrlen("\xa9", 1, NULL) == 1, "mbrlen's state kept");

    /* mbsnrtowcs and mbsrtowcs */
    src = "\xc3";
    check(mbsnrtowcs(wbuf, &src, 1, 8, NULL) == 0, "mbsnrtowcs leaves a partial character");
    src = "\xa9";
    errno = 0;
    check(mbsrtowcs(wbuf, &src, 8, NULL) == (size_t)-1 && errno == EILSEQ, "mbsrtowcs's own state");
    src = "\xa9";
    check(mbsnrtowcs(wbuf, &src, 1, 8, NULL) == 1 && wbuf[0] == 0xe9, "mbsnrtowcs's state kept");

    printf("success\n");
    return 0;
}
