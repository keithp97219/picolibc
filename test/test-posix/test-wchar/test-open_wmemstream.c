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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifdef __PICOLIBC__
#include <malloc.h>

/* Let the test make the stream's buffer unable to grow */
static int fail_realloc;

void *
realloc(void *ptr, size_t size)
{
    void  *new;
    size_t old;

    if (fail_realloc) {
        errno = ENOMEM;
        return NULL;
    }
    new = malloc(size);
    if (new != NULL && ptr != NULL) {
        old = malloc_usable_size(ptr);
        memcpy(new, ptr, old < size ? old : size);
        free(ptr);
    }
    return new;
}
#endif

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
    /*
     * A wide character that doesn't fit when the buffer can't grow is
     * not written at all, so those written later stay whole
     */
    f = open_wmemstream(&buf, &size);
    check(f != NULL, "open_wmemstream for no memory");
    fail_realloc = 1;
    for (i = 0; i < 100000; i++)
        if (fputwc(L'a' + i % 26, f) == WEOF)
            break;
    fail_realloc = 0;
    check(i < 100000, "out of memory");
    clearerr(f);
    check(fputwc(L'Z', f) == L'Z', "fputwc after no memory");
    check(fflush(f) == 0, "fflush after no memory");
    check(size == (size_t)i + 1, "size after no memory");
    check(buf[i] == L'Z' && buf[i + 1] == L'\0', "wide character after no memory");
    check(buf[i - 1] == (wchar_t)(L'a' + (i - 1) % 26), "wide character before no memory");
    check(fclose(f) == 0, "fclose after no memory");
    free(buf);

    /* POSIX allows rejecting null pointers; picolibc does */
    check(open_wmemstream(NULL, &size) == NULL, "no buffer pointer");
    check(open_wmemstream(&buf, NULL) == NULL, "no size pointer");
#endif

    printf("success\n");
    return 0;
}
