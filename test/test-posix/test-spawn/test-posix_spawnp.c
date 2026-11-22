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
#if TEST_POSIX_SPAWN && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawnp searches PATH for the program */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

extern char **environ;

/* Runs 'script' with /bin/sh and returns its wait status */
static int
run(const char *script, const posix_spawn_file_actions_t *fa, const posix_spawnattr_t *attr)
{
    char *argv[] = { "sh", "-c", (char *)script, NULL };
    pid_t pid;
    int   status, ret;

    ret = posix_spawn(&pid, "/bin/sh", fa, attr, argv, environ);
    check(ret == 0, "posix_spawn");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

static int
exited(int status, int code)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

static char dir[] = "tmp-XXXXXX";

/* Creates 'dir/name' holding 'contents', executable */
static void
make_prog(const char *name, const char *contents)
{
    char path[64];
    int  fd;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    check(fd >= 0, "create");
    check(write(fd, contents, strlen(contents)) == (ssize_t)strlen(contents), "write");
    check(close(fd) == 0, "close");
}

int
main(void)
{
    char *argv[] = { "sh", "-c", "exit 4", NULL };
    char  path[64];
    pid_t pid;
    int   status;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(setenv("PATH", "/nonexistent:/bin", 1) == 0, "setenv");
    check(posix_spawnp(&pid, "sh", NULL, NULL, argv, environ) == 0, "posix_spawnp");
    check(waitpid(pid, &status, 0) == pid && exited(status, 4), "status");
    check(posix_spawnp(&pid, "no-such-program-here", NULL, NULL, argv, environ) == ENOENT,
          "ENOENT");
    /* An empty name names no file */
    check(posix_spawnp(&pid, "", NULL, NULL, argv, environ) == ENOENT, "empty name");

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/a", dir);
    check(mkdir(path, 0700) == 0, "mkdir a");
    snprintf(path, sizeof(path), "%s/b", dir);
    check(mkdir(path, 0700) == 0, "mkdir b");
    make_prog("a/prog", "exit 6\n");
    make_prog("b/prog", "#!/bin/sh\nexit 5\n");

    /* A name with a slash is used as the pathname, without searching */
    snprintf(path, sizeof(path), "%s/b", dir);
    check(setenv("PATH", path, 1) == 0, "setenv");
    check(posix_spawnp(&pid, "prog", NULL, NULL, argv, environ) == 0, "search");
    check(waitpid(pid, &status, 0) == pid && exited(status, 5), "found by search");
    snprintf(path, sizeof(path), "%s/b/prog", dir);
    check(posix_spawnp(&pid, path, NULL, NULL, argv, environ) == 0, "slash");
    check(waitpid(pid, &status, 0) == pid && exited(status, 5), "slash status");
    check(posix_spawnp(&pid, "b/prog", NULL, NULL, argv, environ) == ENOENT, "slash not searched");

    /* A file that isn't executable fails, rather than running sh or searching on */
    snprintf(path, sizeof(path), "%s/a:%s/b", dir, dir);
    check(setenv("PATH", path, 1) == 0, "setenv");
    check(posix_spawnp(&pid, "prog", NULL, NULL, argv, environ) == ENOEXEC, "ENOEXEC");

    snprintf(path, sizeof(path), "%s/a/prog", dir);
    check(unlink(path) == 0, "unlink a/prog");
    snprintf(path, sizeof(path), "%s/b/prog", dir);
    check(unlink(path) == 0, "unlink b/prog");
    snprintf(path, sizeof(path), "%s/a", dir);
    check(rmdir(path) == 0, "rmdir a");
    snprintf(path, sizeof(path), "%s/b", dir);
    check(rmdir(path) == 0 && rmdir(dir) == 0, "cleanup");
    (void)run;
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
