/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <fnmatch.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include "../collate.h"

#define EOS           '\0'

#define RANGE_MATCH   1
#define RANGE_NOMATCH 0
#define RANGE_ERROR   (-1)

static int rangematch(const char *, wchar_t, int, char **);

/*
 * Patterns and strings are matched a character at a time, in the
 * current locale's encoding (POSIX: '?' matches one character, not
 * one byte). Bytes that aren't a valid character stand for
 * themselves. Returns the length, 0 at the end of the string.
 */
static size_t
nextchar(const char *s, wchar_t *wc)
{
    mbstate_t state;
    size_t    len;

    if (*s == EOS) {
        *wc = EOS;
        return 0;
    }
    memset(&state, 0, sizeof(state));
    len = mbrtowc(wc, s, MB_LEN_MAX, &state);
    if (len == (size_t)-1 || len == (size_t)-2 || len == 0) {
        *wc = (unsigned char)*s;
        return 1;
    }
    return len;
}

/* Is the period at string leading, so that only a period may match it */
static int
leading_period(const char *string, const char *stringstart, int flags)
{
    return *string == '.' && (flags & FNM_PERIOD)
        && (string == stringstart || ((flags & FNM_PATHNAME) && *(string - 1) == '/'));
}

/*
 * Match without recursion: each element after a '*' matches exactly
 * one character, so when the rest fails to match, only the most recent
 * '*' needs to try a longer string. Earlier ones can't do better.
 */
int
fnmatch(const char *pattern, const char *string, int flags)
{
    const char *stringstart = string;
    const char *star_pattern = NULL;
    const char *star_string = NULL;
    char       *newp;
    char        c;
    wchar_t     pc, sc;
    size_t      len;

    for (;;) {
        pattern += nextchar(pattern, &pc);
        switch (pc) {
        case EOS:
            if ((flags & FNM_LEADING_DIR) && *string == '/')
                return (0);
            if (*string == EOS)
                return (0);
            goto backtrack;
        case '?':
            if (*string == EOS)
                return (FNM_NOMATCH);
            if (*string == '/' && (flags & FNM_PATHNAME))
                goto backtrack;
            if (leading_period(string, stringstart, flags))
                goto backtrack;
            string += nextchar(string, &sc);
            break;
        case '*':
            c = *pattern;
            /* Collapse multiple stars. */
            while (c == '*')
                c = *++pattern;

            if (leading_period(string, stringstart, flags))
                goto backtrack;

            /* Optimize for pattern with * at end or before /. */
            if (c == EOS) {
                if (flags & FNM_PATHNAME)
                    return ((flags & FNM_LEADING_DIR) || strchr(string, '/') == NULL ? 0
                                                                                     : FNM_NOMATCH);
                return (0);
            }
            if (c == '/' && flags & FNM_PATHNAME) {
                if ((string = strchr(string, '/')) == NULL)
                    return (FNM_NOMATCH);
                break;
            }

            /* Start by matching the empty string */
            star_pattern = pattern;
            star_string = string;
            break;
        case '[':
            if (*string == EOS)
                return (FNM_NOMATCH);
            if (*string == '/' && (flags & FNM_PATHNAME))
                goto backtrack;
            if (leading_period(string, stringstart, flags))
                goto backtrack;

            len = nextchar(string, &sc);
            switch (rangematch(pattern, sc, flags, &newp)) {
            case RANGE_ERROR:
                goto norm;
            case RANGE_MATCH:
                pattern = newp;
                break;
            case RANGE_NOMATCH:
                goto backtrack;
            }
            string += len;
            break;
        case '\\':
            if (!(flags & FNM_NOESCAPE)) {
                len = nextchar(pattern, &pc);
                if (len == 0)
                    pc = '\\';
                pattern += len;
            }
            __fallthrough;
        default:
        norm:
            len = nextchar(string, &sc);
            if (len == 0)
                goto backtrack;
            if (pc == sc)
                ;
            else if ((flags & FNM_CASEFOLD) && towlower((wint_t)pc) == towlower((wint_t)sc))
                ;
            else
                goto backtrack;
            string += len;
            break;
        }
        continue;

    backtrack:
        /* Let the most recent '*' match one more character */
        if (star_pattern == NULL || *star_string == EOS)
            return (FNM_NOMATCH);
        if (*star_string == '/' && (flags & FNM_PATHNAME))
            return (FNM_NOMATCH);
        pattern = star_pattern;
        star_string += nextchar(star_string, &sc);
        string = star_string;
    }
}

/*
 * Fetch one character of a bracket expression: an ordinary or escaped
 * character, a collating symbol or an equivalence class. All collating
 * elements are single characters here.
 */
