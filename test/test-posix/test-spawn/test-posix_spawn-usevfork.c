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

#define _GNU_SOURCE
#include <unistd.h>
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_SPAWN && TEST_POSIX_MULTI_PROCESS

#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 * POSIX_SPAWN_USEVFORK (glibc's, not POSIX) is accepted along with
 * the POSIX flags (coreutils' split and sort pass it, and gnulib
 * makes one up when <spawn.h> lacks it)
 */

int
main(void)
{
#ifdef POSIX_SPAWN_USEVFORK
    posix_spawnattr_t attr;
    char             *argv[] = { "sh", "-c", "exit 3", NULL };
    short             flags;
    pid_t             pid;
    int               status;

    if (posix_spawnattr_init(&attr) != 0
        || posix_spawnattr_setflags(&attr, POSIX_SPAWN_USEVFORK | POSIX_SPAWN_SETSIGDEF) != 0
        || posix_spawnattr_getflags(&attr, &flags) != 0
        || flags != (POSIX_SPAWN_USEVFORK | POSIX_SPAWN_SETSIGDEF)) {
        printf("POSIX_SPAWN_USEVFORK refused\n");
        return 1;
    }
    if (posix_spawn(&pid, "/bin/sh", NULL, &attr, argv, NULL) != 0
        || waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 3) {
        printf("posix_spawn with POSIX_SPAWN_USEVFORK\n");
        return 1;
    }
    posix_spawnattr_destroy(&attr);
    printf("success\n");
    return 0;
#else
    printf("no POSIX_SPAWN_USEVFORK, skipping\n");
    return 77;
#endif
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
