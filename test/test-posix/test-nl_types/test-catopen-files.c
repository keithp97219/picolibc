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
#include "../posix-groups.h"

#include <errno.h>
#include <nl_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__PICOLIBC_POSIX_FILE_SYSTEM)
#include <fcntl.h>
#include <sys/stat.h>
#endif
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)
#include <sys/wait.h>
#endif

/*
 * <nl_types.h>'s types, constants and prototypes, and catopen's
 * failures: a missing catalog, an empty name, and files that aren't
 * message catalogs, found by pathname or through NLSPATH. picolibc
 * has no catalog format, so every file is invalid.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static nl_catd   catd_value __attribute__((unused));
static nl_item   item_value __attribute__((unused));
static const int setd = NL_SETD;
static const int cat_locale = NL_CAT_LOCALE;

/* Open a catalog that must not exist; return errno */
static int
open_fails(const char *name, int oflag)
{
    nl_catd catd;

    errno = 0;
    catd = catopen(name, oflag);
    if (catd != (nl_catd)-1) {
        printf("catopen(\"%s\") succeeded\n", name);
        catclose(catd);
        exit(1);
    }
    return errno;
}

int
main(void)
{
    int     (*p_catclose)(nl_catd) = catclose;
    char   *(*p_catgets)(nl_catd, int, int, const char *) = catgets;
    nl_catd (*p_catopen)(const char *, int) = catopen;

    check(p_catclose && p_catgets && p_catopen, "prototypes");
    check(cat_locale != 0, "NL_CAT_LOCALE differs from 0");
    (void)setd;

    /* A catalog that doesn't exist, by pathname */
    check(open_fails("/nonexistent-picolibc-test/no-such-catalog", 0) == ENOENT, "missing path");
    check(open_fails("/nonexistent-picolibc-test/no-such-catalog", NL_CAT_LOCALE) == ENOENT,
          "missing path, NL_CAT_LOCALE");
    check(open_fails("", 0) == ENOENT, "empty name");

#if defined(__PICOLIBC_POSIX_FILE_SYSTEM)
    {
        char        dir[] = "tmp-catopen-XXXXXX";
        char        path[64];
        int         fd;
        static char junk[] = "this is not a message catalog\n";

        check(mkdtemp(dir) != NULL, "mkdtemp");
        snprintf(path, sizeof(path), "%s/junk.cat", dir);
        fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        check(fd >= 0, "create junk.cat");
        check(write(fd, junk, sizeof(junk) - 1) == (ssize_t)sizeof(junk) - 1, "write");
        check(close(fd) == 0, "close");

        /* An existing file in an invalid format is ENOENT */
#ifndef __GLIBC__
        /* glibc 2.43 fails here without setting errno */
        check(open_fails(path, 0) == ENOENT, "file that isn't a catalog");
#else
        (void)open_fails(path, 0);
#endif

#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)
        {
            /* NLSPATH finds the file, which is ignored as invalid */
            pid_t pid;
            int   status;
            char  nlspath[96];

            snprintf(nlspath, sizeof(nlspath), "%s/%%N.cat", dir);
            fflush(stdout);
            pid = fork();
            check(pid >= 0, "fork");
            if (pid == 0) {
                check(setenv("NLSPATH", nlspath, 1) == 0, "setenv");
                check(setenv("LANG", "C", 1) == 0, "setenv");
#ifndef __GLIBC__
                check(open_fails("junk", 0) == ENOENT, "NLSPATH junk");
                check(open_fails("junk", NL_CAT_LOCALE) == ENOENT, "NLSPATH junk, NL_CAT_LOCALE");
#else
                (void)open_fails("junk", 0);
                (void)open_fails("junk", NL_CAT_LOCALE);
#endif
                _exit(0);
            }
            check(waitpid(pid, &status, 0) == pid, "waitpid");
            check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "NLSPATH child");
        }
#endif

        check(unlink(path) == 0, "unlink");
        check(rmdir(dir) == 0, "rmdir");
    }
#endif

    printf("success\n");
    return 0;
}
