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

/* Without the system support this test needs, it is skipped */
#if _POSIX_SPAWN > 0 && defined(__PICOLIBC_POSIX_FD_MGMT) && defined(__PICOLIBC_POSIX_FILE_SYSTEM) \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawn_file_actions_adddup2 duplicates descriptors in the child */

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

int
main(void)
{
    posix_spawn_file_actions_t fa;
    char                       buf[8];
    char                      *argv[] = { "sh", "-c", "exit 0", NULL };
    pid_t                      pid;
    long                       open_max;
    int                        fds[2];

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(pipe(fds) == 0, "pipe");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_adddup2(&fa, fds[1], 1) == 0, "adddup2");
    check(exited(run("echo dup", &fa, NULL), 0), "child");
    check(read(fds[0], buf, 4) == 4 && memcmp(buf, "dup\n", 4) == 0, "output");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");

    /* Duplicating a descriptor onto itself clears FD_CLOEXEC */
    check(fcntl(fds[1], F_SETFD, FD_CLOEXEC) == 0, "FD_CLOEXEC");
    check(dup2(fds[1], 8) == 8 && fcntl(8, F_SETFD, FD_CLOEXEC) == 0, "fd 8");
    check(!exited(run("echo x >&8", NULL, NULL), 0), "closed on exec");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_adddup2(&fa, 8, 8) == 0, "adddup2 same");
    check(exited(run("echo kept >&8", &fa, NULL), 0), "kept open");
    check(read(fds[0], buf, 5) == 5 && memcmp(buf, "kept\n", 5) == 0, "output");
    check(posix_spawn_file_actions_adddup2(&fa, -1, 3) == EBADF, "EBADF");
    check(posix_spawn_file_actions_adddup2(&fa, 3, -1) == EBADF, "EBADF new");
    open_max = sysconf(_SC_OPEN_MAX);
    if (open_max > 0 && open_max < 0x7fffffff) {
        check(posix_spawn_file_actions_adddup2(&fa, (int)open_max, 3) == EBADF, "EBADF max");
        check(posix_spawn_file_actions_adddup2(&fa, 3, (int)open_max) == EBADF, "EBADF new max");
    }
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");

    /* Duplicating a closed descriptor fails in posix_spawn */
    check(fcntl(20, F_GETFD) < 0, "fd 20 closed");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_adddup2(&fa, 20, 21) == 0, "adddup2 closed");
    check(posix_spawn(&pid, "/bin/sh", &fa, NULL, argv, environ) == EBADF, "dup2 EBADF");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    check(close(8) == 0 && close(fds[0]) == 0 && close(fds[1]) == 0, "close");
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
