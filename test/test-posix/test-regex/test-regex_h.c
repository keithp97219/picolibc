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

/*
 * <regex.h>: its types, constants and prototypes, checked before any
 * other header, so size_t has to come from <regex.h> itself
 */

static size_t   size_check;
static regoff_t regoff_check;

/* regex_t has a size_t re_nsub, regmatch_t regoff_t rm_so and rm_eo */
static size_t *
nsub_check(regex_t *r)
{
    return &r->re_nsub;
}
static regoff_t *
so_check(regmatch_t *m)
{
    return &m->rm_so;
}
static regoff_t *
eo_check(regmatch_t *m)
{
    return &m->rm_eo;
}

/* The prototypes */
typedef int    regcomp_t(regex_t    *restrict preg, const char    *restrict pattern, int cflags);

typedef size_t regerror_t(int    errcode, const regex_t    *restrict preg, char    *restrict errbuf,
                          size_t errbuf_size);

typedef int    regexec_t(const regex_t    *restrict preg, const char    *restrict string, size_t nmatch,
                         regmatch_t pmatch[restrict], int eflags);

typedef void   regfree_t(regex_t *preg);

static regcomp_t  *regcomp_check = regcomp;
static regerror_t *regerror_check = regerror;
static regexec_t  *regexec_check = regexec;
static regfree_t  *regfree_check = regfree;

/* The flags are distinct bits, so they can be ORed */
#define CFLAGS (REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE)
_Static_assert(REG_EXTENDED && REG_ICASE && REG_NOSUB && REG_NEWLINE, "cflags");
_Static_assert((REG_EXTENDED & REG_ICASE) == 0 && (REG_EXTENDED & REG_NOSUB) == 0
                   && (REG_EXTENDED & REG_NEWLINE) == 0 && (REG_ICASE & REG_NOSUB) == 0
                   && (REG_ICASE & REG_NEWLINE) == 0 && (REG_NOSUB & REG_NEWLINE) == 0,
               "cflags are distinct bits");
_Static_assert(REG_NOTBOL && REG_NOTEOL && (REG_NOTBOL & REG_NOTEOL) == 0, "eflags");
#ifdef REG_MINIMAL
_Static_assert(REG_MINIMAL && (REG_MINIMAL & CFLAGS) == 0, "REG_MINIMAL");
#endif

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* regoff_t is signed, and holds any ptrdiff_t or ssize_t */
_Static_assert((regoff_t)-1 < 0, "regoff_t is signed");
/* glibc 2.43 makes regoff_t an int */
#ifndef __GLIBC__
_Static_assert(sizeof(regoff_t) >= sizeof(ptrdiff_t) && sizeof(regoff_t) >= sizeof(size_t),
               "regoff_t is wide enough");
#endif

static const int codes[] = {
    REG_NOMATCH, REG_BADPAT, REG_ECOLLATE, REG_ECTYPE, REG_EESCAPE, REG_ESUBREG, REG_EBRACK,
    REG_EPAREN,  REG_EBRACE, REG_BADBR,    REG_ERANGE, REG_ESPACE,  REG_BADRPT,
};

int
main(void)
{
    unsigned   i, j;
    int        errors = 0;
    regex_t    r;
    regmatch_t m;

    (void)size_check;
    (void)regoff_check;
    if (nsub_check(&r) != &r.re_nsub || so_check(&m) != &m.rm_so || eo_check(&m) != &m.rm_eo)
        errors++;
    if (!regcomp_check || !regerror_check || !regexec_check || !regfree_check)
        errors++;

    /* The error codes are non-zero and distinct */
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

#ifndef REG_MINIMAL
    /* POSIX.1-2024 added REG_MINIMAL; neither picolibc nor glibc 2.43 has it */
    printf("no REG_MINIMAL\n");
#endif

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
