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

/*
 * regcomp and regexec on basic and extended regular expressions, as
 * XBD 9 describes them, with each compile and execution flag
 */

#define NM 4

struct test {
    const char *pattern;
    int         cflags;
    const char *string;
    int         eflags;
    int         ret; /* from regexec, or regcomp's error with nmatch -1 */
    int         nmatch;
    regoff_t    m[NM][2];
};

#define BRE 0
#define ERE REG_EXTENDED

static const struct test tests[] = {
    /* BRE: ordinary characters and anchors */
    { "abc",                       BRE,               "xabcx",      0,          0,            1,  { { 1, 4 } }                       },
    { "^abc",                      BRE,               "xabc",       0,          REG_NOMATCH,  0,  { { 0 } }                          },
    { "abc$",                      BRE,               "abcx",       0,          REG_NOMATCH,  0,  { { 0 } }                          },
    { "a$b",                       BRE,               "xa$b",       0,          0,            1,  { { 1, 4 } }                       },
    { "^*ab",                      BRE,               "*ab",        0,          0,            1,  { { 0, 3 } }                       },
    { "a|b",                       BRE,               "xa|b",       0,          0,            1,  { { 1, 4 } }                       },
    { "a+?",                       BRE,               "aa+?",       0,          0,            1,  { { 1, 4 } }                       },
    { "a{2}",                      BRE,               "a{2}",       0,          0,            1,  { { 0, 4 } }                       },
    { "a\\.c",                     BRE,               "abca.c",     0,          0,            1,  { { 3, 6 } }                       },

    /* BRE: intervals, subexpressions and back-references */
    { "a\\{2\\}",                  BRE,               "aaaa",       0,          0,            1,  { { 0, 2 } }                       },
    { "a\\{2,\\}",                 BRE,               "baaaa",      0,          0,            1,  { { 1, 5 } }                       },
    { "a\\{2,3\\}",                BRE,               "aaaa",       0,          0,            1,  { { 0, 3 } }                       },
    { "xa\\{0\\}y",                BRE,               "xy",         0,          0,            1,  { { 0, 2 } }                       },
    { "\\(ab\\)*c",                BRE,               "ababc",      0,          0,            2,  { { 0, 5 }, { 2, 4 } }             },
    { "\\(a*\\)b\\1",              BRE,               "aabaa",      0,          0,            2,  { { 0, 5 }, { 0, 2 } }             },
    { "\\(.\\)\\1",                BRE,               "xyyz",       0,          0,            2,  { { 1, 3 }, { 1, 2 } }             },
    { "\\(a\\)\\(b\\)\\2\\1",      BRE,               "abba",       0,          0,            3,  { { 0, 4 }, { 0, 1 }, { 1, 2 } }   },
    { "\\(a\\)x\\1",               BRE,               "axb",        0,          REG_NOMATCH,  0,  { { 0 } }                          },

    /* Bracket expressions */
    { "[]a]*",                     BRE,               "]a]b",       0,          0,            1,  { { 0, 3 } }                       },
    { "[^]a]",                     BRE,               "]ab",        0,          0,            1,  { { 2, 3 } }                       },
    { "[a-]*",                     BRE,               "-a-b",       0,          0,            1,  { { 0, 3 } }                       },
    { "[[:digit:]]\\{3\\}",        BRE,               "ab123c",     0,          0,            1,  { { 2, 5 } }                       },
    { "[[:alpha:][:space:]]*",     ERE,               "ab c1",      0,          0,            1,  { { 0, 4 } }                       },
    { "[[.-.]]",                   ERE,               "a-b",        0,          0,            1,  { { 1, 2 } }                       },
    { "[[=a=]]",                   ERE,               "ba",         0,          0,            1,  { { 1, 2 } }                       },
    { "[a-c]*",                    ERE,               "abcd",       0,          0,            1,  { { 0, 3 } }                       },
    { "[.]",                       ERE,               "a.",         0,          0,            1,  { { 1, 2 } }                       },

    /* ERE */
    { "a+",                        ERE,               "baaa",       0,          0,            1,  { { 1, 4 } }                       },
    { "ab?c",                      ERE,               "xac",        0,          0,            1,  { { 1, 3 } }                       },
    { "a{2,3}",                    ERE,               "aaaa",       0,          0,            1,  { { 0, 3 } }                       },
    { "a|bc",                      ERE,               "xbc",        0,          0,            1,  { { 1, 3 } }                       },
    { "(a)|(b)",                   ERE,               "b",          0,          0,            3,  { { 0, 1 }, { -1, -1 }, { 0, 1 } } },
    { "(a*)*",                     ERE,               "b",          0,          0,            2,  { { 0, 0 }, { 0, 0 } }             },
    { "(wee|week)(knights|night)", ERE,               "weeknights", 0,          0,            3,  { { 0, 10 }, { 0, 3 }, { 3, 10 } } },
    { "x(a|ab)y",                  ERE,               "xaby",       0,          0,            2,  { { 0, 4 }, { 1, 3 } }             },
    { "a\\+",                      ERE,               "aa+",        0,          0,            1,  { { 1, 3 } }                       },

    /* Leftmost, then longest */
    { "a*",                        ERE,               "baaa",       0,          0,            1,  { { 0, 0 } }                       },
    { "(a|ab)(c|bcd)",             ERE,               "abcd",       0,          0,            1,  { { 0, 4 } }                       },

    /* Compile flags */
    { "ABC",                       BRE | REG_ICASE,   "xabc",       0,          0,            1,  { { 1, 4 } }                       },
    { "[A-C]*",                    ERE | REG_ICASE,   "abcd",       0,          0,            1,  { { 0, 3 } }                       },
    { "^b",                        ERE,               "a\nb",       0,          REG_NOMATCH,  0,  { { 0 } }                          },
    { "^b",                        ERE | REG_NEWLINE, "a\nb",       0,          0,            1,  { { 2, 3 } }                       },
    { "a$",                        ERE | REG_NEWLINE, "a\nb",       0,          0,            1,  { { 0, 1 } }                       },
    { "a.b",                       ERE,               "a\nb",       0,          0,            1,  { { 0, 3 } }                       },
    { "a.b",                       ERE | REG_NEWLINE, "a\nb",       0,          REG_NOMATCH,  0,  { { 0 } }                          },
    { "a[^x]b",                    ERE | REG_NEWLINE, "a\nb",       0,          REG_NOMATCH,  0,  { { 0 } }                          },

    /* Execution flags */
    { "^a",                        ERE,               "a",          REG_NOTBOL, REG_NOMATCH,  0,  { { 0 } }                          },
    { "^a",                        ERE | REG_NEWLINE, "a\na",       REG_NOTBOL, 0,            1,  { { 2, 3 } }                       },
    { "a$",                        ERE,               "a",          REG_NOTEOL, REG_NOMATCH,  0,  { { 0 } }                          },

    /* Compilation errors */
    { "\\(a",                      BRE,               "",           0,          REG_EPAREN,   -1, { { 0 } }                          },
    { "a\\)",                      BRE,               "",           0,          REG_EPAREN,   -1, { { 0 } }                          },
    { "(a",                        ERE,               "",           0,          REG_EPAREN,   -1, { { 0 } }                          },
    { "[a",                        ERE,               "",           0,          REG_EBRACK,   -1, { { 0 } }                          },
    { "a\\{1",                     BRE,               "",           0,          REG_EBRACE,   -1, { { 0 } }                          },
    { "a{1",                       ERE,               "",           0,          REG_EBRACE,   -1, { { 0 } }                          },
    { "a\\{2,1\\}",                BRE,               "",           0,          REG_BADBR,    -1, { { 0 } }                          },
    { "\\(a\\)\\2",                BRE,               "",           0,          REG_ESUBREG,  -1, { { 0 } }                          },
    { "[[:nosuch:]]",              ERE,               "",           0,          REG_ECTYPE,   -1, { { 0 } }                          },
    { "[b-a]",                     ERE,               "",           0,          REG_ERANGE,   -1, { { 0 } }                          },
    { "a\\",                       ERE,               "",           0,          REG_EESCAPE,  -1, { { 0 } }                          },
    { "[[.nosuch.]]",              ERE,               "",           0,          REG_ECOLLATE, -1, { { 0 } }                          },
};

