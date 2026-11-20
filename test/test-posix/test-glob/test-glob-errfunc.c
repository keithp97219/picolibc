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

static char dir[] = "test-glob-errfunc-XXXXXX";

static void
cleanup(void)
{
    char path[64];

    snprintf(path, sizeof(path), "%s/noread", dir);
    (void)chmod(path, 0700);
    snprintf(path, sizeof(path), "%s/noread/xb", dir);
    (void)unlink(path);
    snprintf(path, sizeof(path), "%s/noread", dir);
    (void)rmdir(path);
    snprintf(path, sizeof(path), "%s/ok/xa", dir);
    (void)unlink(path);
    snprintf(path, sizeof(path), "%s/ok", dir);
    (void)rmdir(path);
    snprintf(path, sizeof(path), "%s/loop", dir);
    (void)unlink(path);
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

static int calls, seen_eacces, seen_eloop, seen_enametoolong, stop;

/* Does epath name dir/name, with or without a trailing slash */
static int
names(const char *epath, const char *name)
{
    size_t dlen = strlen(dir), nlen = strlen(name);

    return strncmp(epath, dir, dlen) == 0 && epath[dlen] == '/'
        && strncmp(epath + dlen + 1, name, nlen) == 0
        && (epath[dlen + 1 + nlen] == '\0' || strcmp(epath + dlen + 1 + nlen, "/") == 0);
}

static int
errfunc(const char *epath, int eerrno)
{
    calls++;
    if (eerrno == EACCES && names(epath, "noread"))
        seen_eacces++;
    if (eerrno == ELOOP && names(epath, "loop"))
        seen_eloop++;
    if (eerrno == ENAMETOOLONG)
        seen_enametoolong++;
    return stop;
}

static void
reset(int ret)
{
    calls = seen_eacces = seen_eloop = seen_enametoolong = 0;
    stop = ret;
}

int
main(void)
{
    char   path[64];
    char   want[64];
    char  *longpat;
    glob_t g;
    size_t i, len;
    int    root = geteuid() == 0;
    FILE  *f;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/ok", dir);
    check(mkdir(path, 0700) == 0, path);
    snprintf(path, sizeof(path), "%s/ok/xa", dir);
    check((f = fopen(path, "w")) != NULL, path);
    fclose(f);
    snprintf(path, sizeof(path), "%s/noread", dir);
    check(mkdir(path, 0700) == 0, path);
    snprintf(path, sizeof(path), "%s/noread/xb", dir);
    check((f = fopen(path, "w")) != NULL, path);
    fclose(f);
    snprintf(path, sizeof(path), "%s/noread", dir);
    check(chmod(path, 0) == 0, "chmod");
    snprintf(path, sizeof(path), "%s/loop", dir);
    check(symlink("loop", path) == 0, "symlink");

    snprintf(path, sizeof(path), "%s/*/x*", dir);
    snprintf(want, sizeof(want), "%s/ok/xa", dir);

    /* Without GLOB_ERR or errfunc, errors are ignored */
    reset(0);
    check(glob(path, 0, NULL, &g) == 0, "ignored errors");
    check(g.gl_pathc == 1 && strcmp(g.gl_pathv[0], want) == 0 && g.gl_pathv[1] == NULL,
          "ignored errors match");
    globfree(&g);

    /* errfunc sees each directory that can't be read, and returning 0 goes on */
    check(glob(path, 0, errfunc, &g) == 0, "errfunc returning 0");
    check(g.gl_pathc == 1 && strcmp(g.gl_pathv[0], want) == 0 && g.gl_pathv[1] == NULL,
          "errfunc returning 0 match");
    globfree(&g);
    if (!root)
        check(seen_eacces == 1, "EACCES reported");
#ifndef __GLIBC__
    /* glibc skips the looping link without reporting it */
    check(seen_eloop == 1, "ELOOP reported");
#endif

    /* errfunc returning non-zero stops, with the names found so far */
    reset(1);
    check(glob(path, 0, errfunc, &g) == GLOB_ABORTED, "errfunc returning non-zero");
    check(calls == 1, "errfunc called once");
    check(g.gl_pathc <= 1 && (g.gl_pathc == 0 || g.gl_pathv[g.gl_pathc] == NULL),
          "aborted results");
    if (g.gl_pathc == 1)
        check(strcmp(g.gl_pathv[0], want) == 0, "aborted match");
    globfree(&g);

    /* GLOB_ERR stops, after calling errfunc */
    reset(0);
    check(glob(path, GLOB_ERR, errfunc, &g) == GLOB_ABORTED, "GLOB_ERR");
    check(calls == 1, "GLOB_ERR errfunc called");
    globfree(&g);
    check(glob(path, GLOB_ERR, NULL, &g) == GLOB_ABORTED, "GLOB_ERR without errfunc");
    globfree(&g);

    /* A symbolic link loop */
    snprintf(path, sizeof(path), "%s/loop/x*", dir);
    reset(0);
    check(glob(path, 0, errfunc, &g) == GLOB_NOMATCH, "loop");
    check(seen_eloop == 1, "loop ELOOP");
    globfree(&g);

    /* A component longer than NAME_MAX */
    len = strlen(dir) + 1 + 300 + 3;
    longpat = malloc(len);
    check(longpat != NULL, "malloc");
    snprintf(longpat, len, "%s/", dir);
    for (i = strlen(longpat); i < len - 3; i++)
        longpat[i] = 'a';
    strcpy(longpat + len - 3, "/*");
    reset(0);
    check(glob(longpat, GLOB_ERR, errfunc, &g) == GLOB_ABORTED, "long name");
    check(seen_enametoolong == 1, "long name ENAMETOOLONG");
    globfree(&g);
    free(longpat);

    /* A path longer than any system allows */
    len = strlen(dir) + 2 * 3000 + 3;
    longpat = malloc(len);
    check(longpat != NULL, "malloc");
    snprintf(longpat, len, "%s", dir);
    for (i = strlen(longpat); i < len - 3; i += 2)
        memcpy(longpat + i, "/.", 2);
    strcpy(longpat + i, "/*");
    reset(0);
    check(glob(longpat, GLOB_ERR, errfunc, &g) == GLOB_ABORTED, "long path");
    check(seen_enametoolong == 1, "long path ENAMETOOLONG");
    globfree(&g);
    free(longpat);

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
