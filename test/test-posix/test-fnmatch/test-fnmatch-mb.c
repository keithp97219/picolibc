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

#define _GNU_SOURCE
#include <unistd.h>
#include "../test-flags.h"

#include <fnmatch.h>
#include <locale.h>
#include <stdio.h>

/*
 * fnmatch matches characters, not bytes: in a UTF-8 locale '?' and
 * bracket expressions take a whole multibyte character (gnulib's
 * fnmatch check, grep, diffutils)
 */

static int ret;

static void
expect(const char *pattern, const char *string, int flags, int want)
{
    int got = fnmatch(pattern, string, flags);

    if ((got == 0) != want) {
        printf("fnmatch(\"%s\", \"%s\", %#x) = %d, want %s\n", pattern, string, flags, got,
               want ? "match" : "no match");
        ret = 1;
    }
}

int
main(void)
{
    /* The C locale: bytes are characters */
    expect("x??y", "x\303\274y", 0, 1);
    expect("x?y", "x\303\274y", 0, 0);

    if (setlocale(LC_ALL, "C.UTF-8") == NULL) {
        printf("no C.UTF-8 locale, skipping the UTF-8 checks\n");
        return ret ? ret : 77;
    }
    expect("x?y", "x\303\274y", 0, 1);
    expect("[\303\274]", "\303\274", 0, 1);
    expect("[!\303\274]", "\303\274", 0, 0);
    expect("[!a]", "\303\274", 0, 1);
    expect("[[:alpha:]]", "\303\251", 0, 1);
    expect("[[:digit:]]", "\303\251", 0, 0);
    expect("*\303\274*", "gr\303\274n", 0, 1);
    expect("gr\303\274n", "gr\303\274n", 0, 1);
    expect("gr\303\234n", "gr\303\274n", FNM_CASEFOLD, 1);
    expect("gr[\303\234]n", "gr\303\274n", FNM_CASEFOLD, 1);
    expect("\\\303\274", "\303\274", 0, 1);
    /* A range by code point: e-acute is past z */
    expect("[a-z]", "\303\251", 0, 0);
    expect("[\303\240-\303\277]", "\303\251", 0, 1);

    if (ret == 0)
        printf("success\n");
    return ret;
}
