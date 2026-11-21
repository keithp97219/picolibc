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
/* mbstowcs and wcstombs counting without a buffer is XSI */
#define _XOPEN_SOURCE 800
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

/*
 * The <stdlib.h> multibyte functions in the POSIX locale, where
 * MB_CUR_MAX is 1 and no encoding has shift states: mblen, mbtowc,
 * wctomb, mbstowcs and wcstombs on portable characters.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

int
main(void)
{
    wchar_t wc;
    wchar_t ws[8];
    char    mb[8];

    check(MB_CUR_MAX == 1, "MB_CUR_MAX is 1");

    /* A null s asks whether the encoding has shift states: it doesn't */
    check(mblen(NULL, 0) == 0, "mblen(NULL)");
    check(mbtowc(NULL, NULL, 0) == 0, "mbtowc(NULL)");
    check(wctomb(NULL, 0) == 0, "wctomb(NULL)");

    check(mblen("a", 1) == 1, "mblen of a character");
    check(mblen("", 1) == 0, "mblen of the null character");
    check(mblen("abc", 3) == 1, "mblen looks at one character");

    wc = L'x';
    check(mbtowc(&wc, "A", 1) == 1 && wc == L'A', "mbtowc");
    check(mbtowc(&wc, "", 1) == 0 && wc == 0, "mbtowc of the null character");
    check(mbtowc(NULL, "A", 1) == 1, "mbtowc without pwc");

    check(wctomb(mb, L'Z') == 1 && mb[0] == 'Z', "wctomb");
    check(wctomb(mb, 0) == 1 && mb[0] == '\0', "wctomb of the null character");

    check(mbstowcs(ws, "hi!", 8) == 3 && ws[0] == L'h' && ws[2] == L'!' && ws[3] == 0, "mbstowcs");
    check(mbstowcs(NULL, "hello", 0) == 5, "mbstowcs counts without a buffer");
    check(wcstombs(mb, L"ok", 8) == 2 && strcmp(mb, "ok") == 0, "wcstombs");
    check(wcstombs(NULL, L"four", 0) == 4, "wcstombs counts without a buffer");

    printf("success\n");
    return 0;
}
