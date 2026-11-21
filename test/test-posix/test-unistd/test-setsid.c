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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* setsid makes a new session and process group led by the caller, unless one would clash */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    pid_t pid;
    int   status;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pid_t self = getpid();

        if (setsid() != self)
            _exit(1);
        if (getsid(0) != self || getpgid(0) != self || getpgrp() != self)
            _exit(2);
        /* A process group leader can't make another session */
        if (setsid() != (pid_t)-1 || errno != EPERM)
            _exit(3);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    check(WEXITSTATUS(status) == 0, "child checks");

    /* Nor can a process whose process ID names another process's group */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        pid_t self = getpid(), group = getpgrp(), member;
        int   fds[2];
        char  c;

        if (setpgid(0, 0) != 0 || pipe(fds) != 0)
            _exit(10);
        member = fork();
        if (member < 0)
            _exit(11);
        if (member == 0) {
            (void)close(fds[1]);
            (void)read(fds[0], &c, 1);
            _exit(0);
        }
        /* Move back to the old group; the member keeps this one alive */
        if (setpgid(0, group) != 0 || getpgrp() == self || getpgid(member) != self)
            _exit(12);
        errno = 0;
        if (setsid() != (pid_t)-1 || errno != EPERM)
            _exit(13);
        (void)close(fds[1]);
        if (waitpid(member, &status, 0) != member)
            _exit(14);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    check(WEXITSTATUS(status) == 0, "group named for the caller");

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
