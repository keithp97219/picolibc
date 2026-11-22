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
#if TEST_POSIX_CLOFORK && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FD_MGMT && TEST_POSIX_PIPE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Descriptors marked close-on-fork, by O_CLOFORK, FD_CLOFORK,
 * F_DUPFD_CLOFORK, dup3 or pipe2, are closed in the child of fork and
 * stay open in the parent
 */

_Static_assert(FD_CLOFORK != FD_CLOEXEC, "FD_CLOFORK is distinct from FD_CLOEXEC");
_Static_assert((O_CLOFORK & (O_CLOEXEC | O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC | O_ACCMODE)) == 0,
               "O_CLOFORK is distinct from the other flags");

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define NFD 6

int
main(void)
{
    int   fds[2], cfds[2], keep, fd[NFD], status, i;
    pid_t pid;

    check(pipe(fds) == 0, "pipe");
    keep = fds[0];

    fd[0] = open("/dev/null", O_RDONLY | O_CLOFORK);
    check(fd[0] >= 0, "open with O_CLOFORK");
    check(fcntl(fd[0], F_GETFD) & FD_CLOFORK, "O_CLOFORK sets FD_CLOFORK");
    check(!(fcntl(fd[0], F_GETFD) & FD_CLOEXEC), "O_CLOFORK leaves FD_CLOEXEC clear");

    fd[1] = dup(keep);
    check(fd[1] >= 0 && fcntl(fd[1], F_GETFD) == 0, "dup clears the descriptor flags");
    check(fcntl(fd[1], F_SETFD, FD_CLOFORK) == 0, "F_SETFD FD_CLOFORK");
    check(fcntl(fd[1], F_GETFD) == FD_CLOFORK, "F_GETFD reports FD_CLOFORK");

    fd[2] = fcntl(keep, F_DUPFD_CLOFORK, 0);
    check(fd[2] >= 0 && fcntl(fd[2], F_GETFD) == FD_CLOFORK, "F_DUPFD_CLOFORK");

    fd[3] = dup(keep);
    check(fd[3] >= 0, "dup");
    check(dup3(keep, fd[3], O_CLOFORK) == fd[3], "dup3 with O_CLOFORK");
    check(fcntl(fd[3], F_GETFD) == FD_CLOFORK, "dup3 sets FD_CLOFORK");

    check(pipe2(cfds, O_CLOFORK) == 0, "pipe2 with O_CLOFORK");
    fd[4] = cfds[0];
    fd[5] = cfds[1];
    check(fcntl(fd[4], F_GETFD) == FD_CLOFORK && fcntl(fd[5], F_GETFD) == FD_CLOFORK,
          "pipe2 sets FD_CLOFORK");

    /* The plain descriptors stay */
    check(fcntl(keep, F_GETFD) == 0, "pipe sets no descriptor flags");

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        /* Exit status bit i set when fd[i] is still open */
        int open_fds = 0;

        for (i = 0; i < NFD; i++) {
            errno = 0;
            if (fcntl(fd[i], F_GETFD) != -1 || errno != EBADF)
                open_fds |= 1 << i;
        }
        if (fcntl(keep, F_GETFD) == -1)
            open_fds |= 1 << NFD;
        _exit(open_fds);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    check(!(WEXITSTATUS(status) & (1 << NFD)), "the plain descriptor stays open in the child");
    for (i = 0; i < NFD; i++)
        if (WEXITSTATUS(status) & (1 << i)) {
            printf("descriptor %d stayed open in the child\n", i);
            exit(1);
        }

    /* The parent keeps them all */
    for (i = 0; i < NFD; i++)
        check(fcntl(fd[i], F_GETFD) == FD_CLOFORK, "the parent keeps its descriptors");

    /* Clearing the flag keeps the descriptor across fork */
    check(fcntl(fd[0], F_SETFD, 0) == 0 && fcntl(fd[0], F_GETFD) == 0, "clear FD_CLOFORK");

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
