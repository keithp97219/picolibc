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
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

/*
 * getsubopt: commas separate suboptions, the first '=' separates a
 * token from its value, and nothing else is special. *optionp moves
 * to the next suboption, or to the terminating null after the last.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            return 1;                                  \
        }                                              \
    } while (0)

static char * const tokens[] = { "ro", "rw", "size", "name with blank", "", NULL };

int
main(void)
{
    char  buf[128];
    char *opt;
    char *value;
    char *end;
    int   r;

    strcpy(buf, "ro,size=10,rw");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == 0 && value == NULL && opt == buf + 3, "a token without a value");
    r = getsubopt(&opt, tokens, &value);
    check(r == 2 && value != NULL && strcmp(value, "10") == 0, "a token with a value");
    check(opt == buf + 11, "optionp moves past the comma");
    end = buf + strlen("ro,size=10,rw");
    r = getsubopt(&opt, tokens, &value);
    check(r == 1 && value == NULL && opt == end && *opt == '\0', "the last suboption");

    /* Subsequent '=' are part of the value */
    strcpy(buf, "size=a=b");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == 2 && strcmp(value, "a=b") == 0 && *opt == '\0', "'=' in the value");

    /* Blanks are part of the token and the value */
    strcpy(buf, "name with blank=x y,size= 5 ,ro");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == 3 && strcmp(value, "x y") == 0, "blanks in the token and the value");
    r = getsubopt(&opt, tokens, &value);
    check(r == 2 && strcmp(value, " 5 ") == 0, "blanks around the value");
    r = getsubopt(&opt, tokens, &value);
    check(r == 0 && value == NULL && *opt == '\0', "after blanks");

    /* A blank before a token makes another token */
    strcpy(buf, "ro, rw");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == 0, "ro");
    r = getsubopt(&opt, tokens, &value);
    check(r == -1 && *opt == '\0', "' rw' isn't 'rw'");

    /* An empty suboption is a suboption */
    strcpy(buf, "ro,,rw");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == 0 && opt == buf + 3, "ro before an empty suboption");
    r = getsubopt(&opt, tokens, &value);
    check(r == 4 && value == NULL && opt == buf + 4, "the empty suboption");
    r = getsubopt(&opt, tokens, &value);
    check(r == 1 && *opt == '\0', "rw after an empty suboption");

    /* No match: -1, with the value still found */
    strcpy(buf, "bogus=7,rw");
    opt = buf;
    r = getsubopt(&opt, tokens, &value);
    check(r == -1 && opt == buf + 8, "an unknown token");
#ifndef __GLIBC__
    /* glibc points value at the whole suboption when nothing matches */
    check(value != NULL && strcmp(value, "7") == 0, "the unknown token's value");
#endif
    r = getsubopt(&opt, tokens, &value);
    check(r == 1, "rw after an unknown token");

    /* The token list isn't changed */
    check(strcmp(tokens[0], "ro") == 0 && strcmp(tokens[2], "size") == 0 && tokens[5] == NULL,
          "keylistp unchanged");

    printf("success\n");
    return 0;
}
