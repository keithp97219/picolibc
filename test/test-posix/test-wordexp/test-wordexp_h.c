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
#include <wordexp.h>

/*
 * <wordexp.h>: its type, constants and prototypes, checked before any
 * other header, so size_t has to come from <wordexp.h> itself, and the
 * names it mustn't define in strict mode
 */

static size_t size_check;

/* wordexp_t has size_t we_wordc, char **we_wordv and size_t we_offs */
static size_t *
wordc_check(wordexp_t *w)
{
    return &w->we_wordc;
}
static char ***
wordv_check(wordexp_t *w)
{
    return &w->we_wordv;
}
static size_t *
offs_check(wordexp_t *w)
{
    return &w->we_offs;
}

/* The prototypes */
typedef int  wordexp_fn(const char  *restrict words, wordexp_t  *restrict pwordexp, int flags);

typedef void wordfree_fn(wordexp_t *pwordexp);

static wordexp_fn  *wordexp_check = wordexp;
static wordfree_fn *wordfree_check = wordfree;

/* The flags are distinct bits, so they can be ORed */
#define F1 (WRDE_APPEND | WRDE_DOOFFS | WRDE_NOCMD)
#define F2 (WRDE_REUSE | WRDE_SHOWERR | WRDE_UNDEF)
_Static_assert(WRDE_APPEND != 0 && WRDE_DOOFFS != 0 && WRDE_NOCMD != 0 && WRDE_REUSE != 0
                   && WRDE_SHOWERR != 0 && WRDE_UNDEF != 0,
               "flags");
_Static_assert((WRDE_APPEND & (WRDE_DOOFFS | WRDE_NOCMD | F2)) == 0
                   && (WRDE_DOOFFS & (WRDE_NOCMD | F2)) == 0 && (WRDE_NOCMD & F2) == 0
                   && (WRDE_REUSE & (WRDE_SHOWERR | WRDE_UNDEF)) == 0
                   && (WRDE_SHOWERR & WRDE_UNDEF) == 0,
               "flags are distinct bits");

/* <wordexp.h> may only define size_t and the we_ and WRDE_ names */
#ifdef INT8_MAX
#error "<wordexp.h> defines INT8_MAX"
#endif
#ifdef SIZE_MAX
#error "<wordexp.h> defines SIZE_MAX"
#endif
#ifdef FD_SETSIZE
#error "<wordexp.h> defines FD_SETSIZE"
#endif

#include <stdio.h>

static const int codes[] = {
    WRDE_BADCHAR, WRDE_BADVAL, WRDE_CMDSUB, WRDE_NOSPACE, WRDE_SYNTAX,
};

int
main(void)
{
    unsigned  i, j;
    int       errors = 0;
    wordexp_t w;

    (void)size_check;
    if (wordc_check(&w) != &w.we_wordc || wordv_check(&w) != &w.we_wordv
        || offs_check(&w) != &w.we_offs)
        errors++;
    if (!wordexp_check || !wordfree_check)
        errors++;

    /* The errors are non-zero, since 0 is success, and distinct */
    for (i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        if (codes[i] == 0) {
            printf("error %u is 0\n", i);
            errors++;
        }
        for (j = 0; j < i; j++)
            if (codes[i] == codes[j]) {
                printf("errors %u and %u are the same\n", i, j);
                errors++;
            }
    }

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
