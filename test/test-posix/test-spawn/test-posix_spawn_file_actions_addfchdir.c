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
#if TEST_POSIX_SPAWN >= 202405L && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT \
    && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawn_file_actions_addfchdir changes directory in the child */

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

int
main(void)
{
    posix_spawn_file_actions_t fa;
    char                      *argv[] = { "sh", "-c", "exit 0", NULL };
    pid_t                      pid;
    int                        dfd, fd;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open dir");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_addfchdir(&fa, dfd) == 0, "addfchdir");
    check(exited(run("echo x > here", &fa, NULL), 0), "child");
    check(posix_spawn_file_actions_addfchdir(&fa, -1) == EBADF, "EBADF");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    check(close(dfd) == 0, "close");

    /* A failed fchdir is reported */
    check(fcntl(20, F_GETFD) < 0, "fd 20 closed");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_addfchdir(&fa, 20) == 0, "addfchdir closed");
    check(posix_spawn(&pid, "/bin/sh", &fa, NULL, argv, environ) == EBADF, "fchdir EBADF");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    fd = open("/bin/sh", O_RDONLY);
    check(fd >= 0, "open file");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_addfchdir(&fa, fd) == 0, "addfchdir file");
    check(posix_spawn(&pid, "/bin/sh", &fa, NULL, argv, environ) == ENOTDIR, "fchdir ENOTDIR");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    check(close(fd) == 0, "close file");

    check(chdir(dir) == 0 && access("here", F_OK) == 0, "created after fchdir");
    check(unlink("here") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
