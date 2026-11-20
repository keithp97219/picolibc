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
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(_POSIX_SPAWN) && _POSIX_SPAWN > 0)                                            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)         \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0))

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawn_file_actions_addopen opens files in the child */

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
    char                       buf[8];
    int                        fd;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_addopen(&fa, 1, "out", O_WRONLY | O_CREAT | O_TRUNC, 0600) == 0,
          "addopen");
    check(exited(run("echo hello", &fa, NULL), 0), "child");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
    fd = open("out", O_RDONLY);
    check(fd >= 0 && read(fd, buf, 6) == 6 && memcmp(buf, "hello\n", 6) == 0, "output");
    check(close(fd) == 0, "close");

    /* A failed open is reported by posix_spawn */
    check(posix_spawn_file_actions_init(&fa) == 0, "init");
    check(posix_spawn_file_actions_addopen(&fa, 0, "missing", O_RDONLY, 0) == 0, "addopen");
    {
        char *argv[] = { "sh", "-c", "exit 0", NULL };
        pid_t pid;

        check(posix_spawn(&pid, "/bin/sh", &fa, NULL, argv, environ) == ENOENT, "ENOENT");
    }
    check(posix_spawn_file_actions_addopen(&fa, -1, "x", O_RDONLY, 0) == EBADF, "EBADF");
    check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");

    /*
     * Opening on each low descriptor in turn, one of them where posix_spawn
     * keeps its own, still reports an exec failure and leaves the file alone
     */
    for (fd = 3; fd < 8; fd++) {
        char *argv[] = { "x", NULL };
        pid_t pid;
        int   f;

        check(posix_spawn_file_actions_init(&fa) == 0, "init");
        check(posix_spawn_file_actions_addopen(&fa, fd, "low", O_WRONLY | O_CREAT | O_TRUNC, 0600)
                  == 0,
              "addopen low");
        check(posix_spawn(&pid, "/nonexistent/program", &fa, NULL, argv, environ) == ENOENT,
              "exec failure reported");
        check(posix_spawn_file_actions_destroy(&fa) == 0, "destroy");
        f = open("low", O_RDONLY);
        check(f >= 0 && read(f, buf, sizeof(buf)) == 0 && close(f) == 0, "file untouched");
    }
    check(unlink("low") == 0, "unlink low");
    check(unlink("out") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
