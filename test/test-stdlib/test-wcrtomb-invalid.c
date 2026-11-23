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
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * wcrtomb and wctomb reject wide characters that are no character,
 * including negative wchar_t values (wchar_t is signed on most
 * targets): (wchar_t)0xFFFFFFFF came out as one byte in UTF-8 (gawk's
 * cmdlinefsbacknl2)
 */

static int ret;

static void
check_bad(const char *where, wchar_t wc)
{
    char      buf[MB_LEN_MAX];
    mbstate_t state;
    size_t    n;

    memset(&state, 0, sizeof(state));
    errno = 0;
    n = wcrtomb(buf, wc, &state);
    if (n != (size_t)-1 || errno != EILSEQ) {
        printf("%s: wcrtomb(%#lx) = %zd, errno %d\n", where, (unsigned long)(unsigned)wc, n, errno);
        ret = 1;
    }
    if (wctomb(buf, wc) != -1) {
        printf("%s: wctomb(%#lx) accepted\n", where, (unsigned long)(unsigned)wc);
        ret = 1;
    }
}

int
main(void)
{
    check_bad("C", (wchar_t)-1);
    check_bad("C", (wchar_t)-200);
    if (setlocale(LC_CTYPE, "C.UTF-8") != NULL) {
        check_bad("C.UTF-8", (wchar_t)-1);
        check_bad("C.UTF-8", (wchar_t)-0x7f);
        check_bad("C.UTF-8", (wchar_t)0xd800);
    }
    if (ret == 0)
        printf("success\n");
    return ret;
}
