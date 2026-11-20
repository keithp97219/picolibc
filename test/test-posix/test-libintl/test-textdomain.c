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
#include <errno.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char *dir;

    /* The default domain is "messages"; NULL just reports it */
    check(strcmp(textdomain(NULL), "messages") == 0, "default domain");
    check(strcmp(textdomain("picolibc-test"), "picolibc-test") == 0, "set the domain");
    check(strcmp(textdomain(NULL), "picolibc-test") == 0, "query the domain");
    check(strcmp(textdomain(""), "messages") == 0, "an empty name restores the default");

    /* Binding a directory, replacing it, and asking for it */
    check(strcmp(bindtextdomain("picolibc-test", "/a"), "/a") == 0, "bind a directory");
    check(strcmp(bindtextdomain("picolibc-test", "/b"), "/b") == 0, "rebind it");
    check(strcmp(bindtextdomain("picolibc-test", NULL), "/b") == 0, "query the binding");

    /* An unbound domain reports some default directory */
    dir = bindtextdomain("picolibc-unbound", NULL);
    check(dir != NULL && *dir != '\0', "default directory");

    /* No domain: nothing changes and errno is untouched */
    errno = 1234;
    check(bindtextdomain(NULL, "/c") == NULL, "null domain");
    check(bindtextdomain("", "/c") == NULL, "empty domain");
    check(bind_textdomain_codeset(NULL, "UTF-8") == NULL, "codeset for a null domain");
    check(errno == 1234, "errno unchanged");

    /* Codesets */
    check(bind_textdomain_codeset("picolibc-test", NULL) == NULL, "no codeset bound");
    check(strcmp(bind_textdomain_codeset("picolibc-test", "UTF-8"), "UTF-8") == 0,
          "bind a codeset");
    check(strcmp(bind_textdomain_codeset("picolibc-test", NULL), "UTF-8") == 0,
          "query the codeset");
    check(strcmp(bindtextdomain("picolibc-test", NULL), "/b") == 0, "directory kept");

    printf("success\n");
    return 0;
}
