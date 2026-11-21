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
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

/*
 * As POSIX describes: fork, then execl(<shell>, "sh", "-c", "--",
 * command, NULL), but without the "--" (see below). SIGINT and SIGQUIT are ignored and SIGCHLD
 * blocked while waiting; the child gets the caller's dispositions and mask back before running the
 * shell.
 */
int
system(const char *command)
{
    struct sigaction ign = { .sa_handler = SIG_IGN };
    struct sigaction old_int, old_quit;
    sigset_t         chld, old_mask;
    char            *argv[4];
    pid_t            pid;
    int              status;
    int              err = 0;

    /* There is always a shell */
    if (command == NULL)
        return 1;

    sigemptyset(&ign.sa_mask);
    sigemptyset(&chld);
    sigaddset(&chld, SIGCHLD);

    (void)sigprocmask(SIG_BLOCK, &chld, &old_mask);
    (void)sigaction(SIGINT, &ign, &old_int);
    (void)sigaction(SIGQUIT, &ign, &old_quit);

    /*
     * FreeBSD's sh takes the argument after -c as the command, even
     * "--" or one starting with '-', so this leaves out the "--" POSIX
     * shows
     */
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = (char *)command;
    argv[3] = NULL;

    pid = fork();
    if (pid == 0) {
        (void)sigaction(SIGINT, &old_int, NULL);
        (void)sigaction(SIGQUIT, &old_quit, NULL);
        (void)sigprocmask(SIG_SETMASK, &old_mask, NULL);
        execve("/bin/sh", argv, environ);
        _exit(127);
    }

    if (pid < 0) {
        err = errno;
        status = -1;
    } else {
        /* Other signals may interrupt the wait; it ends with the child */
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                err = errno;
                status = -1;
                break;
            }
        }
    }

    (void)sigaction(SIGINT, &old_int, NULL);
    (void)sigaction(SIGQUIT, &old_quit, NULL);
    (void)sigprocmask(SIG_SETMASK, &old_mask, NULL);

    if (status == -1)
        errno = err;
    return status;
}
