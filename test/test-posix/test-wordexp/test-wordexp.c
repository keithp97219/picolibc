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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                             \
    || (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0)

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

/* wordexp expands words as the shell does, and wordfree frees them */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int
words(wordexp_t *we, int count, ...)
{
    const char *w[8];
    int         i;
    va_list     ap;

    if ((int)we->we_wordc != count)
        return 0;
    va_start(ap, count);
    for (i = 0; i < count; i++)
        w[i] = va_arg(ap, const char *);
    va_end(ap);
    for (i = 0; i < count; i++)
        if (strcmp(we->we_wordv[we->we_offs + i], w[i]) != 0)
            return 0;
    return we->we_wordv[we->we_offs + count] == NULL;
}

int
main(void)
{
    wordexp_t we;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(wordexp("one  two\tthree", &we, 0) == 0 && words(&we, 3, "one", "two", "three"),
          "field splitting");
    wordfree(&we);
    check(wordexp("'a b' \"c d\" e\\ f", &we, 0) == 0 && words(&we, 3, "a b", "c d", "e f"),
          "quoting");
    wordfree(&we);
    check(setenv("WORDEXP_TEST", "value", 1) == 0, "setenv");
    check(wordexp("$WORDEXP_TEST ${WORDEXP_TEST}s", &we, 0) == 0
              && words(&we, 2, "value", "values"),
          "parameters");
    wordfree(&we);
    check(wordexp("$((6*7))", &we, 0) == 0 && words(&we, 1, "42"), "arithmetic");
    wordfree(&we);
    check(wordexp("$(echo hi) `echo there`", &we, 0) == 0 && words(&we, 2, "hi", "there"),
          "command substitution");
    wordfree(&we);
    check(wordexp("", &we, 0) == 0 && words(&we, 0), "nothing");
    wordfree(&we);
    check(wordexp("$WORDEXP_UNSET_VARIABLE", &we, 0) == 0 && words(&we, 0), "unset");
    wordfree(&we);

    /* Errors */
    check(wordexp("a|b", &we, 0) == WRDE_BADCHAR, "|");
    check(wordexp("a;b", &we, 0) == WRDE_BADCHAR, ";");
    check(wordexp("a > b", &we, 0) == WRDE_BADCHAR, ">");
    check(wordexp("'a|b'", &we, 0) == 0 && words(&we, 1, "a|b"), "quoted |");
    wordfree(&we);
    check(wordexp("$(echo hi)", &we, WRDE_NOCMD) == WRDE_CMDSUB, "WRDE_NOCMD $()");
    check(wordexp("`echo hi`", &we, WRDE_NOCMD) == WRDE_CMDSUB, "WRDE_NOCMD ``");
    check(wordexp("$((1+1))", &we, WRDE_NOCMD) == 0 && words(&we, 1, "2"),
          "WRDE_NOCMD allows arithmetic");
    wordfree(&we);
    check(wordexp("$(( (1+2)*(3+3) ))", &we, WRDE_NOCMD) == 0 && words(&we, 1, "18"),
          "WRDE_NOCMD allows nested arithmetic");
    /* Some shells read these as command substitutions */
    check(wordexp("$((echo hi); id)", &we, WRDE_NOCMD) == WRDE_CMDSUB, "WRDE_NOCMD $(( ) )");
    check(wordexp("$((echo hi) )", &we, WRDE_NOCMD) == WRDE_CMDSUB, "WRDE_NOCMD $(( ) )");
    check(wordexp("$((1;2))", &we, WRDE_NOCMD) != 0, "WRDE_NOCMD ; in $(( ))");
    wordfree(&we);
    check(wordexp("$WORDEXP_UNSET_VARIABLE", &we, WRDE_UNDEF) == WRDE_BADVAL, "WRDE_UNDEF");
    check(wordexp("\"unterminated", &we, 0) == WRDE_SYNTAX, "WRDE_SYNTAX");

    /* Offsets, appending and reuse */
    we.we_offs = 2;
    check(wordexp("a b", &we, WRDE_DOOFFS) == 0, "WRDE_DOOFFS");
    check(we.we_wordv[0] == NULL && we.we_wordv[1] == NULL && words(&we, 2, "a", "b"),
          "offset words");
    check(wordexp("c", &we, WRDE_DOOFFS | WRDE_APPEND) == 0 && words(&we, 3, "a", "b", "c"),
          "WRDE_APPEND");
    check(wordexp("d", &we, WRDE_DOOFFS | WRDE_REUSE) == 0 && words(&we, 1, "d"), "WRDE_REUSE");
    wordfree(&we);
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
