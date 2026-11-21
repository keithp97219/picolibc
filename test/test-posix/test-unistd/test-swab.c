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
/* swab is an XSI interface */
#define _XOPEN_SOURCE 800
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * swab copies nbytes bytes exchanging adjacent ones; for an odd count
 * it exchanges nbytes - 1 of them (the last byte is unspecified), and
 * for a negative count it does nothing. Nothing past nbytes changes,
 * and the source is left alone.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define SIZE 32

int
main(void)
{
    char    src[SIZE], copy[SIZE], dst[SIZE];
    ssize_t n, i;

    for (i = 0; i < SIZE; i++)
        src[i] = (char)('a' + i);
    memcpy(copy, src, SIZE);

    for (n = 0; n <= SIZE; n++) {
        memset(dst, '#', SIZE);
        swab(src, dst, n);
        check(memcmp(src, copy, SIZE) == 0, "source unchanged");
        for (i = 0; i + 1 < n; i += 2) {
            check(dst[i] == src[i + 1], "even byte from the odd one");
            check(dst[i + 1] == src[i], "odd byte from the even one");
        }
        /* An odd count's last byte is unspecified; nothing after it changes */
        for (i = n; i < SIZE; i++)
            check(dst[i] == '#', "nothing past nbytes");
    }

    /*
     * A negative count does nothing; volatile, as glibc's declaration
     * makes gcc reject negative constants
     */
    volatile ssize_t neg[] = { -1, -2, -SIZE };

    memset(dst, '#', SIZE);
    for (i = 0; i < 3; i++)
        swab(src, dst, neg[i]);
    for (i = 0; i < SIZE; i++)
        check(dst[i] == '#', "negative nbytes does nothing");

    printf("success\n");
    return 0;
}
