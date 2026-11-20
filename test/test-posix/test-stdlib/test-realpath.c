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
#define _DEFAULT_SOURCE /* glibc only has realpath for XSI or the default */
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS) && __PICOLIBC_POSIX_SYMBOLIC_LINKS > 0))

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char base[] = "test-realpath-XXXXXX";
static char top[PATH_MAX];

static void
cleanup(void)
{
    char                      path[PATH_MAX + 32];
    static const char * const names[] = { "loop", "abs", "rel", "d/f" };
    size_t                    i;

    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", base, names[i]);
        (void)unlink(path);
    }
    snprintf(path, sizeof(path), "%s/d", base);
    (void)rmdir(path);
    (void)rmdir(base);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Resolve base/name and compare with top/want */
static void
resolves(const char *name, const char *want)
{
    char path[PATH_MAX + 32];
    char expect[PATH_MAX + 32];
    char got[PATH_MAX];

    snprintf(path, sizeof(path), "%s/%s", base, name);
    snprintf(expect, sizeof(expect), "%s/%s", top, want);
    check(realpath(path, got) == got, path);
    if (strcmp(got, expect) != 0)
        printf("%s: got %s want %s\n", path, got, expect);
    check(strcmp(got, expect) == 0, path);
}

/* Resolving base/name fails with err */
static void
fails(const char *name, int err)
{
    char path[PATH_MAX + 32];
    char got[PATH_MAX];

    snprintf(path, sizeof(path), "%s/%s", base, name);
    errno = 0;
    check(realpath(path, got) == NULL && errno == err, path);
}

int
main(void)
{
    char  cwd[PATH_MAX];
    char  path[PATH_MAX + 32];
    char  got[PATH_MAX + 32];
    char *alloc;
    FILE *f;

    check(getcwd(cwd, sizeof(cwd)) != NULL, "getcwd");
    check(mkdtemp(base) != NULL, "mkdtemp");

    /* The current directory and the root */
    check(realpath(".", got) == got && strcmp(got, cwd) == 0, "dot is the current directory");
    check(realpath("/", got) == got && strcmp(got, "/") == 0, "the root");

    /* The test directory itself; cwd may contain links */
    check(realpath(base, top) == top, "resolve the test directory");

    /* A directory, a file, links to the directory and a loop */
    snprintf(path, sizeof(path), "%s/d", base);
    check(mkdir(path, 0700) == 0, "mkdir");
    snprintf(path, sizeof(path), "%s/d/f", base);
    f = fopen(path, "w");
    check(f != NULL, "create a file");
    fclose(f);
    snprintf(path, sizeof(path), "%s/rel", base);
    check(symlink("d", path) == 0, "relative link");
    snprintf(path, sizeof(path), "%s/abs", base);
    snprintf(got, sizeof(got), "%s/d", top);
    check(symlink(got, path) == 0, "absolute link");
    snprintf(path, sizeof(path), "%s/loop", base);
    check(symlink("loop", path) == 0, "looping link");

    resolves("d/f", "d/f");
    resolves("./d//./f", "d/f");
    resolves("d/../d/f", "d/f");
    resolves("rel/f", "d/f");
    resolves("abs/f", "d/f");
    resolves("rel/../rel/f", "d/f");
    resolves("rel", "d");

    fails("missing", ENOENT);
    fails("d/f/x", ENOTDIR);
    fails("d/f/", ENOTDIR);
    fails("loop", ELOOP);
    errno = 0;
    check(realpath("", got) == NULL && errno == ENOENT, "empty path");

    /* Without a buffer, the result is allocated */
    snprintf(path, sizeof(path), "%s/rel/f", base);
    alloc = realpath(path, NULL);
    check(alloc != NULL, "allocated result");
    snprintf(got, sizeof(got), "%s/d/f", top);
    check(strcmp(alloc, got) == 0, "allocated result value");
    free(alloc);

    cleanup();
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
