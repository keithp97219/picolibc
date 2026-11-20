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
#if !defined(__PICOLIBC__)                                                                  \
    || ((defined(_POSIX_SPAWN) && _POSIX_SPAWN > 0)                                         \
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

/* posix_spawnattr_setflags stores flags, and posix_spawnattr_getflags returns them */

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
    posix_spawnattr_t attr;
    short             flags = -1;

    check(posix_spawnattr_init(&attr) == 0, "init");
    check(posix_spawnattr_getflags(&attr, &flags) == 0 && flags == 0, "default");
    check(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK) == 0,
          "setflags");
    check(posix_spawnattr_getflags(&attr, &flags) == 0
              && flags == (POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK),
          "getflags");
#ifdef __PICOLIBC__
    check(posix_spawnattr_setflags(&attr, 0x4000) == EINVAL, "unknown flag");
#endif
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
