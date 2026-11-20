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

/* POSIX.1-2024 provides the case-insensitive flags in strict mode */
#define _POSIX_C_SOURCE 202405L
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(FNM_CASEFOLD) && defined(FNM_IGNORECASE)

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define y(pattern, string, flags) (fnmatch((pattern), (string), (flags)) == 0)
#define n(pattern, string, flags) (fnmatch((pattern), (string), (flags)) == FNM_NOMATCH)

int
main(void)
{
    check(FNM_IGNORECASE == FNM_CASEFOLD, "FNM_IGNORECASE");
    check(y("aB*", "Abc", FNM_IGNORECASE), "FNM_IGNORECASE");

    /* A string matches if it would with either case of each character */
    check(y("aBc", "AbC", FNM_CASEFOLD) && n("aBc", "AbC", 0), "ordinary");
    check(n("abc", "abd", FNM_CASEFOLD), "mismatch");
    check(y("*.C", "x.c", FNM_CASEFOLD) && y("?B", "ab", FNM_CASEFOLD), "wildcards");
    check(y("\\A", "a", FNM_CASEFOLD), "escaped");
    check(y(".X", ".x", FNM_CASEFOLD | FNM_PERIOD), "period");

    /* Including in bracket expressions */
    check(y("[a-c]x", "BX", FNM_CASEFOLD) && y("[A-C]", "b", FNM_CASEFOLD), "range");
    check(n("[A-C]", "d", FNM_CASEFOLD), "range mismatch");
    check(y("[B]", "b", FNM_CASEFOLD) && n("[B]", "c", FNM_CASEFOLD), "list");
    check(n("[!a]", "A", FNM_CASEFOLD) && y("[!a]", "B", FNM_CASEFOLD), "non-matching list");
    check(n("[[:upper:]]", "1", FNM_CASEFOLD), "class mismatch");

    /*
     * Character classes and ranges match the other case too: 'A' is
     * in [[:upper:]] and [Z-a], so 'a' and 'z' are when ignoring
     * case. (glibc doesn't fold the class or the range)
     */
    check(y("[[:upper:]]", "A", FNM_CASEFOLD) && y("[[:upper:]]", "a", FNM_CASEFOLD),
          "upper class");
    check(y("[[:lower:]]", "A", FNM_CASEFOLD) && y("[[:lower:]]", "a", FNM_CASEFOLD),
          "lower class");
    check(y("[Z-a]", "z", FNM_CASEFOLD) && y("[Z-a]", "A", FNM_CASEFOLD), "range of both cases");
    check(y("[Z-a]", "_", FNM_CASEFOLD) && n("[Z-a]", "b", FNM_CASEFOLD), "range between cases");
    check(n("[![:upper:]]", "a", FNM_CASEFOLD), "non-matching class");
    printf("success\n");
    return 0;
}

#elif defined(__PICOLIBC__)
#error FNM_CASEFOLD and FNM_IGNORECASE missing
#else

int
main(void)
{
    /* glibc offers FNM_CASEFOLD only to GNU code, and lacks FNM_IGNORECASE */
    printf("no FNM_CASEFOLD, skipping\n");
    return 77;
}

#endif
