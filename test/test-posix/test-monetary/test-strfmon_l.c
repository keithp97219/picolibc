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
/* strfmon is an XSI interface */
#define _XOPEN_SOURCE 700
#include <unistd.h>

#include <errno.h>
#include <locale.h>
#include <monetary.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strfmon_l formats like strfmon in the locale it is given */

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
    locale_t loc = newlocale(LC_ALL_MASK, "POSIX", (locale_t)0);
    char     a[64], b[64];
    ssize_t  n;

    check(loc != (locale_t)0, "newlocale");
    n = strfmon_l(a, sizeof(a), loc, "[%n] [%10.2n] [%-10.2n] [%i]", 1234.5, -2.25, 3.0, 4.5);
    check(n > 0 && (size_t)n == strlen(a), "length");
    n = strfmon(b, sizeof(b), "[%n] [%10.2n] [%-10.2n] [%i]", 1234.5, -2.25, 3.0, 4.5);
    check(n > 0 && strcmp(a, b) == 0, "same as strfmon in the POSIX locale");
    check(strstr(a, "[      2.25]") != NULL || strstr(a, "[     -2.25]") != NULL, "right aligned");

    /* Too small a buffer */
    errno = 0;
    check(strfmon_l(a, 4, loc, "%n", 1234.5) == -1 && errno == E2BIG, "E2BIG");
    freelocale(loc);
    printf("success\n");
    return 0;
}
