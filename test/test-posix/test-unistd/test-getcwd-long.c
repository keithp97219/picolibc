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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_FD

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * getcwd in a directory whose name is longer than PATH_MAX and than
 * the kernels' own limits (1024 bytes for FreeBSD's __getcwd, 4096 for
 * Linux's getcwd), reached by changing directory one level at a time.
 * Checks the name at every level, checks the deepest name by walking
 * back down it one component at a time, and checks ERANGE, getcwd(NULL)
 * growing past PATH_MAX and EACCES from an ancestor that can't be read.
 * When the test runs on a separately mounted /tmp, the walk also
 * crosses a mount point.
 */

#define NAME_LEN   200
#define DEPTH_LEN  5000
#define BIG_BUFFER 16384

static char dir[] = "tmp-XXXXXX";
static char top[BIG_BUFFER];
static char expect[BIG_BUFFER];
static char big[BIG_BUFFER];
static char name[NAME_LEN + 1];

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* The directory name for one level: distinct, NAME_LEN bytes long */
static const char *
level_name(int level)
{
    snprintf(name, sizeof(name), "%04d", level);
    memset(name + 4, 'a' + level % 26, NAME_LEN - 4);
    name[NAME_LEN] = '\0';
    return name;
}

int
main(void)
{
    struct stat deep, st;
    size_t      len;
    char       *got, *copy, *comp, *save;
    int         level, levels, dfd;

    check(getcwd(top, sizeof(top)) != NULL && top[0] == '/', "getcwd");
    check(mkdtemp(dir) != NULL, "scratch directory");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open the scratch directory");
    check(chdir(dir) == 0, "chdir to the scratch directory");
    if (strcmp(top, "/") != 0)
        strcpy(expect, top);
    strcat(expect, "/");
    strcat(expect, dir);

    /* Down one level at a time, checking the name at each */
    for (levels = 0; strlen(expect) < DEPTH_LEN; levels++) {
        level_name(levels);
        check(mkdir(name, 0700) == 0, "mkdir");
        check(chdir(name) == 0, "chdir");
        strcat(expect, "/");
        strcat(expect, name);
        errno = 0;
        got = getcwd(big, sizeof(big));
        check(got == big, "getcwd");
        if (strcmp(big, expect) != 0) {
            printf("level %d: getcwd gave %s\n", levels, big);
            exit(1);
        }
    }

    /* A null buffer of size zero grows past PATH_MAX as needed */
    len = strlen(expect);
    got = getcwd(NULL, 0);
    check(got != NULL && strcmp(got, expect) == 0, "getcwd(NULL, 0)");
    free(got);
    got = getcwd(NULL, len + 1);
    check(got != NULL && strcmp(got, expect) == 0, "getcwd(NULL, exact)");
    free(got);
    errno = 0;
    check(getcwd(NULL, len) == NULL && errno == ERANGE, "getcwd(NULL, short)");

    /* The buffer size: an exact fit, else ERANGE */
    memset(big, 0, sizeof(big));
    check(getcwd(big, len + 1) == big && strcmp(big, expect) == 0, "an exact fit");
    errno = 0;
    check(getcwd(big, len) == NULL && errno == ERANGE, "one byte short");
    errno = 0;
    check(getcwd(big, PATH_MAX) == NULL && errno == ERANGE, "PATH_MAX bytes");

    /* Walk back down the name from the root to the same directory */
    check(stat(".", &deep) == 0, "stat the deepest directory");
    copy = strdup(expect);
    check(copy != NULL, "strdup");
    check(chdir("/") == 0, "chdir to the root");
    for (comp = strtok_r(copy, "/", &save); comp; comp = strtok_r(NULL, "/", &save))
        check(chdir(comp) == 0, "chdir down one component");
    free(copy);
    check(stat(".", &st) == 0 && st.st_dev == deep.st_dev && st.st_ino == deep.st_ino,
          "the name leads back to the same directory");

    /* An ancestor that can be searched but not read: EACCES */
    if (geteuid() != 0) {
        check(fchmod(dfd, 0100) == 0, "remove read permission");
        errno = 0;
        got = getcwd(big, sizeof(big));
        check(got == NULL && errno == EACCES, "EACCES from an unreadable ancestor");
        check(fchmod(dfd, 0700) == 0, "restore read permission");
        check(getcwd(big, sizeof(big)) == big && strcmp(big, expect) == 0,
              "getcwd after restoring permission");
    }

    /* Remove the directories on the way back up */
    for (level = levels - 1; level >= 0; level--) {
        check(chdir("..") == 0, "chdir up");
        check(rmdir(level_name(level)) == 0, "rmdir");
    }
    check(close(dfd) == 0, "close");
    check(chdir(top) == 0 && rmdir(dir) == 0, "remove the scratch directory");
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
