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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_SIGNALS)

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* waitid reports exited, killed, stopped and continued children */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static pid_t
child(int status)
{
    pid_t pid = fork();

    check(pid >= 0, "fork");
    if (pid == 0) {
        if (status < 0)
            for (;;)
                pause();
        _exit(status);
    }
    return pid;
}

int
main(void)
{
    siginfo_t info;
    pid_t     pid;
    int       status;

    /* A child that exits */
    pid = child(7);
    memset(&info, 0xff, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WEXITED) == 0, "waitid exited");
    check(info.si_pid == pid, "si_pid");
    check(info.si_signo == SIGCHLD, "si_signo");
    check(info.si_code == CLD_EXITED, "si_code exited");
    check(info.si_status == 7, "si_status exited");

    /* WNOWAIT leaves the child waitable */
    pid = child(9);
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOWAIT) == 0, "waitid WNOWAIT");
    check(info.si_pid == pid && info.si_status == 9, "WNOWAIT info");
    check(waitpid(pid, &status, 0) == pid, "waitpid after WNOWAIT");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 9, "status after WNOWAIT");

    /* A child that is stopped, continued and killed */
    pid = child(-1);
    memset(&info, 0xff, sizeof(info));
    check(waitid(P_PID, (id_t)pid, &info, WEXITED | WNOHANG) == 0, "waitid WNOHANG");
    check(info.si_pid == 0, "WNOHANG with no change");

    check(kill(pid, SIGSTOP) == 0, "SIGSTOP");
    check(waitid(P_PID, (id_t)pid, &info, WSTOPPED) == 0, "waitid stopped");
    check(info.si_code == CLD_STOPPED && info.si_status == SIGSTOP, "stopped info");

    check(kill(pid, SIGCONT) == 0, "SIGCONT");
    check(waitid(P_PID, (id_t)pid, &info, WCONTINUED) == 0, "waitid continued");
    check(info.si_code == CLD_CONTINUED && info.si_status == SIGCONT, "continued info");

    check(kill(pid, SIGTERM) == 0, "SIGTERM");
    check(waitid(P_ALL, 0, &info, WEXITED) == 0, "waitid killed");
    check(info.si_pid == pid, "killed si_pid");
    check(info.si_code == CLD_KILLED && info.si_status == SIGTERM, "killed info");

    /* No children left */
    check(waitid(P_ALL, 0, &info, WEXITED) == -1 && errno == ECHILD, "ECHILD");

    /* Some state to wait for is required */
    check(waitid(P_ALL, 0, &info, WNOHANG) == -1 && errno == EINVAL, "EINVAL");

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
