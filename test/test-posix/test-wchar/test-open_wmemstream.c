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
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

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
    wchar_t *buf = NULL;
    size_t   size = 1;
    FILE    *f;
    int      i;

    f = open_wmemstream(&buf, &size);
    check(f != NULL, "open_wmemstream");

    /* The size counts wide characters, up to the terminating null */
    check(fputwc(L'a', f) == L'a', "fputwc");
    check(fwprintf(f, L"%ls %d", L"bc", 42) == 5, "fwprintf");
    check(fflush(f) == 0, "fflush");
    check(size == 6, "size after fflush");
    check(wcscmp(buf, L"abc 42") == 0, "contents after fflush");

    /* Grow past the initial allocation */
    for (i = 0; i < 1000; i++)
        check(fputwc(L'x', f) == L'x', "fputwc many");
    check(fclose(f) == 0, "fclose");
    check(size == 1006, "size after fclose");
    check(buf[1005] == L'x' && buf[1006] == L'\0', "terminated after fclose");
    free(buf);

#ifdef __PICOLIBC__
    /* POSIX allows rejecting null pointers; picolibc does */
    check(open_wmemstream(NULL, &size) == NULL, "no buffer pointer");
    check(open_wmemstream(&buf, NULL) == NULL, "no size pointer");
#endif

    printf("success\n");
    return 0;
}
