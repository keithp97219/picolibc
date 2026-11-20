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

#include <fnmatch.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>

static int
y(char const *pattern, char const *string, int flags)
{
    return fnmatch(pattern, string, flags) == 0;
}
static int
n(char const *pattern, char const *string, int flags)
{
    return fnmatch(pattern, string, flags) == FNM_NOMATCH;
}

static int
fail(int val, int line)
{
    printf("Failure type %d at line %d\n", val, line);
    return val;
}

int
main(void)
{
    char const       *Apat = 'A' < '\\' ? "[A-\\\\]" : "[\\\\-A]";
    char const       *apat = 'a' < '\\' ? "[a-\\\\]" : "[\\\\-a]";
    static char const A_1[] = { 'A' - 1, 0 };
    static char const A01[] = { 'A' + 1, 0 };
    static char const a_1[] = { 'a' - 1, 0 };
    static char const a01[] = { 'a' + 1, 0 };
    static char const bs_1[] = { '\\' - 1, 0 };
    static char const bs01[] = { '\\' + 1, 0 };
    int               result = 0;
    /* ==== Start of tests in the "C" locale ==== */
    /* These are sanity checks. They all succeed on current platforms.  */
    if (!n("a*", "", 0))
        result |= fail(1, __LINE__);
    if (!y("a*", "abc", 0))
        result |= fail(1, __LINE__);
    if (!n("d*/*1", "d/s/1", FNM_PATHNAME))
        result |= fail(1, __LINE__);
    if (!y("a\\bc", "abc", 0))
        result |= fail(1, __LINE__);
    if (!n("a\\bc", "abc", FNM_NOESCAPE))
        result |= fail(1, __LINE__);
    if (!y("*x", ".x", 0))
        result |= fail(1, __LINE__);
    if (!n("*x", ".x", FNM_PERIOD))
        result |= fail(1, __LINE__);
    if (!y(Apat, "\\", 0))
        result |= fail(2, __LINE__);
    if (!y(Apat, "A", 0))
        result |= fail(2, __LINE__);
    if (!y(apat, "\\", 0))
        result |= fail(2, __LINE__);
    if (!y(apat, "a", 0))
        result |= fail(2, __LINE__);
    if (!(n(Apat, A_1, 0) == ('A' < '\\')))
        result |= fail(2, __LINE__);
    if (!(n(apat, a_1, 0) == ('a' < '\\')))
        result |= fail(2, __LINE__);
    if (!(y(Apat, A01, 0) == ('A' < '\\')))
        result |= fail(2, __LINE__);
    if (!(y(apat, a01, 0) == ('a' < '\\')))
        result |= fail(2, __LINE__);
    if (!(y(Apat, bs_1, 0) == ('A' < '\\')))
        result |= fail(2, __LINE__);
    if (!(y(apat, bs_1, 0) == ('a' < '\\')))
        result |= fail(2, __LINE__);
    if (!(n(Apat, bs01, 0) == ('A' < '\\')))
        result |= fail(2, __LINE__);
    if (!(n(apat, bs01, 0) == ('a' < '\\')))
        result |= fail(2, __LINE__);
    /* glibc bug <https://sourceware.org/PR12378>
       exists in glibc 2.12, fixed in glibc 2.13.  */
    if (!y("[/b", "[/b", 0)) /*"]]"*/
        result |= fail(4, __LINE__);
    /* This test fails on FreeBSD 13.2, NetBSD 10.0, Cygwin 3.4.6.  */
    if (!y("[[:alnum:]]", "a", 0))
        result |= fail(8, __LINE__);

    /* Bracket expressions: lists, ranges and non-matching lists */
    if (!y("[abc]", "b", 0) || !n("[abc]", "d", 0) || !n("[abc]", "", 0))
        result |= fail(64, __LINE__);
    if (!y("[a-c]x", "bx", 0) || !n("[a-c]x", "dx", 0))
        result |= fail(64, __LINE__);
    if (!y("[!a-c]", "d", 0) || !n("[!a-c]", "b", 0) || !n("[!a-c]", "", 0))
        result |= fail(64, __LINE__);
    /* A right bracket is literal first in the list, even after '!' */
    if (!y("[]a]", "]", 0) || !y("[]a]", "a", 0) || !n("[]a]", "b", 0))
        result |= fail(64, __LINE__);
    if (!y("[!]a]", "b", 0) || !n("[!]a]", "]", 0))
        result |= fail(64, __LINE__);
    /* A hyphen is literal first or last, or as the end of a range */
    if (!y("[-ac]", "-", 0) || !y("[ac-]", "-", 0) || !n("[ac-]", "b", 0))
        result |= fail(64, __LINE__);
    if (!y("[!-ac]", "b", 0) || !n("[!ac-]", "-", 0))
        result |= fail(64, __LINE__);
    if (!y("[%--]", "+", 0) || !y("[%--]", "-", 0) || !n("[%--]", ".", 0))
        result |= fail(64, __LINE__);
    if (!y("[--@]", "0", 0) || !n("[--@]", "A", 0))
        result |= fail(64, __LINE__);
    if (!y("[][.-.]-0]", "]", 0) || !y("[][.-.]-0]", "/", 0) || !n("[][.-.]-0]", "1", 0))
        result |= fail(64, __LINE__);
    /* '*', '?' and '[' are ordinary in a bracket expression */
    if (!y("[*?[]", "*", 0) || !y("[*?[]", "?", 0) || !y("[*?[]", "[", 0) || !n("[*?[]", "a", 0))
        result |= fail(64, __LINE__);
    /* A '[' that does not introduce a bracket expression matches itself */
    if (!y("[", "[", 0) || !y("a[b", "a[b", 0) || !n("a[b", "ab", 0) || !y("[!", "[!", 0))
        result |= fail(64, __LINE__);
    /* Collating symbols and equivalence classes */
    if (!y("[[.a.]]", "a", 0) || !n("[[.a.]]", "b", 0) || !y("[[.].]]", "]", 0))
        result |= fail(64, __LINE__);
    if (!y("[[=a=]b]", "a", 0) || !y("[[=a=]b]", "b", 0) || !n("[[=a=]b]", "c", 0))
        result |= fail(64, __LINE__);

    /* Every character class required in all locales */
    if (!y("[[:alnum:]]", "7", 0) || !n("[[:alnum:]]", "_", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:alpha:]]", "Q", 0) || !n("[[:alpha:]]", "1", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:blank:]]", "\t", 0) || !n("[[:blank:]]", "\n", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:cntrl:]]", "\001", 0) || !n("[[:cntrl:]]", "a", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:digit:]]", "0", 0) || !n("[[:digit:]]", "a", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:graph:]]", "~", 0) || !n("[[:graph:]]", " ", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:lower:]]", "z", 0) || !n("[[:lower:]]", "Z", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:print:]]", " ", 0) || !n("[[:print:]]", "\177", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:punct:]]", "!", 0) || !n("[[:punct:]]", "a", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:space:]]", "\v", 0) || !n("[[:space:]]", "x", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:upper:]]", "A", 0) || !n("[[:upper:]]", "a", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:xdigit:]]", "f", 0) || !n("[[:xdigit:]]", "g", 0))
        result |= fail(64, __LINE__);
    if (!y("[![:digit:]x]", "a", 0) || !n("[![:digit:]x]", "5", 0) || !n("[![:digit:]x]", "x", 0))
        result |= fail(64, __LINE__);
    if (!y("[[:digit:][:upper:]]", "B", 0) || !n("[[:digit:][:upper:]]", "b", 0))
        result |= fail(64, __LINE__);

    /* '?' matches one character, '*' any string including the null string */
    if (!y("?", "a", 0) || !n("?", "", 0) || !n("?", "ab", 0) || !y("a?c", "a/c", 0))
        result |= fail(128, __LINE__);
    if (!y("*", "", 0) || !y("a*", "a", 0) || !y("a*b*c", "aXbYbc", 0) || !n("a*b*c", "aXbYb", 0))
        result |= fail(128, __LINE__);
    if (!y("**a", "xa", 0) || !y("*a*", "a", 0) || !y("*/*", "a/b", 0) || !y("a*", "a/b", 0))
        result |= fail(128, __LINE__);

    /* Backslash quotes the next character, inside brackets too */
    if (!y("\\*", "*", 0) || !n("\\*", "a", 0) || !y("\\?", "?", 0) || !n("\\?", "a", 0))
        result |= fail(128, __LINE__);
    if (!y("\\[a]", "[a]", 0) || !n("\\[a]", "a", 0) || !y("\\\\", "\\", 0))
        result |= fail(128, __LINE__);
    if (!y("[\\]]", "]", 0) || !y("[a\\-c]", "-", 0) || !n("[a\\-c]", "b", 0)
        || !y("[\\!a]", "!", 0))
        result |= fail(128, __LINE__);
    /* FNM_NOESCAPE makes backslash ordinary */
    if (!y("\\*", "\\x", FNM_NOESCAPE) || !n("\\*", "*", FNM_NOESCAPE))
        result |= fail(128, __LINE__);
    if (!y("[\\]a", "\\a", FNM_NOESCAPE) || !n("[\\]a", "a", FNM_NOESCAPE))
        result |= fail(128, __LINE__);

    /* FNM_PATHNAME: only a slash matches a slash */
    if (!y("a/b", "a/b", FNM_PATHNAME) || !n("a?b", "a/b", FNM_PATHNAME))
        result |= fail(128, __LINE__);
    if (!n("a*b", "a/b", FNM_PATHNAME) || !n("a*", "a/b", FNM_PATHNAME)
        || !y("a*/b", "ax/b", FNM_PATHNAME))
        result |= fail(128, __LINE__);
    if (!n("a[/]b", "a/b", FNM_PATHNAME) || !n("a[!x]b", "a/b", FNM_PATHNAME))
        result |= fail(128, __LINE__);
    if (!y("a[!x]b", "a/b", 0) || !y("*", "a/b", 0))
        result |= fail(128, __LINE__);
    /* A slash in a bracket expression doesn't stop the rest matching */
    if (!y("a[/b]c", "abc", FNM_PATHNAME) || !y("[!/]", "a", FNM_PATHNAME))
        result |= fail(128, __LINE__);
    /* An unclosed '[' is ordinary, even when followed by a slash */
    if (!y("[/", "[/", FNM_PATHNAME) || !y("a[/b", "a[/b", FNM_PATHNAME))
        result |= fail(128, __LINE__);

    /* FNM_PERIOD: a leading period must be matched explicitly */
    if (!n("?x", ".x", FNM_PERIOD) || !n("[!a]x", ".x", FNM_PERIOD))
        result |= fail(128, __LINE__);
    if (!n("[%-0]x", ".x", FNM_PERIOD) || !n("[[:punct:]]x", ".x", FNM_PERIOD))
        result |= fail(128, __LINE__);
    if (!y(".x", ".x", FNM_PERIOD) || !y("?x", ".x", 0) || !y("a*", "a.x", FNM_PERIOD))
        result |= fail(128, __LINE__);
    /* Only the first character is leading without FNM_PATHNAME */
    if (!y("a/*", "a/.x", FNM_PERIOD) || !y("a/?x", "a/.x", FNM_PERIOD))
        result |= fail(128, __LINE__);
    /* With FNM_PATHNAME, a period after a slash is leading too */
    if (!n("a/*", "a/.x", FNM_PERIOD | FNM_PATHNAME)
        || !n("a/?x", "a/.x", FNM_PERIOD | FNM_PATHNAME))
        result |= fail(128, __LINE__);
    if (!n("a/[!a]x", "a/.x", FNM_PERIOD | FNM_PATHNAME)
        || !y("a/.*", "a/.x", FNM_PERIOD | FNM_PATHNAME))
        result |= fail(128, __LINE__);
    if (!y("a/*", "a/.x", FNM_PATHNAME) || !y("*/x", ".a/x", FNM_PATHNAME))
        result |= fail(128, __LINE__);
    /* ==== End of tests in the "C" locale ==== */
    /* ==== Start of tests that require a specific locale ==== */
    /* This test fails on Solaris 11.4.  */
    if (setlocale(LC_ALL, "en_US.UTF-8") != NULL) {
        if (!n("[!a-z]", "", 0))
            result |= fail(16, __LINE__);
    }
#ifndef __PICOLIBC__
    /* This test requires working multi-byte support in fnmatch */
    if (setlocale(LC_ALL, "C.UTF-8") != NULL) {
        if (!y("x?y", "x\303\274y", 0))
            result |= fail(32, __LINE__);
    }
#endif
    /* ==== End of tests that require a specific locale ==== */
    return result;
}
