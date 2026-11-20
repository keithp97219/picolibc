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

#ifndef _LOCAL_SPAWN_H_
#define _LOCAL_SPAWN_H_

#define _DEFAULT_SOURCE
#include <spawn.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>

struct __posix_spawnattr {
    short              flags;
    pid_t              pgroup;
    int                policy;
    struct sched_param param;
    sigset_t           sigdefault;
    sigset_t           sigmask;
};

enum __spawn_action_kind {
    __SPAWN_OPEN,
    __SPAWN_CLOSE,
    __SPAWN_DUP2,
    __SPAWN_CHDIR,
    __SPAWN_FCHDIR,
};

struct __spawn_action {
    enum __spawn_action_kind kind;
    int                      fd;
    int                      newfd;
    int                      oflag;
    mode_t                   mode;
    char                    *path;
};

struct __posix_spawn_file_actions {
    size_t                 count;
    size_t                 size;
    struct __spawn_action *actions;
};

/* Appends an action, copying 'path'; returns 0 or an error number */
int __spawn_add_action(posix_spawn_file_actions_t *fa, const struct __spawn_action *action);

/* Common code for posix_spawn and posix_spawnp */
int __posix_spawn(pid_t * __restrict pid, const char * __restrict path,
                  const posix_spawn_file_actions_t *fa, const posix_spawnattr_t * __restrict attr,
                  char * const argv[], char * const envp[], int search);

#endif /* _LOCAL_SPAWN_H_ */
