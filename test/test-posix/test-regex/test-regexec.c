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
#include "../test-flags.h"
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * regexec against the matching rules of XBD 9 and the reporting rules
 * of regexec: the examples the standard gives, leftmost-longest
 * matching, which substring each subexpression reports, the bracket
 * expression forms in the POSIX locale and the flags
 */

#define NM   5
#define SKIP -2 /* don't check this offset */

struct test {
    const char *pattern;
    int         cflags;
    const char *string;
    int         eflags;
    int         ret;    /* from regexec */
    int         nmatch; /* pmatch entries to check; the rest up to NM are -1 */
    regoff_t    m[NM][2];
};

#define BRE 0
#define ERE REG_EXTENDED
#define NO  REG_NOMATCH

static const struct test tests[] = {
    /* XBD 9.1: leftmost, then longest, and each subpattern longest */
    { "bb*",                       BRE,               "abbbc",              0,                       0,  1, { { 1, 4 } }                         },
    { "(wee|week)(knights|night)", ERE,               "weeknights",         0,                       0,  3, { { 0, 10 }, { 0, 3 }, { 3, 10 } }   },
    { "\\(.*\\).*",                BRE,               "abcdef",             0,                       0,  2, { { 0, 6 }, { 0, 6 } }               },
    { "\\(a*\\)*",                 BRE,               "bc",                 0,                       0,  2, { { 0, 0 }, { 0, 0 } }               },
    { "(a*)(ab)*b",                ERE,               "aabb",               0,                       0,  3, { { 0, 4 }, { 0, 1 }, { 1, 3 } }     },

    /* XBD 9.3.6: back-references */
    { "^\\(.*\\)\\1$",             BRE,               "abcabc",             0,                       0,  2, { { 0, 6 }, { 0, 3 } }               },
    { "^\\(.*\\)\\1$",             BRE,               "abcab",              0,                       NO, 0, { { 0 } }                            },
    { "\\(a\\)*\\1",               BRE,               "a",                  0,                       NO, 0, { { 0 } }                            },
    { "\\(a\\(b\\)*\\)*\\2",       BRE,               "ababb",              0,                       0,  3, { { 0, 5 }, { 2, 4 }, { 3, 4 } }     },
    { "^\\(ab*\\)*\\1$",           BRE,               "ababbabb",           0,                       0,  2, { { 0, 8 }, { 2, 5 } }               },
    { "^\\(ab*\\)*\\1$",           BRE,               "ababbab",            0,                       NO, 0, { { 0 } }                            },
    { "\\(a*\\)\\1x",              BRE,               "aaaax",              0,                       0,  2, { { 0, 5 }, { 0, 2 } }               },
    { "\\(.\\)\\1",                BRE | REG_ICASE,   "xaAy",               0,                       0,  2, { { 1, 3 }, { 1, 2 } }               },

    /* XBD 9.3.6: intervals */
    { "c\\{3\\}",                  BRE,               "abababccccccd",      0,                       0,  1, { { 6, 9 } }                         },
    { "\\(ab\\)\\{4,\\}",          BRE,               "abababccccccd",      0,                       NO, 0, { { 0 } }                            },
    { "c\\{1,3\\}d",               BRE,               "abababccccccd",      0,                       0,  1, { { 9, 13 } }                        },
    { "\\(ab\\)\\{0,1\\}c",        BRE,               "xc",                 0,                       0,  2, { { 1, 2 }, { -1, -1 } }             },
    { "a\\{0,\\}",                 BRE,               "aaa",                0,                       0,  1, { { 0, 3 } }                         },

    /* XBD 9.3.3: where * is ordinary in a BRE */
    { "*a",                        BRE,               "x*a",                0,                       0,  1, { { 1, 3 } }                         },
    { "\\(*a\\)",                  BRE,               "x*a",                0,                       0,  2, { { 1, 3 }, { 1, 3 } }               },
    { "^*a",                       BRE,               "*a",                 0,                       0,  1, { { 0, 2 } }                         },
    { "\\]",                       BRE,               "a]",                 0,                       0,  1, { { 1, 2 } }                         },

    /* XBD 9.3.8: BRE anchors */
    { "^ab",                       BRE,               "abcdef",             0,                       0,  1, { { 0, 2 } }                         },
    { "^ab",                       BRE,               "cdefab",             0,                       NO, 0, { { 0 } }                            },
    { "^abcdef$",                  BRE,               "abcdef",             0,                       0,  1, { { 0, 6 } }                         },
    { "^abcdef$",                  BRE,               "abcdefg",            0,                       NO, 0, { { 0 } }                            },
    { "a^b",                       BRE,               "a^b",                0,                       0,  1, { { 0, 3 } }                         },
    { "a\\$",                      BRE,               "a$b",                0,                       0,  1, { { 0, 2 } }                         },

    /* XBD 9.4.6: ERE repetition */
    { "b+(bc)",                    ERE,               "acabbbcde",          0,                       0,  2, { { 3, 7 }, { 5, 7 } }               },
    { "b*c",                       ERE,               "cabbbcde",           0,                       0,  1, { { 0, 1 } }                         },
    { "b*cd",                      ERE,               "cabbbcdebbbbbbcdbc", 0,                       0,  1, { { 2, 7 } }                         },
    { "b?c",                       ERE,               "acabbbcde",          0,                       0,  1, { { 1, 2 } }                         },
    { "c{3}",                      ERE,               "abababccccccd",      0,                       0,  1, { { 6, 9 } }                         },
    { "(ab){2,}",                  ERE,               "abababccccccd",      0,                       0,  2, { { 0, 6 }, { 4, 6 } }               },
    { "[ab]+",                     ERE,               "cabd",               0,                       0,  1, { { 1, 3 } }                         },

    /* XBD 9.4.7 and 9.4.8: alternation and precedence */
    { "a((bc)|d)",                 ERE,               "abc",                0,                       0,  3, { { 0, 3 }, { 1, 3 }, { 1, 3 } }     },
    { "a((bc)|d)",                 ERE,               "ad",                 0,                       0,  3, { { 0, 2 }, { 1, 2 }, { -1, -1 } }   },
    { "abba|cde",                  ERE,               "abbcde",             0,                       0,  1, { { 3, 6 } }                         },
    { "abba|cde",                  ERE,               "abbade",             0,                       0,  1, { { 0, 4 } }                         },

    /* XBD 9.4.9: ERE anchors */
    { "(^ab)",                     ERE,               "abcdef",             0,                       0,  2, { { 0, 2 }, { 0, 2 } }               },
    { "(^ab)",                     ERE,               "cdefab",             0,                       NO, 0, { { 0 } }                            },
    { "a^b",                       ERE,               "a^b",                0,                       NO, 0, { { 0 } }                            },
    { "(ef$)",                     ERE,               "abcdef",             0,                       0,  2, { { 4, 6 }, { 4, 6 } }               },
    { "e$f",                       ERE,               "e$f",                0,                       NO, 0, { { 0 } }                            },
    { "x|^a",                      ERE,               "ba",                 0,                       NO, 0, { { 0 } }                            },

    /* XBD 9.4.1, 9.4.2, 9.4.3: escapes and ')' alone in an ERE */
    { "a\\]\\}",                   ERE,               "xa]}",               0,                       0,  1, { { 1, 4 } }                         },
    { "\\(\\*\\+\\?\\{\\|\\.",     ERE,               "x(*+?{|.",           0,                       0,  1, { { 1, 8 } }                         },
    { "a)",                        ERE,               "xa)",                0,                       0,  1, { { 1, 3 } }                         },

    /* regexec: subexpressions that didn't take part are -1 */
    { "(a)*b",                     ERE,               "b",                  0,                       0,  2, { { 0, 1 }, { -1, -1 } }             },
    { "(a)?b",                     ERE,               "b",                  0,                       0,  2, { { 0, 1 }, { -1, -1 } }             },
    { "(a){0,2}b",                 ERE,               "b",                  0,                       0,  2, { { 0, 1 }, { -1, -1 } }             },
    { "\\(a\\)*b",                 BRE,               "b",                  0,                       0,  2, { { 0, 1 }, { -1, -1 } }             },
    { "(a)|(b)",                   ERE,               "b",                  0,                       0,  3, { { 0, 1 }, { -1, -1 }, { 0, 1 } }   },
    /* the last of several matches */
    { "(.)*",                      ERE,               "abc",                0,                       0,  2, { { 0, 3 }, { 2, 3 } }               },
    { "\\(.\\)*",                  BRE,               "abc",                0,                       0,  2, { { 0, 3 }, { 2, 3 } }               },
    /* nested: within the substring the outer one reports */
    { "((a)|b)+",                  ERE,               "ba",                 0,                       0,  3, { { 0, 2 }, { 1, 2 }, { 1, 2 } }     },
    { "x((a)b)?y",                 ERE,               "xy",                 0,                       0,  3, { { 0, 2 }, { -1, -1 }, { -1, -1 } } },
    /* empty matches are at the following character */
    { "a(b*)",                     ERE,               "ac",                 0,                       0,  2, { { 0, 1 }, { 1, 1 } }               },
    { "(a*)b",                     ERE,               "cb",                 0,                       0,  2, { { 1, 2 }, { 1, 1 } }               },
    { "x(a*)*y",                   ERE,               "xy",                 0,                       0,  2, { { 0, 2 }, { 1, 1 } }               },

    /* XBD 9.3.5: bracket expressions in the POSIX locale */
    { "[[=a=]b]*",                 ERE,               "abac",               0,                       0,  1, { { 0, 3 } }                         },
    { "[[.].]a]*",                 ERE,               "]a]b",               0,                       0,  1, { { 0, 3 } }                         },
    { "[[.a.]-c]*",                ERE,               "abcd",               0,                       0,  1, { { 0, 3 } }                         },
    { "[][.-.]-0]*",               ERE,               "]-./0a",             0,                       0,  1, { { 0, 5 } }                         },
    { "[%--]*",                    ERE,               "%+,-.",              0,                       0,  1, { { 0, 4 } }                         },
    { "[--@]*",                    ERE,               "-09@A",              0,                       0,  1, { { 0, 4 } }                         },
    { "[-ac]*",                    ERE,               "-acb",               0,                       0,  1, { { 0, 3 } }                         },
    { "[ac-]*",                    ERE,               "-acb",               0,                       0,  1, { { 0, 3 } }                         },
    { "[^-ac]*",                   ERE,               "bd-",                0,                       0,  1, { { 0, 2 } }                         },
    { "[^ac-]*",                   ERE,               "bd-",                0,                       0,  1, { { 0, 2 } }                         },
    { "[.*[\\]*",                  BRE,               ".*[\\x",             0,                       0,  1, { { 0, 4 } }                         },
    { "[.(*+?{|$[\\^]*",           ERE,               ".(*+?{|$[\\^x",      0,                       0,  1, { { 0, 11 } }                        },
    { "[a^]*",                     ERE,               "a^b",                0,                       0,  1, { { 0, 2 } }                         },
    { "[^a]",                      ERE,               "a\n",                0,                       0,  1, { { 1, 2 } }                         },
    { "[[:alnum:]]*",              ERE,               "a1Z_",               0,                       0,  1, { { 0, 3 } }                         },
    { "[[:alpha:]]*",              ERE,               "aZ1",                0,                       0,  1, { { 0, 2 } }                         },
    { "[[:blank:]]*",              ERE,               " \t\n",              0,                       0,  1, { { 0, 2 } }                         },
    { "[[:cntrl:]]*",              ERE,               "\001\177a",          0,                       0,  1, { { 0, 2 } }                         },
    { "[[:digit:]]*",              ERE,               "09a",                0,                       0,  1, { { 0, 2 } }                         },
    { "[[:graph:]]*",              ERE,               "a!~ ",               0,                       0,  1, { { 0, 3 } }                         },
    { "[[:lower:]]*",              ERE,               "azA",                0,                       0,  1, { { 0, 2 } }                         },
    { "[[:print:]]*",              ERE,               "a ~\001",            0,                       0,  1, { { 0, 3 } }                         },
    { "[[:punct:]]*",              ERE,               "!/~a",               0,                       0,  1, { { 0, 3 } }                         },
    { "[[:space:]]*",              ERE,               " \t\n\v\f\ra",       0,                       0,  1, { { 0, 6 } }                         },
    { "[[:upper:]]*",              ERE,               "AZa",                0,                       0,  1, { { 0, 2 } }                         },
    { "[[:xdigit:]]*",             ERE,               "09afAFg",            0,                       0,  1, { { 0, 6 } }                         },
    { "[^[:digit:]x]*",            ERE,               "ab1",                0,                       0,  1, { { 0, 2 } }                         },

    /* XBD 4.1 and regcomp: REG_ICASE */
    { "abc",                       ERE | REG_ICASE,   "xAbC",               0,                       0,  1, { { 1, 4 } }                         },
    { "[a-c]*",                    ERE | REG_ICASE,   "AbCd",               0,                       0,  1, { { 0, 3 } }                         },
    { "[^a]",                      ERE | REG_ICASE,   "Ab",                 0,                       0,  1, { { 1, 2 } }                         },
    { "[[:upper:]]*",              ERE | REG_ICASE,   "aBc1",               0,                       0,  1, { { 0, 3 } }                         },

    /* regcomp: REG_NEWLINE */
    { "a.b",                       ERE,               "a\nb",               0,                       0,  1, { { 0, 3 } }                         },
    { "a[^x]b",                    ERE,               "a\nb",               0,                       0,  1, { { 0, 3 } }                         },
    { "a$",                        ERE,               "a\nb",               0,                       NO, 0, { { 0 } }                            },
    { "^b",                        ERE,               "a\nb",               0,                       NO, 0, { { 0 } }                            },
    { "a.b",                       ERE | REG_NEWLINE, "a\nb",               0,                       NO, 0, { { 0 } }                            },
    { "a[^x]b",                    ERE | REG_NEWLINE, "a\nb",               0,                       NO, 0, { { 0 } }                            },
    { "a[\n]b",                    ERE | REG_NEWLINE, "a\nb",               0,                       0,  1, { { 0, 3 } }                         },
    { "^b",                        BRE | REG_NEWLINE, "a\nb",               REG_NOTBOL,              0,  1, { { 2, 3 } }                         },
    { "a$",                        BRE | REG_NEWLINE, "a\nb",               REG_NOTEOL,              0,  1, { { 0, 1 } }                         },
    { "^a$",                       ERE | REG_NEWLINE, "b\na\nc",            0,                       0,  1, { { 2, 3 } }                         },

    /* regexec: REG_NOTBOL and REG_NOTEOL */
    { "^a",                        BRE,               "a",                  REG_NOTBOL,              NO, 0, { { 0 } }                            },
    { "a$",                        BRE,               "a",                  REG_NOTEOL,              NO, 0, { { 0 } }                            },
    { "a",                         BRE,               "a",                  REG_NOTBOL | REG_NOTEOL, 0,  1, { { 0, 1 } }                         },
    { "\\^a",                      BRE,               "^a",                 REG_NOTBOL,              0,  1, { { 0, 2 } }                         },
};