#define NTEST (sizeof(tests) / sizeof(tests[0]))

int
main(void)
{
    regmatch_t match[NM + 1];
    regex_t    regex;
    unsigned   t;
    int        i, ret, errors = 0;

    for (t = 0; t < NTEST; t++) {
        const struct test *test = &tests[t];

        ret = regcomp(&regex, test->pattern, test->cflags);
        if (test->nmatch < 0) {
            if (ret != test->ret) {
                printf("\"%s\": regcomp returned %d, not %d\n", test->pattern, ret, test->ret);
                errors++;
            }
            if (ret == 0)
                regfree(&regex);
            continue;
        }
        if (ret != 0) {
            printf("\"%s\": regcomp failed with %d\n", test->pattern, ret);
            errors++;
            continue;
        }
        memset(match, 0x5a, sizeof(match));
        ret = regexec(&regex, test->string, NM + 1, match, test->eflags);
        if (ret != test->ret) {
            printf("\"%s\" on \"%s\": regexec returned %d, not %d\n", test->pattern, test->string,
                   ret, test->ret);
            errors++;
        } else if (ret == 0) {
            /* Those listed match; any past re_nsub are -1 */
            for (i = 0; i <= NM; i++) {
                regoff_t so = -1, eo = -1;

                if (i < test->nmatch) {
                    so = test->m[i][0];
                    eo = test->m[i][1];
                } else if (i <= (int)regex.re_nsub) {
                    continue;
                }
                if (match[i].rm_so != so || match[i].rm_eo != eo) {
                    printf("\"%s\" on \"%s\": match %d is (%ld,%ld), not (%ld,%ld)\n",
                           test->pattern, test->string, i, (long)match[i].rm_so,
                           (long)match[i].rm_eo, (long)so, (long)eo);
                    errors++;
                }
            }
        }
        regfree(&regex);
    }

    /* re_nsub counts the subexpressions */
    if (regcomp(&regex, "\\(a\\)\\(b\\(c\\)\\)", 0) != 0 || regex.re_nsub != 3) {
        printf("re_nsub\n");
        errors++;
    } else {
        regfree(&regex);
    }

    /* REG_NOSUB reports only whether there's a match */
    if (regcomp(&regex, "(a)b", REG_EXTENDED | REG_NOSUB) != 0
        || regexec(&regex, "xab", 0, NULL, 0) != 0
        || regexec(&regex, "xb", 0, NULL, 0) != REG_NOMATCH) {
        printf("REG_NOSUB\n");
        errors++;
    } else {
        regfree(&regex);
    }

    /* nmatch 0 ignores pmatch */
    if (regcomp(&regex, "b", 0) != 0 || regexec(&regex, "ab", 0, NULL, 0) != 0) {
        printf("nmatch 0\n");
        errors++;
    } else {
        regfree(&regex);
    }

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
