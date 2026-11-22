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
#if TEST_POSIX_SPAWN && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_PIPE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawnattr_setpgroup puts the child in a new process group */

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

/* Starts a shell that waits for input on 'in' */
static int
start(pid_t *pid, const posix_spawnattr_t *attr, int in)
{
    posix_spawn_file_actions_t fa;
    char                      *argv[] = { "sh", "-c", "read x", NULL };
    int                        ret;

    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_adddup2(&fa, in, 0) == 0, "adddup2");
    ret = posix_spawn(pid, "/bin/sh", &fa, attr, argv, environ);
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    return ret;
}

int
main(void)
{
    posix_spawnattr_t attr;
    pid_t             pid[4], pgroup = -1;
    int               fds[2], status, ret, i, n = 3;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    /* Each child waits until the pipe closes */
    check(pipe(fds) == 0 && fcntl(fds[1], F_SETFD, FD_CLOEXEC) == 0, "pipe");

    /* Without SETPGROUP or SETSID, the child stays in the parent's group */
    check(start(&pid[0], NULL, fds[0]) == 0, "posix_spawn");
    check(getpgid(pid[0]) == getpgrp(), "parent's process group");

    check(posix_spawnattr_init(&attr) == 0, "init");
    check(posix_spawnattr_getpgroup(&attr, &pgroup) == 0 && pgroup == 0, "default");
    check(posix_spawnattr_setpgroup(&attr, 0) == 0, "setpgroup");
    check(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP) == 0, "setflags");
    check(start(&pid[1], &attr, fds[0]) == 0, "posix_spawn");
    check(getpgid(pid[1]) == pid[1], "own process group");

    /* A non-zero spawn-pgroup names the group to join */
    check(posix_spawnattr_setpgroup(&attr, pid[1]) == 0, "setpgroup");
    check(posix_spawnattr_getpgroup(&attr, &pgroup) == 0 && pgroup == pid[1], "getpgroup");
    check(start(&pid[2], &attr, fds[0]) == 0, "posix_spawn");
    check(getpgid(pid[2]) == pid[1], "joined process group");

    /*
     * SETSID makes the child lead a new session and process group; glibc
     * only defines POSIX_SPAWN_SETSID for _GNU_SOURCE
     */
#if defined(__PICOLIBC__) || defined(POSIX_SPAWN_SETSID)
    check(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID) == 0, "setflags");
    check(start(&pid[n], &attr, fds[0]) == 0, "posix_spawn");
    check(getsid(pid[n]) == pid[n] && getpgid(pid[n]) == pid[n], "new session");
    n++;
#endif

    check(close(fds[1]) == 0, "close");
    for (i = 0; i < n; i++)
        check(waitpid(pid[i], &status, 0) == pid[i], "waitpid");
    check(close(fds[0]) == 0, "close");

    /* A setpgid failure is reported */
    check(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP) == 0, "setflags");
    ret = posix_spawnattr_setpgroup(&attr, -1);
    if (ret == 0)
        ret = start(&pid[0], &attr, 0);
    check(ret == EINVAL, "EINVAL");
    check(posix_spawnattr_destroy(&attr) == 0, "destroy");
    (void)run;
    (void)exited;
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
