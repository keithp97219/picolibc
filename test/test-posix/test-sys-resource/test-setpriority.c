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
/* These are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>

/*
 * setpriority changes nice values: of the caller, clamped to the
 * supported range, and of every process in a group. Unprivileged
 * callers can't lower them. The changes are made in a child.
 */

#ifndef NZERO
#error "NZERO missing from <limits.h>"
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
child(int prio)
{
    int   ret, report[2], go[2], seen;
    pid_t pid;
    char  c;

    /* Raising the nice value is always allowed */
    check(setpriority(PRIO_PROCESS, 0, prio + 1) == 0, "setpriority");
    check(getpriority(PRIO_PROCESS, 0) == prio + 1, "new value");

    /* Lowering it again takes privilege (or a Linux RLIMIT_NICE allowance) */
    errno = 0;
    ret = setpriority(PRIO_PROCESS, 0, prio);
    if (ret != 0) {
        check(errno == EACCES || errno == EPERM, "lowering fails with EACCES");
        check(getpriority(PRIO_PROCESS, 0) == prio + 1, "unchanged after EACCES");
    } else if (geteuid() != 0) {
        printf("lowering the nice value was allowed\n");
    }

    /* PRIO_PGRP 0 changes every process in the caller's group */
    check(setpgid(0, 0) == 0, "own process group");
    check(pipe(report) == 0 && pipe(go) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int value;

        if (read(go[0], &c, 1) != 1)
            _exit(1);
        value = getpriority(PRIO_PROCESS, 0);
        (void)write(report[1], &value, sizeof(value));
        _exit(0);
    }
    check(setpriority(PRIO_PGRP, 0, prio + 2) == 0, "setpriority(PRIO_PGRP)");
    check(write(go[1], "x", 1) == 1, "go");
    check(read(report[0], &seen, sizeof(seen)) == sizeof(seen), "report");
    check(seen == prio + 2, "the other member has the group's value");
    check(waitpid(pid, NULL, 0) == pid, "waitpid");

    /* For a group, getpriority reports the lowest value */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        (void)read(go[0], &c, 1);
        _exit(0);
    }
    check(setpriority(PRIO_PROCESS, 0, prio + 3) == 0, "setpriority");
    errno = 0;
    check(getpriority(PRIO_PGRP, 0) == prio + 2 && errno == 0, "the group's lowest value");
    check(write(go[1], "x", 1) == 1, "go");
    check(waitpid(pid, NULL, 0) == pid, "waitpid");

    /* Values above the range give the highest supported value */
    check(setpriority(PRIO_PROCESS, 0, INT_MAX) == 0, "setpriority(INT_MAX)");
    ret = getpriority(PRIO_PROCESS, 0);
    /* Linux supports the whole range, up to NZERO - 1 */
    check(ret == NZERO - 1, "clamped to the highest value");
    exit(0);
}

int
main(void)
{
    int   prio, status;
    pid_t pid;

    errno = 0;
    prio = getpriority(PRIO_PROCESS, 0);
    check(errno == 0 && prio >= -NZERO && prio <= NZERO - 1, "own priority");
    if (prio > NZERO - 4) {
        printf("nice value %d leaves no room, skipping\n", prio);
        return 77;
    }

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        child(prio);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child checks");
    check(getpriority(PRIO_PROCESS, 0) == prio, "the parent is unchanged");

    errno = 0;
    check(setpriority(42, 0, 0) == -1 && errno == EINVAL, "EINVAL");
    errno = 0;
    check(setpriority(PRIO_PROCESS, INT_MAX, prio + 1) == -1 && errno == ESRCH, "ESRCH");
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