#define NTEST (sizeof(tests) / sizeof(tests[0]))

/*
 * glibc 2.43 gets these wrong: it doesn't make each subpattern the
 * longest, and it reports a contained subexpression's match from an
 * earlier pass through the one containing it
 */
#ifndef __GLIBC__
static const struct test strict_tests[] = {
    { "(a|ab)(c|bcd)(d*)",   ERE, "abcd", 0, 0,  4, { { 0, 4 }, { 0, 2 }, { 2, 3 }, { 3, 4 } } },
    { "\\(a\\(b\\)*\\)*\\2", BRE, "abab", 0, NO, 0, { { 0 } }                                  },
    { "\\(a\\(b\\)*\\)*\\1", BRE, "abaa", 0, 0,  3, { { 0, 4 }, { 2, 3 }, { -1, -1 } }         },
    { "((a)|b)*",            ERE, "ab",   0, 0,  3, { { 0, 2 }, { 1, 2 }, { -1, -1 } }         },
};
#define NSTRICT (sizeof(strict_tests) / sizeof(strict_tests[0]))
#endif

static int errors;

static void
run(const struct test *test)
{
    regmatch_t match[NM];
    regex_t    regex;
    int        i, ret;

    ret = regcomp(&regex, test->pattern, test->cflags);
    if (ret != 0) {
        printf("\"%s\": regcomp failed with %d\n", test->pattern, ret);
        errors++;
        return;
    }
    memset(match, 0x5a, sizeof(match));
    ret = regexec(&regex, test->string, NM, match, test->eflags);
    if (ret != test->ret) {
        printf("\"%s\" on \"%s\": regexec returned %d, not %d\n", test->pattern, test->string, ret,
               test->ret);
        errors++;
    } else if (ret == 0) {
        for (i = 0; i < NM; i++) {
            regoff_t so = -1, eo = -1;

            if (i < test->nmatch) {
                so = test->m[i][0];
                eo = test->m[i][1];
            }
            if (so == SKIP)
                continue;
            if (match[i].rm_so != so || match[i].rm_eo != eo) {
                printf("\"%s\" on \"%s\": match %d is (%ld,%ld), not (%ld,%ld)\n", test->pattern,
                       test->string, i, (long)match[i].rm_so, (long)match[i].rm_eo, (long)so,
                       (long)eo);
                errors++;
            }
        }
    }
    regfree(&regex);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            errors++;                                  \
        }                                              \
    } while (0)