static int
bracketchar(const char **patternp, int flags, wchar_t *cp)
{
    const char *pattern = *patternp;
    wchar_t     c, sym;
    size_t      len;

    pattern += nextchar(pattern, &c);
    if (c == '\\' && !(flags & FNM_NOESCAPE))
        pattern += nextchar(pattern, &c);
    if (c == EOS)
        return (RANGE_ERROR);

    /* Equivalence classes -- [=class=] */
    if (c == '[' && pattern[0] == '=' && pattern[1] != EOS) {
        len = nextchar(pattern + 1, &sym);
        if (pattern[1 + len] == '=' && pattern[2 + len] == ']') {
            c = sym;
            pattern += 1 + len + 2;
        }
    }

    /* Collating sequences -- [.symbol.] */
    if (c == '[' && pattern[0] == '.' && pattern[1] != EOS) {
        len = nextchar(pattern + 1, &sym);
        if (pattern[1 + len] == '.' && pattern[2 + len] == ']') {
            /* Extract collating symbol, skipping [. and .] */
            c = sym;
            pattern += 1 + len + 2;
        } else if (strstr(pattern + 1, ".]") != NULL) {
            /* We have no collating symbols longer than 1 char */
            return (RANGE_ERROR);
        }
    }

    *cp = c;
    *patternp = pattern;
    return (RANGE_MATCH);
}

static int
rangematch(const char *pattern, wchar_t test, int flags, char **newp)
{
    int     negate, ok, first;
    wchar_t c, c2;
    wchar_t tests[3];
    int     ntests = 1, t;

    /*
     * A bracket expression starting with an unquoted circumflex
     * character produces unspecified results (IEEE 1003.2-1992,
     * 3.13.2).  This implementation treats it like '!', for
     * consistency with the regular expression syntax.
     * J.T. Conklin (conklin@ngai.kaleida.com)
     */
    if ((negate = (*pattern == '!' || *pattern == '^')))
        ++pattern;

    /*
     * Ignoring case, the character matches when either of its cases
     * would (XBD 4.1)
     */
    tests[0] = test;
    if (flags & FNM_CASEFOLD) {
        tests[ntests++] = (wchar_t)towlower((wint_t)test);
        tests[ntests++] = (wchar_t)towupper((wint_t)test);
    }

    /*
     * A right bracket shall lose its special meaning and represent
     * itself in a bracket expression if it occurs first in the list.
     * -- POSIX.2 2.8.3.2
     */
    ok = 0;
    for (first = 1; first || *pattern != ']'; first = 0) {
        /* Character class -- [:class:] */
        if (pattern[0] == '[' && pattern[1] == ':') {
            size_t name_len = 0;

            while (islower((unsigned char)pattern[2 + name_len]))
                name_len++;

            if (pattern[2 + name_len] == ':' && pattern[2 + name_len + 1] == ']') {
                char     name[16];
                wctype_t class = 0;

                /* Found a valid class pattern, now see if it's one we know */
                if (name_len < sizeof(name)) {
                    memcpy(name, &pattern[2], name_len);
                    name[name_len] = '\0';
                    class = wctype(name);
                }
                if (class != 0) {
                    for (t = 0; t < ntests; t++)
                        if (iswctype((wint_t)tests[t], class))
                            ok = 1;
                }

                /* Skip over the pattern and continue evaluating the expression */
                pattern = pattern + 2 + name_len + 2;
                continue;
            }
        }

        if (bracketchar(&pattern, flags, &c) == RANGE_ERROR)
            return (RANGE_ERROR);

        /* The end of a range may also be a collating symbol */
        if (pattern[0] == '-' && pattern[1] != EOS && pattern[1] != ']') {
            pattern++;
            if (bracketchar(&pattern, flags, &c2) == RANGE_ERROR)
                return (RANGE_ERROR);

            for (t = 0; t < ntests; t++)
                if (
#ifdef __HAVE_REAL_STRCOLL
                    (c < 0x100 && c2 < 0x100 && tests[t] < 0x100)
                        ? (__collate_range_cmp((unsigned char)c, (unsigned char)tests[t]) <= 0
                           && __collate_range_cmp((unsigned char)tests[t], (unsigned char)c2) <= 0)
                        :
#endif
                        (c <= tests[t] && tests[t] <= c2))
                    ok = 1;
        } else {
            for (t = 0; t < ntests; t++)
                if (c == tests[t])
                    ok = 1;
        }
    }

    *newp = (char *)pattern + 1;
    return (ok == negate ? RANGE_NOMATCH : RANGE_MATCH);
}
