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
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char               dir[] = "test-glob-XXXXXX";

static const char * const files[] = { "a.c", "b.c", "c.h", ".hidden.c", "sub/x.c", "st*r", "star" };
#define NFILES (sizeof(files) / sizeof(files[0]))

static void
cleanup(void)
{
    char   path[64];
    size_t i;

    for (i = 0; i < NFILES; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        (void)unlink(path);
    }
    snprintf(path, sizeof(path), "%s/sub", dir);
    (void)rmdir(path);
    (void)rmdir(dir);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Expand dir/pattern and compare with the expected names, each under dir */
static void
expands(const char *pattern, int flags, int want_ret, const char * const *want, size_t nwant)
{
    char   full[64];
    char   name[64];
    glob_t g;
    size_t i;
    int    ret;

    snprintf(full, sizeof(full), "%s/%s", dir, pattern);
    ret = glob(full, flags, NULL, &g);
    if (ret != want_ret)
        printf("%s: returned %d\n", full, ret);
    check(ret == want_ret, full);
    if (ret == GLOB_NOMATCH) {
        globfree(&g);
        return;
    }
    if (g.gl_pathc != nwant)
        printf("%s: %zu matches\n", full, g.gl_pathc);
    check(g.gl_pathc == nwant, full);
    for (i = 0; i < nwant; i++) {
        snprintf(name, sizeof(name), "%s/%s", dir, want[i]);
        if (strcmp(g.gl_pathv[i], name) != 0)
            printf("%s: got %s want %s\n", full, g.gl_pathv[i], name);
        check(strcmp(g.gl_pathv[i], name) == 0, full);
    }
    check(g.gl_pathv[nwant] == NULL, "null terminated");
    globfree(&g);
}

#define N(a) (a), (sizeof(a) / sizeof((a)[0]))

int
main(void)
{
    static const char * const c_files[] = { "a.c", "b.c" };
    static const char * const hidden[] = { ".hidden.c" };
    static const char * const deeper[] = { "sub/x.c" };
    static const char * const subdir[] = { "sub/" };
    static const char * const a_c[] = { "a.c" };
    static const char * const escaped[] = { "st*r" };
    static const char * const stars[] = { "st*r", "star" };
    char                      path[64];
    glob_t                    g;
    size_t                    i;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/sub", dir);
    check(mkdir(path, 0700) == 0, "mkdir");
    for (i = 0; i < NFILES; i++) {
        FILE *f;

        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        f = fopen(path, "w");
        check(f != NULL, path);
        fclose(f);
    }

    /* Sorted, and a leading period must be matched explicitly */
    expands("*.c", 0, 0, N(c_files));
    expands(".*.c", 0, 0, N(hidden));
    expands("[ab].c", 0, 0, N(c_files));
    expands("?.c", 0, 0, N(c_files));

    /* Patterns in more than one component, and trailing slashes */
    expands("*/*.c", 0, 0, N(deeper));
    expands("s*/", 0, 0, N(subdir));
    expands("su?", GLOB_MARK, 0, N(subdir));

    /* Literal components must exist */
    expands("a.c", 0, 0, N(a_c));
    expands("zz.c", 0, GLOB_NOMATCH, NULL, 0);
    expands("none*", 0, GLOB_NOMATCH, NULL, 0);

    /* Backslash escapes, unless GLOB_NOESCAPE */
    expands("st\\*r", 0, 0, N(escaped));
    expands("st*r", 0, 0, N(stars));
    expands("st\\*r", GLOB_NOESCAPE, GLOB_NOMATCH, NULL, 0);

    /* GLOB_NOCHECK returns the pattern */
    snprintf(path, sizeof(path), "%s/none*", dir);
    check(glob(path, GLOB_NOCHECK, NULL, &g) == 0, "GLOB_NOCHECK");
    check(g.gl_pathc == 1 && strcmp(g.gl_pathv[0], path) == 0, "GLOB_NOCHECK returns the pattern");
    globfree(&g);

    /* GLOB_DOOFFS reserves leading slots; GLOB_APPEND adds to the list */
    g.gl_offs = 2;
    snprintf(path, sizeof(path), "%s/*.c", dir);
    check(glob(path, GLOB_DOOFFS, NULL, &g) == 0, "GLOB_DOOFFS");
    check(g.gl_pathv[0] == NULL && g.gl_pathv[1] == NULL, "reserved slots");
    check(g.gl_pathc == 2, "GLOB_DOOFFS count");
    snprintf(path, sizeof(path), "%s/*.h", dir);
    check(glob(path, GLOB_DOOFFS | GLOB_APPEND, NULL, &g) == 0, "GLOB_APPEND");
    check(g.gl_pathc == 3, "GLOB_APPEND count");
    snprintf(path, sizeof(path), "%s/c.h", dir);
    check(strcmp(g.gl_pathv[4], path) == 0 && g.gl_pathv[5] == NULL, "appended entry");
    globfree(&g);

    cleanup();
    printf("success\n");
    return 0;
}
