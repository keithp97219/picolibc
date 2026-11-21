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
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <paths.h>

/* Runs path, or runs it with the shell when it isn't something the system can execute */
static int
exec_file(const char *path, char * const argv[], char * const env[])
{
    size_t argc, n;
    char **sh_argv;

    execve(path, argv, env);
    if (errno != ENOEXEC)
        return -1;
    for (argc = 0; argv[argc]; argc++)
        ;
    n = argc ? argc - 1 : 0;
    sh_argv = malloc((n + 3) * sizeof(char *));
    if (!sh_argv)
        return -1;
    sh_argv[0] = "sh";
    sh_argv[1] = (char *)path;
    memcpy(sh_argv + 2, argv + 1, n * sizeof(char *));
    sh_argv[n + 2] = NULL;
    execve(_PATH_BSHELL, sh_argv, env);
    free(sh_argv);
    errno = ENOEXEC;
    return -1;
}

int
execvpe(const char *file, char * const argv[], char * const env[])
{
    if (!*file) {
        errno = ENOENT;
        return -1;
    }
    if (strchr(file, '/'))
        return exec_file(file, argv, env);

    char        path[PATH_MAX + 1];
    char       *p, *colon;
    size_t      file_len = strlen(file);
    ptrdiff_t   dir_len;
    const char *exec_path;
    bool        denied = false;
    bool        too_long = false;

    /* Without PATH, search the directories confstr(_CS_PATH) names */
    p = getenv("PATH");
    if (!p)
        p = "/bin:/usr/bin";
    for (;;) {
        colon = strchrnul(p, ':');
        dir_len = colon - p;
        if (dir_len + file_len + 2 <= sizeof(path)) {
            if (dir_len > 0) {
                memcpy(path, p, dir_len);
                path[dir_len] = '/';
                memcpy(path + dir_len + 1, file, file_len);
                path[dir_len + 1 + file_len] = '\0';
                exec_path = path;
            } else {
                exec_path = file;
            }
            exec_file(exec_path, argv, env);
            /* Keep looking past files that aren't there or can't be run */
            switch (errno) {
            case EACCES:
                denied = true;
                break;
            case ENOENT:
            case ENOTDIR:
                break;
            default:
                return -1;
            }
        } else {
            too_long = true;
        }
        if (!*colon) {
            /* Report a name that was too long to try, rather than ENOENT */
            errno = denied ? EACCES : too_long ? ENAMETOOLONG : ENOENT;
            return -1;
        }
        p = colon + 1;
    }
}
