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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                      \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)             \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0))

#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char               dir[] = "test-glob-XXXXXX";

static const char * const files[] = { "a.c", "b.c", "c.h", ".hidden.c", "sub/x.c", "st*r", "star" };
#define NFILES (sizeof(files) / sizeof(files[0]))

/* More names, in their own directory: 'd' directories, 'l' links to "dir" */
static const struct {
    char        type;
    const char *name;
} more[] = {
    { 'd', "more"         },
    { 'f', "more/B"       },
    { 'f', "more/a"       },
    { 'f', "more/b"       },
    { 'd', "more/dir"     },
    { 'l', "more/link"    },
    { 'd', "more/a[b"     },
    { 'f', "more/a[b/c]d" },
    { 'f', "more/abd"     },
};
#define NMORE (sizeof(more) / sizeof(more[0]))

static void
cleanup(void)
{
    char   path[64];
    size_t i;

    for (i = 0; i < NFILES; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        (void)unlink(path);
    }
    for (i = NMORE; i-- > 0;) {
        snprintf(path, sizeof(path), "%s/%s", dir, more[i].name);
        if (more[i].type == 'd')
            (void)rmdir(path);
        else
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
    static const char * const sorted[]
        = { "more/B", "more/a", "more/a[b", "more/abd", "more/b", "more/dir", "more/link" };
    static const char * const marked[]
        = { "more/B", "more/a", "more/a[b/", "more/abd", "more/b", "more/dir/", "more/link/" };
    static const char * const slash[] = { "more/a[b/c]d" };
    static const char * const ab[] = { "more/a", "more/b" };
    static const char * const appended[] = { "more/b", "more/B", "more/a" };
    char                      pattern[64];
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
    for (i = 0; i < NMORE; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, more[i].name);
        if (more[i].type == 'd') {
            check(mkdir(path, 0700) == 0, path);
        } else if (more[i].type == 'l') {
            check(symlink("dir", path) == 0, path);
        } else {
            FILE *f = fopen(path, "w");

            check(f != NULL, path);
            fclose(f);
        }
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

    /* Sorted in the C locale collating sequence */
    expands("more/*", 0, 0, N(sorted));

    /* GLOB_MARK marks directories, including through symbolic links */
    expands("more/*", GLOB_MARK, 0, N(marked));
#ifdef __PICOLIBC__
    /* glibc returns "//" */
    check(glob("/", GLOB_MARK, NULL, &g) == 0, "GLOB_MARK /");
    check(g.gl_pathc == 1 && strcmp(g.gl_pathv[0], "/") == 0, "GLOB_MARK leaves / alone");
    globfree(&g);
#endif

    /* A bracket expression can't contain a slash; the '[' is then ordinary */
    expands("more/a[b/c]d", 0, 0, N(slash));

    /* GLOB_NOSORT returns the same names, in any order */
    snprintf(path, sizeof(path), "%s/more/[ab]", dir);
    check(glob(path, GLOB_NOSORT, NULL, &g) == 0, "GLOB_NOSORT");
    check(g.gl_pathc == 2 && g.gl_pathv[2] == NULL, "GLOB_NOSORT count");
    check(strcmp(g.gl_pathv[0], g.gl_pathv[1]) != 0, "GLOB_NOSORT distinct");
    for (i = 0; i < 2; i++) {
        size_t len = strlen(g.gl_pathv[i]);

        check(len > 0 && (g.gl_pathv[i][len - 1] == 'a' || g.gl_pathv[i][len - 1] == 'b'),
              "GLOB_NOSORT names");
    }
    globfree(&g);

    /* GLOB_NOCHECK returns the pattern unchanged, only when nothing matches */
    snprintf(path, sizeof(path), "%s/more/x\\*", dir);
    check(glob(path, GLOB_NOCHECK, NULL, &g) == 0, "GLOB_NOCHECK escaped");
    check(g.gl_pathc == 1 && strcmp(g.gl_pathv[0], path) == 0 && g.gl_pathv[1] == NULL,
          "GLOB_NOCHECK keeps the backslash");
    globfree(&g);
    expands("more/[ab]", GLOB_NOCHECK, 0, N(ab));
    expands("more/b*", GLOB_NOCHECK, 0, appended, 1);

    /* GLOB_DOOFFS with GLOB_NOCHECK, as when building an argument list */
    g.gl_offs = 1;
    snprintf(path, sizeof(path), "%s/none*", dir);
    check(glob(path, GLOB_DOOFFS | GLOB_NOCHECK, NULL, &g) == 0, "GLOB_DOOFFS | GLOB_NOCHECK");
    check(g.gl_pathc == 1 && g.gl_pathv[0] == NULL, "GLOB_DOOFFS | GLOB_NOCHECK slot");
    check(strcmp(g.gl_pathv[1], path) == 0 && g.gl_pathv[2] == NULL,
          "GLOB_DOOFFS | GLOB_NOCHECK pattern");
    globfree(&g);

    /* GLOB_APPEND keeps the earlier names first, then sorts the new ones */
    snprintf(path, sizeof(path), "%s/more/b", dir);
    check(glob(path, 0, NULL, &g) == 0, "before GLOB_APPEND");
    snprintf(pattern, sizeof(pattern), "%s/more/[aB]", dir);
    check(glob(pattern, GLOB_APPEND, NULL, &g) == 0, "GLOB_APPEND");
    check(g.gl_pathc == 3 && g.gl_pathv[3] == NULL, "GLOB_APPEND count");
    for (i = 0; i < 3; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, appended[i]);
        check(strcmp(g.gl_pathv[i], path) == 0, "GLOB_APPEND order");
    }

    /* globfree leaves errno alone */
    errno = EINTR;
    globfree(&g);
    check(errno == EINTR, "globfree errno");

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
