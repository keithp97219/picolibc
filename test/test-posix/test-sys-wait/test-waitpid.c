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
#if !defined(__PICOLIBC__)                                                              \
    || ((defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0)          \
        && (defined(_POSIX_JOB_CONTROL) && _POSIX_JOB_CONTROL > 0))

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

/* waitpid reports the chosen child's exit, stop and continuation, with or without waiting */

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
    pid_t pid, other;
    int   status = 12345;

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        for (;;)
            pause();
    }

    /* WNOHANG with nothing to report returns 0 and leaves status alone */
    check(waitpid(pid, &status, WNOHANG) == 0 && status == 12345, "WNOHANG");

    /* Stopped and continued children, with the signal that stopped them */
    check(kill(pid, SIGSTOP) == 0, "SIGSTOP");
    check(waitpid(pid, &status, WUNTRACED) == pid, "WUNTRACED");
    check(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP, "stopped");
    check(!WIFEXITED(status) && !WIFSIGNALED(status), "only stopped");
    check(kill(pid, SIGCONT) == 0, "SIGCONT");
    check(waitpid(pid, &status, WCONTINUED) == pid && WIFCONTINUED(status), "continued");

    /* Only the child asked for is reported */
    other = fork();
    check(other >= 0, "fork");
    if (other == 0)
        _exit(9);
    check(kill(pid, SIGUSR1) == 0, "SIGUSR1");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1, "killed by SIGUSR1");

    /* A null status pointer is allowed */
    check(waitpid(other, NULL, 0) == other, "null status");

    errno = 0;
    check(waitpid(-1, &status, 0) == -1 && errno == ECHILD, "ECHILD");
    errno = 0;
    check(waitpid(getpid(), &status, 0) == -1 && errno == ECHILD, "not a child");
    errno = 0;
    check(waitpid(-1, &status, 0x100) == -1 && errno == EINVAL, "bad options");
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
