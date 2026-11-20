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
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* regerror describes each error, truncating to fit and reporting the full size */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static const int codes[] = {
    REG_NOMATCH, REG_BADPAT, REG_ECOLLATE, REG_ECTYPE, REG_EESCAPE, REG_ESUBREG, REG_EBRACK,
    REG_EPAREN,  REG_EBRACE, REG_BADBR,    REG_ERANGE, REG_ESPACE,  REG_BADRPT,
};

int
main(void)
{
    char     buf[256], small[4];
    regex_t  regex;
    size_t   size;
    unsigned i, j;

    for (i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        /* Without a buffer, the size needed, including the null */
        size = regerror(codes[i], NULL, NULL, 0);
        check(size > 1 && size <= sizeof(buf), "size");
        check(regerror(codes[i], NULL, buf, sizeof(buf)) == size, "same size");
        check(strlen(buf) == size - 1, "message length");

        /* Each code has its own message */
        for (j = 0; j < i; j++) {
            char other[256];

            regerror(codes[j], NULL, other, sizeof(other));
            check(strcmp(buf, other) != 0, "distinct messages");
        }

        /* A short buffer gets the start of the message, null terminated */
        memset(small, 'x', sizeof(small));
        check(regerror(codes[i], NULL, small, sizeof(small)) == size, "truncated size");
        check(small[sizeof(small) - 1] == '\0', "terminated");
        check(strncmp(small, buf, sizeof(small) - 1) == 0, "prefix");
    }

    /* The regex_t that produced the error may be passed */
    check(regcomp(&regex, "a\\{1", 0) == REG_EBRACE, "regcomp");
    check(regerror(REG_EBRACE, &regex, buf, sizeof(buf)) > 1, "with preg");

    printf("success\n");
    return 0;
}