static int
compile(const char *pattern, int cflags)
{
    regex_t regex;
    int     ret = regcomp(&regex, pattern, cflags);

    if (ret == 0)
        regfree(&regex);
    return ret;
}

int
main(void)
{
    regmatch_t match[4];
    regex_t    regex;
    unsigned   t;
    int        i;
    char       pattern[300], string[300];

#if TEST_POSIX_SIGNALS
    /* Back-references can take exponential time; don't hang the run */
    alarm(60);
#endif
    for (t = 0; t < NTEST; t++)
        run(&tests[t]);
#ifndef __GLIBC__
    for (t = 0; t < NSTRICT; t++)
        run(&strict_tests[t]);
#endif

    /* pmatch past nmatch isn't touched, and elements past re_nsub are -1 */
    check(regcomp(&regex, "(a)(b)(c)", REG_EXTENDED) == 0, "regcomp");
    memset(match, 0x5a, sizeof(match));
    check(regexec(&regex, "abc", 2, match, 0) == 0, "nmatch 2");
    check(match[0].rm_so == 0 && match[0].rm_eo == 3, "nmatch 2, match 0");
    check(match[1].rm_so == 0 && match[1].rm_eo == 1, "nmatch 2, match 1");
    {
        regmatch_t sentinel;

        memset(&sentinel, 0x5a, sizeof(sentinel));
        check(memcmp(&match[2], &sentinel, sizeof(sentinel)) == 0, "past nmatch untouched");
        check(memcmp(&match[3], &sentinel, sizeof(sentinel)) == 0, "past nmatch untouched");
    }
    regfree(&regex);
    check(regcomp(&regex, "(a)", REG_EXTENDED) == 0, "regcomp");
    memset(match, 0x5a, sizeof(match));
    check(regexec(&regex, "a", 4, match, 0) == 0, "nmatch 4");
    check(match[2].rm_so == -1 && match[2].rm_eo == -1 && match[3].rm_so == -1
              && match[3].rm_eo == -1,
          "unused elements are -1");
    /* nmatch 0 ignores pmatch */
    check(regexec(&regex, "a", 0, NULL, 0) == 0, "nmatch 0");
    regfree(&regex);

    /* REG_NOSUB ignores pmatch, and re_nsub need not be set */
    check(regcomp(&regex, "(a)", REG_EXTENDED | REG_NOSUB) == 0, "REG_NOSUB");
    memset(match, 0x5a, sizeof(match));
    {
        regmatch_t sentinel[4];

        memset(sentinel, 0x5a, sizeof(sentinel));
        check(regexec(&regex, "xa", 4, match, 0) == 0, "REG_NOSUB match");
        check(memcmp(match, sentinel, sizeof(match)) == 0, "REG_NOSUB pmatch untouched");
        check(regexec(&regex, "x", 4, match, 0) == REG_NOMATCH, "REG_NOSUB no match");
    }
    regfree(&regex);

    /* re_nsub counts \( and ( */
    check(regcomp(&regex, "\\(a\\(b\\)\\)\\(c\\)", 0) == 0 && regex.re_nsub == 3, "BRE re_nsub");
    regfree(&regex);
    check(regcomp(&regex, "((a)|(b))*(c)?x", REG_EXTENDED) == 0 && regex.re_nsub == 4,
          "ERE re_nsub");
    regfree(&regex);
    check(regcomp(&regex, "[(]a\\(", REG_EXTENDED) == 0 && regex.re_nsub == 0, "ERE re_nsub 0");
    regfree(&regex);

    /* XBD 9.3.5: invalid bracket expressions */
    check(compile("[[.ab.]]", REG_EXTENDED) == REG_ECOLLATE, "multi-character collating symbol");
    check(compile("[[.nosuch.]]", 0) == REG_ECOLLATE, "unknown collating symbol");
    check(compile("[[:nosuch:]]", 0) == REG_ECTYPE, "unknown class");
    check(compile("[z-a]", 0) == REG_ERANGE, "reversed range");
    check(compile("[a", 0) == REG_EBRACK, "[ imbalance");
    check(compile("[]", REG_EXTENDED) == REG_EBRACK, "[] imbalance");
    check(compile("[[:alpha:]", REG_EXTENDED) == REG_EBRACK, "[[:alpha:] imbalance");

    /* XBD 9.3.6 and 9.4.6: intervals up to RE_DUP_MAX, and no further */
    snprintf(pattern, sizeof(pattern), "^a\\{%d\\}$", RE_DUP_MAX);
    check(compile(pattern, 0) == 0, "RE_DUP_MAX");
    snprintf(pattern, sizeof(pattern), "a{0,%d}", RE_DUP_MAX);
    /* glibc 2.43 runs out of memory for {0,32767} on 32-bit targets */
    i = compile(pattern, REG_EXTENDED);
    check(i == 0 || i == REG_ESPACE, "RE_DUP_MAX ERE");
    /* glibc 2.43 accepts counts past its RE_DUP_MAX of 32767 */
#ifndef __GLIBC__
    snprintf(pattern, sizeof(pattern), "a\\{%ld\\}", (long)RE_DUP_MAX + 1);
    check(compile(pattern, 0) == REG_BADBR, "past RE_DUP_MAX");
    snprintf(pattern, sizeof(pattern), "a{1,%ld}", (long)RE_DUP_MAX + 1);
    check(compile(pattern, REG_EXTENDED) == REG_BADBR, "past RE_DUP_MAX ERE");
#endif
    check(compile("a\\{2,1\\}", 0) == REG_BADBR, "m > n");
    check(compile("a{2,1}", REG_EXTENDED) == REG_BADBR, "m > n ERE");
    check(compile("a\\{1", 0) == REG_EBRACE, "\\{ imbalance");
    check(compile("a{1", REG_EXTENDED) == REG_EBRACE, "{ imbalance");

    /* Back-references and parentheses */
    check(compile("\\(a\\)\\2", 0) == REG_ESUBREG, "\\2 with one subexpression");
    check(compile("\\1\\(a\\)", 0) == REG_ESUBREG, "\\1 before the subexpression");
    check(compile("\\(a", 0) == REG_EPAREN, "\\( imbalance");
    check(compile("(a", REG_EXTENDED) == REG_EPAREN, "( imbalance");
    check(compile("a\\", 0) == REG_EESCAPE, "trailing backslash");
    check(compile("a\\", REG_EXTENDED) == REG_EESCAPE, "trailing backslash ERE");

    /* XBD 9.2: a 256-byte RE is supported */
    for (i = 0; i < 256; i++)
        pattern[i] = (i % 4 == 3) ? '*' : 'a' + i % 4;
    pattern[256] = '\0';
    for (i = 0; i < 192; i++)
        string[i] = 'a' + i % 3;
    string[192] = '\0';
    check(regcomp(&regex, pattern, 0) == 0, "256-byte RE");
    check(regexec(&regex, string, 0, NULL, 0) == 0, "256-byte RE match");
    regfree(&regex);

    /* regfree doesn't change errno */
    check(regcomp(&regex, "\\(a\\)\\1", 0) == 0, "regcomp");
    errno = 1234;
    regfree(&regex);
    check(errno == 1234, "regfree errno");

    /* A compiled RE can be used again, and by several matches */
    check(regcomp(&regex, "b", 0) == 0, "regcomp");
    for (i = 0; i < 3; i++) {
        check(regexec(&regex, "abc", 1, match, 0) == 0 && match[0].rm_so == 1, "repeated regexec");
        check(regexec(&regex, "acd", 1, match, 0) == REG_NOMATCH, "repeated regexec");
    }
    regfree(&regex);

    if (errors) {
        printf("%d errors\n", errors);
        return 1;
    }
    printf("success\n");
    return 0;
}
