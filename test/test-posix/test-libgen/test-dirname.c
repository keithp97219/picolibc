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
/* dirname is an XSI interface */
#define _XOPEN_SOURCE 800
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * dirname returns the directory part, ignoring trailing slashes, without
 * resolving anything, and keeps ".." components.
 */

static int failures;

static void
expect(const char *path, const char *want, const char *or_want)
{
    char  buf[64];
    char *got;

    strcpy(buf, path);
    got = dirname(buf);
    if (strcmp(got, want) != 0 && (!or_want || strcmp(got, or_want) != 0)) {
        printf("dirname(\"%s\") = \"%s\", not \"%s\"\n", path, got, want);
        failures++;
    }
}

int
main(void)
{
    expect("/usr/lib", "/usr", NULL);
    expect("/usr/lib/", "/usr", NULL);
    expect("/usr/", "/", NULL);
    expect("usr", ".", NULL);
    expect("usr/", ".", NULL);
    expect("usr//", ".", NULL);
    expect("/", "/", NULL);
    expect("///", "/", NULL);
    expect(".", ".", NULL);
    expect("..", ".", NULL);
    expect("a/b/c", "a/b", NULL);
    /* ".." before the last component stays */
    expect("a/../b", "a/..", NULL);
    expect("../x", "..", NULL);
    /* Redundant slashes may or may not be removed */
    expect("usr//lib", "usr", "usr//");
    expect("/usr//lib//", "/usr", "/usr//");
    /* A leading "//" may stay */
    expect("//", "/", "//");
    expect("//usr", "/", "//");
    /* No pathname resolution: names that don't exist work the same */
    expect("no/such/dir.invalid/x", "no/such/dir.invalid", NULL);
    expect("", ".", NULL);
    if (strcmp(dirname(NULL), ".") != 0) {
        printf("dirname(NULL) isn't \".\"\n");
        failures++;
    }
    if (failures)
        return 1;
    printf("success\n");
    return 0;
}
