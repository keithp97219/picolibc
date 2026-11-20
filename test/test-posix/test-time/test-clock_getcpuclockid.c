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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Process CPU-time clocks advance, and reaped processes have none */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static long long
ns(clockid_t id)
{
    struct timespec ts;

    check(clock_gettime(id, &ts) == 0, "clock_gettime on a CPU-time clock");
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* Spin for about the given time of wall-clock time */
static void
spin(long long nsec)
{
    long long end = ns(CLOCK_MONOTONIC) + nsec;

    while (ns(CLOCK_MONOTONIC) < end)
        ;
}

int
main(void)
{
    clockid_t id;
    long long before;
    pid_t     pid;
    int       status;

    /* The calling process, by 0 and by its pid */
    check(clock_getcpuclockid(0, &id) == 0, "clock_getcpuclockid(0)");
    before = ns(id);
    spin(50000000);
    check(ns(id) > before, "process CPU time advances");
    check(clock_getcpuclockid(getpid(), &id) == 0, "clock_getcpuclockid(getpid())");
    before = ns(id);
    spin(50000000);
    check(ns(id) > before, "own pid CPU time advances");

    /* A process that's gone */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(0);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(clock_getcpuclockid(pid, &id) == ESRCH, "clock_getcpuclockid of a reaped child");

    printf("success\n");
    return 0;
}
