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
#if TEST_POSIX_SIGNALS

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

/* kill checks and sends signals */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    caught = sig;
}

int
main(void)
{
    struct sigaction act;
    pid_t            child;
    int              status;

    check(kill(getpid(), 0) == 0, "kill with signal 0");
    errno = 0;
    check(kill(getpid(), -1) == -1 && errno == EINVAL, "kill with a negative signal");
    errno = 0;
    check(kill(getpid(), SIGRTMAX + 1) == -1 && errno == EINVAL, "kill past SIGRTMAX");

    /* The signal is delivered to the sender before kill returns */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGUSR1, &act, NULL) == 0, "sigaction");
    check(kill(getpid(), SIGUSR1) == 0 && caught == SIGUSR1, "delivered before kill returns");

    /* pid 0 is the caller's process group */
    caught = 0;
    check(kill(0, SIGUSR1) == 0 && caught == SIGUSR1, "kill to the process group");
    caught = 0;
    check(kill(-getpgrp(), SIGUSR1) == 0 && caught == SIGUSR1, "kill to -pgrp");

#if TEST_POSIX_MULTI_PROCESS
    /* ESRCH once the process is gone */
    child = fork();
    check(child >= 0, "fork");
    if (child == 0)
        _exit(0);
    check(waitpid(child, &status, 0) == child, "waitpid");
    errno = 0;
    check(kill(child, 0) == -1 && errno == ESRCH, "ESRCH");
#else
    (void)child;
    (void)status;
#endif

    /*
     * EPERM for a process of another user. init usually runs as root,
     * but in a container it may be ours, and then the call succeeds.
     */
    if (geteuid() != 0 && getuid() != 0) {
        errno = 0;
        check(kill(1, 0) == 0 || errno == EPERM, "EPERM");
    }

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
