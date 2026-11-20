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
/* <libgen.h> is an XSI header */
#define _XOPEN_SOURCE 800
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * <libgen.h> declares basename and dirname as functions; #undef of a
 * macro version still leaves the function.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char *(* const p_basename)(char *) = basename;
static char *(* const p_dirname)(char *) = dirname;

/* glibc 2.43 defines basename as __xpg_basename and declares nothing else */
#ifndef __GLIBC__
#undef basename
#undef dirname
static char *(* const p_basename_function)(char *) = basename;
static char *(* const p_dirname_function)(char *) = dirname;
#endif

int
main(void)
{
    char path[] = "/usr/lib";

    check(strcmp(p_basename(path), "lib") == 0, "basename");
    check(strcmp(p_dirname(path), "/usr") == 0, "dirname");
#ifndef __GLIBC__
    strcpy(path, "/usr/lib");
    check(strcmp(p_basename_function(path), "lib") == 0, "basename after #undef");
    check(strcmp(p_dirname_function(path), "/usr") == 0, "dirname after #undef");
    strcpy(path, "/usr/lib");
    check(strcmp(basename(path), "lib") == 0, "calling basename after #undef");
#endif
    printf("success\n");
    return 0;
}
