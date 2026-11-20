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
/* basename is an XSI interface */
#define _XOPEN_SOURCE 800
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* basename returns the last component without trailing slashes */

static int failures;

static void
expect(const char *path, const char *want, const char *or_want)
{
    char  buf[64];
    char *got;

    strcpy(buf, path);
    got = basename(buf);
    if (strcmp(got, want) != 0 && (!or_want || strcmp(got, or_want) != 0)) {
        printf("basename(\"%s\") = \"%s\", not \"%s\"\n", path, got, want);
        failures++;
    }
}

int
main(void)
{
    char empty[1] = "";

    expect("/usr/lib", "lib", NULL);
    expect("/usr/", "usr", NULL);
    expect("usr", "usr", NULL);
    expect("usr//", "usr", NULL);
    expect("//usr//lib//", "lib", NULL);
    expect("a/b/c", "c", NULL);
    expect(".", ".", NULL);
    expect("..", "..", NULL);
    expect("a/..", "..", NULL);
    expect("/", "/", NULL);
    expect("///", "/", NULL);
    expect("////", "/", NULL);
    /* Exactly two slashes may give either */
    expect("//", "/", "//");
    expect("", ".", NULL);
    if (strcmp(basename(empty), ".") != 0) {
        printf("basename of an empty string isn't \".\"\n");
        failures++;
    }
    if (strcmp(basename(NULL), ".") != 0) {
        printf("basename(NULL) isn't \".\"\n");
        failures++;
    }
    if (failures)
        return 1;
    printf("success\n");
    return 0;
}
