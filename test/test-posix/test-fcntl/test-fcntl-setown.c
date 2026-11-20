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
    || ((defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)                 \
        && (defined(__PICOLIBC_POSIX_NETWORKING) && __PICOLIBC_POSIX_NETWORKING > 0)        \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0))

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>

/* F_SETOWN, F_GETOWN, F_SETOWN_EX and F_GETOWN_EX name a socket's SIGURG receiver */

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
    pid_t pid = getpid(), pgrp = getpgrp(), dead;
    int   fds[2], status;

    check(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");

    /* No owner at first */
    check(fcntl(fds[0], F_GETOWN) == 0, "no owner");

    /* A positive value is a process */
    check(fcntl(fds[0], F_SETOWN, pid) != -1, "F_SETOWN process");
    check(fcntl(fds[0], F_GETOWN) == pid, "F_GETOWN process");

    /* A negative value is a process group */
    check(fcntl(fds[0], F_SETOWN, -pgrp) != -1, "F_SETOWN process group");
    check(fcntl(fds[0], F_GETOWN) == -pgrp, "F_GETOWN process group");

    /* Zero removes the owner */
    check(fcntl(fds[0], F_SETOWN, 0) != -1 && fcntl(fds[0], F_GETOWN) == 0, "F_SETOWN 0");

    /* A process that doesn't exist: wait for a child so its ID is free */
    dead = fork();
    check(dead >= 0, "fork");
    if (dead == 0)
        _exit(0);
    check(waitpid(dead, &status, 0) == dead, "waitpid");
    errno = 0;
    check(fcntl(fds[0], F_SETOWN, dead) == -1 && errno == ESRCH, "F_SETOWN ESRCH");

#ifdef __PICOLIBC__
    /* glibc has F_GETOWN_EX and F_SETOWN_EX only for _GNU_SOURCE */
    struct f_owner_ex owner;

    owner.type = F_OWNER_PID;
    owner.pid = pid;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) != -1, "F_SETOWN_EX process");
    owner.type = -1;
    owner.pid = -1;
    check(fcntl(fds[1], F_GETOWN_EX, &owner) != -1 && owner.type == F_OWNER_PID && owner.pid == pid,
          "F_GETOWN_EX process");
    check(fcntl(fds[1], F_GETOWN) == pid, "F_GETOWN after F_SETOWN_EX");

    owner.type = F_OWNER_PGRP;
    owner.pid = pgrp;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) != -1, "F_SETOWN_EX process group");
    check(fcntl(fds[1], F_GETOWN_EX, &owner) != -1 && owner.type == F_OWNER_PGRP
              && owner.pid == pgrp,
          "F_GETOWN_EX process group");
    check(fcntl(fds[1], F_GETOWN) == -pgrp, "F_GETOWN after F_SETOWN_EX process group");

    /* F_SETOWN is seen by F_GETOWN_EX */
    check(fcntl(fds[1], F_SETOWN, -pgrp) != -1 && fcntl(fds[1], F_GETOWN_EX, &owner) != -1
              && owner.type == F_OWNER_PGRP && owner.pid == pgrp,
          "F_GETOWN_EX after F_SETOWN");

    /* Zero removes the owner */
    owner.type = F_OWNER_PID;
    owner.pid = 0;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) != -1 && fcntl(fds[1], F_GETOWN_EX, &owner) != -1
              && owner.pid == 0,
          "F_SETOWN_EX 0");

    /* Errors */
    owner.type = 42;
    owner.pid = pid;
    errno = 0;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) == -1 && errno == EINVAL, "F_SETOWN_EX bad type");
    owner.type = F_OWNER_PID;
    owner.pid = -pid;
    errno = 0;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) == -1 && errno == EINVAL, "F_SETOWN_EX negative");
    owner.type = F_OWNER_PGRP;
    errno = 0;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) == -1 && errno == EINVAL,
          "F_SETOWN_EX negative group");
    owner.type = F_OWNER_PID;
    owner.pid = dead;
    errno = 0;
    check(fcntl(fds[1], F_SETOWN_EX, &owner) == -1 && errno == ESRCH, "F_SETOWN_EX ESRCH");
#endif

    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    errno = 0;
    check(fcntl(fds[0], F_GETOWN) == -1 && errno == EBADF, "EBADF");
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
