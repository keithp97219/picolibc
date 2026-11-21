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
#include <sys/times.h>
#include <unistd.h>

/* struct tms and its clock_t members */
static struct tms tms_ __attribute__((unused));
_Static_assert(_Generic(tms_.tms_utime, clock_t: 1, default: 0), "tms_utime");
_Static_assert(_Generic(tms_.tms_stime, clock_t: 1, default: 0), "tms_stime");
_Static_assert(_Generic(tms_.tms_cutime, clock_t: 1, default: 0), "tms_cutime");
_Static_assert(_Generic(tms_.tms_cstime, clock_t: 1, default: 0), "tms_cstime");

/* Without the system support this test needs, it is skipped */
#if _POSIX_TIMERS > 0

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

/*
 * times: elapsed time from a fixed point, the CPU time of this
 * process, and the times of waited-for children only
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static clock_t (* const p_times)(struct tms *) = times;

static double
mono(void)
{
    struct timespec ts;

    check(clock_gettime(CLOCK_MONOTONIC, &ts) == 0, "clock_gettime");
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static volatile unsigned sink;

/* Use about this much CPU time, in seconds */
static void
spin(double sec)
{
    struct timespec ts;
    double          start;

    check(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0, "clock_gettime");
    start = ts.tv_sec + ts.tv_nsec / 1e9;
    do {
        for (int i = 0; i < 10000; i++)
            sink += i;
        check(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0, "clock_gettime");
    } while (ts.tv_sec + ts.tv_nsec / 1e9 - start < sec);
}

int
main(void)
{
    struct tms      a, b;
    clock_t         r0, r1;
    long            tck = sysconf(_SC_CLK_TCK);
    double          m0, m1;
    pid_t           pid, grandchild;
    int             status;
    struct timespec ts = { 0, 300000000 };

    check(p_times != NULL && tck > 0, "times and CLK_TCK");

    /* The return value counts real time in clock ticks */
    m0 = mono();
    r0 = times(&a);
    check(r0 != (clock_t)-1, "times");
    check(nanosleep(&ts, NULL) == 0, "nanosleep");
    spin(0.2);
    r1 = times(&b);
    m1 = mono();
    check(r1 != (clock_t)-1 && r1 >= r0, "times doesn't go backwards");
    {
        double elapsed = (double)(r1 - r0) / tck;
        if (elapsed < (m1 - m0) - 0.05 || elapsed > (m1 - m0) + 0.05) {
            printf("times elapsed %g s, CLOCK_MONOTONIC %g s\n", elapsed, m1 - m0);
            exit(1);
        }
    }

    /* tms_utime + tms_stime is this process's CPU time */
    check(b.tms_utime >= a.tms_utime && b.tms_stime >= a.tms_stime,
          "CPU time doesn't go backwards");
    check((double)((b.tms_utime + b.tms_stime) - (a.tms_utime + a.tms_stime)) / tck >= 0.1,
          "CPU time counted");
    check(b.tms_cutime == 0 && b.tms_cstime == 0, "no children yet");

    /*
     * A child that hasn't waited for its own child doesn't count it,
     * and the parent counts the child once it has waited
     */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct tms c;
        siginfo_t  info;
        grandchild = fork();
        if (grandchild == 0) {
            spin(0.3);
            _exit(0);
        }
        /* Let the grandchild finish, leaving it unwaited for */
        if (grandchild < 0 || waitid(P_PID, (id_t)grandchild, &info, WEXITED | WNOWAIT) != 0)
            _exit(3);
        times(&c);
        if (c.tms_cutime != 0 || c.tms_cstime != 0)
            _exit(2);
        spin(0.3);
        _exit(0);
    }
    times(&a);
    check(a.tms_cutime == 0, "running child not counted");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) != 3, "waitid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "unwaited grandchild not counted");
    times(&b);
    if ((double)(b.tms_cutime + b.tms_cstime) / tck < 0.2) {
        printf("waited-for child counted %g s\n", (double)(b.tms_cutime + b.tms_cstime) / tck);
        exit(1);
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
