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
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-scandir.txt"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifdef __PICOLIBC__
__typeof(opendir) __fake_opendir __weak;
#define fake_dirent() (opendir == __fake_opendir)
#else
#define fake_dirent() 0
#endif

#ifdef TESTS_ENABLE_POSIX_IO
/* Three files, created in reverse order, named after the test */
static const char *suffix[] = { "c", "b", "a" };
#define NFILES (sizeof(suffix) / sizeof(suffix[0]))

static char names[NFILES][sizeof(TEST_FILE_NAME) + 2];

static void
cleanup(void)
{
    size_t i;

    for (i = 0; i < NFILES; i++)
        (void)remove(names[i]);
}

static int
ours(const struct dirent *ent)
{
    return strncmp(ent->d_name, TEST_FILE_NAME ".", sizeof(TEST_FILE_NAME)) == 0;
}

#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)
static int selected;

static int
count_all(const struct dirent *ent)
{
    (void)ent;
    selected++;
    return 0;
}

static int
unordered(const struct dirent **a, const struct dirent **b)
{
    (void)a;
    (void)b;
    return 0;
}
#endif
#endif

int
main(void)
{
#ifndef TESTS_ENABLE_POSIX_IO
    printf("POSIX I/O not enabled, skipping\n");
    return 77;
#else
    struct dirent **list;
    size_t          i;
    int             n;

    if (fake_dirent()) {
        printf("fake dirent ops, skipping\n");
        return 77;
    }

    for (i = 0; i < NFILES; i++) {
        FILE *f;

        snprintf(names[i], sizeof(names[i]), "%s.%s", TEST_FILE_NAME, suffix[i]);
        f = fopen(names[i], "w");
        check(f != NULL, "create a file");
        fclose(f);
    }

    /* Select just our files and sort them by name */
    n = scandir(".", &list, ours, alphasort);
    check(n == (int)NFILES, "scandir finds our files");
    for (i = 0; i < NFILES; i++) {
        check(strcmp(list[i]->d_name, names[NFILES - 1 - i]) == 0, "sorted by name");
        free(list[i]);
    }
    free(list);

    /* With no selection function, every entry is returned */
    n = scandir(".", &list, NULL, NULL);
    check(n >= (int)NFILES, "scandir without selection");
    while (n > 0)
        free(list[--n]);
    free(list);

    /* A directory that doesn't exist fails */
    check(scandir(TEST_FILE_NAME ".missing", &list, NULL, NULL) == -1, "missing directory");

#if !defined(__PICOLIBC__)                                                         \
    || (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)
    /* With a file system that reports them, the errors */
    errno = 0;
    check(scandir(TEST_FILE_NAME ".missing", &list, NULL, NULL) == -1 && errno == ENOENT, "ENOENT");
    errno = 0;
    check(scandir("", &list, NULL, NULL) == -1 && errno == ENOENT, "ENOENT for an empty name");
    errno = 0;
    check(scandir(names[0], &list, NULL, NULL) == -1 && errno == ENOTDIR, "ENOTDIR");

    /* sel sees every entry, dot and dot-dot too, once */
    selected = 0;
    n = scandir(".", &list, count_all, NULL);
    check(n == 0 && selected >= (int)NFILES + 2, "sel sees every entry");
    free(list);

    /* compar need not order totally; all the entries still come back */
    n = scandir(".", &list, ours, unordered);
    check(n == (int)NFILES, "a comparison that isn't a total order");
    for (i = 0; i < NFILES; i++)
        check(ours(list[i]), "only the selected entries");
    while (n > 0)
        free(list[--n]);
    free(list);
#endif

    cleanup();
    printf("success\n");
    return 0;
#endif
}
