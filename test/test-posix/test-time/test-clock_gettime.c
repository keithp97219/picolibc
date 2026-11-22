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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_TIMERS && TEST_POSIX_CPUTIME && TEST_POSIX_THREAD_CPUTIME

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

/*
 * clock, clock_getres, clock_gettime, clock_settime, timespec_get,
 * nanosleep and the CPU-time clocks. Durations are checked with
 * generous bounds so that a busy host doesn't break the test.
 */

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

    check(clock_gettime(id, &ts) == 0, "clock_gettime");
    check(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L, "tv_nsec in range");
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static volatile unsigned sink;

/* Use about this much CPU time */
static void
spin(long long nsec)
{
    long long start = ns(CLOCK_PROCESS_CPUTIME_ID);

    while (ns(CLOCK_PROCESS_CPUTIME_ID) - start < nsec)
        for (int i = 0; i < 10000; i++)
            sink += i;
}

static volatile sig_atomic_t caught;

static void
handler(int sig)
{
    (void)sig;
    caught = 1;
}

static volatile int thread_go;

static void *
waiter(void *arg)
{
    (void)arg;
    while (!__atomic_load_n(&thread_go, __ATOMIC_ACQUIRE))
        sched_yield();
    return NULL;
}

/*
 * A seccomp filter, as containers install, may refuse clock_settime
 * with EPERM before the kernel looks at its arguments
 */
static bool
seccomp_filtered(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char  line[128];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "Seccomp:", 8) == 0) {
            ret = atoi(line + 8) == 2;
            break;
        }
    }
    fclose(f);
    return ret;
}

int
main(void)
{
    bool             filtered = seccomp_filtered();
    struct timespec  ts, res, rem;
    struct sigaction act;
    long long        a, b, c0, c1, p0, p1;
    clock_t          k0, k1;
    pid_t            pid;
    int              status;
    clockid_t        tid_clock;
    pthread_t        thread;

    /* The system may leave it to say at run time */
    if (sysconf(_SC_CPUTIME) <= 0) {
        printf("no CPU-time clock, skipping\n");
        return 77;
    }
    /* The system may leave it to say at run time */
    if (sysconf(_SC_THREAD_CPUTIME) <= 0) {
        printf("no thread CPU-time clock, skipping\n");
        return 77;
    }

    /* CLOCK_REALTIME is seconds since the Epoch, like time() */
    check(clock_gettime(CLOCK_REALTIME, &ts) == 0, "CLOCK_REALTIME");
    check(ts.tv_sec >= 1600000000 && ts.tv_sec - time(NULL) <= 1 && time(NULL) - ts.tv_sec <= 1,
          "CLOCK_REALTIME near time()");

    /* CLOCK_MONOTONIC never goes backwards */
    a = ns(CLOCK_MONOTONIC);
    for (int i = 0; i < 1000; i++) {
        b = ns(CLOCK_MONOTONIC);
        check(b >= a, "CLOCK_MONOTONIC monotonic");
        a = b;
    }

    /* clock_getres for every required clock; a null res is allowed */
    clockid_t clocks[]
        = { CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID };
    for (unsigned i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++) {
        check(clock_getres(clocks[i], &res) == 0, "clock_getres");
        check(res.tv_sec >= 0 && res.tv_nsec >= 0 && res.tv_nsec < 1000000000L
                  && (res.tv_sec > 0 || res.tv_nsec > 0),
              "resolution");
        check(clock_getres(clocks[i], NULL) == 0, "clock_getres with no res");
    }

    /* EINVAL for an unknown clock */
    errno = 0;
    check(clock_getres((clockid_t)12345, &res) == -1 && errno == EINVAL, "clock_getres EINVAL");
    errno = 0;
    check(clock_gettime((clockid_t)12345, &ts) == -1 && errno == EINVAL, "clock_gettime EINVAL");
    errno = 0;
    check(clock_settime((clockid_t)12345, &ts) == -1
              && (errno == EINVAL || (filtered && errno == EPERM)),
          "clock_settime EINVAL");

    /* clock_settime: never CLOCK_MONOTONIC, and tv_nsec must be in range */
    check(clock_gettime(CLOCK_MONOTONIC, &ts) == 0, "clock_gettime");
    errno = 0;
    check(clock_settime(CLOCK_MONOTONIC, &ts) == -1
              && (errno == EINVAL || (filtered && errno == EPERM)),
          "clock_settime CLOCK_MONOTONIC");
    check(clock_gettime(CLOCK_REALTIME, &ts) == 0, "clock_gettime");
    ts.tv_nsec = 1000000000L;
    errno = 0;
    check(clock_settime(CLOCK_REALTIME, &ts) == -1
              && (errno == EINVAL || (filtered && errno == EPERM)),
          "tv_nsec too big");
    ts.tv_nsec = -1;
    errno = 0;
    check(clock_settime(CLOCK_REALTIME, &ts) == -1
              && (errno == EINVAL || (filtered && errno == EPERM)),
          "tv_nsec negative");
    if (geteuid() != 0) {
        /* EPERM may be reported without the privilege */
        check(clock_gettime(CLOCK_REALTIME, &ts) == 0, "clock_gettime");
        errno = 0;
        check(clock_settime(CLOCK_REALTIME, &ts) == -1 && errno == EPERM, "clock_settime EPERM");
    }

    /* timespec_get(TIME_UTC) is clock_gettime(CLOCK_REALTIME) */
    check(timespec_get(&ts, TIME_UTC) == TIME_UTC, "timespec_get returns base");
    check(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L && ts.tv_sec - time(NULL) <= 1
              && time(NULL) - ts.tv_sec <= 1,
          "timespec_get value");
    check(timespec_get(&ts, 0) == 0, "timespec_get with an invalid base");

    /* The CPU-time clocks advance with CPU use and not while sleeping */
    p0 = ns(CLOCK_PROCESS_CPUTIME_ID);
    c0 = ns(CLOCK_THREAD_CPUTIME_ID);
    k0 = clock();
    spin(200000000);
    p1 = ns(CLOCK_PROCESS_CPUTIME_ID);
    c1 = ns(CLOCK_THREAD_CPUTIME_ID);
    k1 = clock();
    check(p1 - p0 >= 200000000 && c1 - c0 >= 100000000, "CPU-time clocks advance");
    check(k0 != (clock_t)-1 && k1 != (clock_t)-1, "clock");
    /* clock() / CLOCKS_PER_SEC is the process's CPU time in seconds */
    {
        double cpu = (double)(p1 - p0) / 1e9;
        double clk = (double)(k1 - k0) / CLOCKS_PER_SEC;
        if (clk < cpu * 0.5 - 0.02 || clk > cpu * 2 + 0.02) {
            printf("clock() measured %g s, CPU-time clock %g s\n", clk, cpu);
            exit(1);
        }
    }
    a = ns(CLOCK_THREAD_CPUTIME_ID);
    ts.tv_sec = 0;
    ts.tv_nsec = 200000000;
    check(nanosleep(&ts, NULL) == 0, "nanosleep");
    check(ns(CLOCK_THREAD_CPUTIME_ID) - a < 100000000, "thread CPU time doesn't count sleep");

    /* clock() doesn't count waited-for children */
    k0 = clock();
    p0 = ns(CLOCK_PROCESS_CPUTIME_ID);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        spin(300000000);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "waitpid");
    k1 = clock();
    p1 = ns(CLOCK_PROCESS_CPUTIME_ID);
    if ((double)(k1 - k0) / CLOCKS_PER_SEC > (double)(p1 - p0) / 1e9 + 0.15) {
        printf("clock() counted %g s across a child using 0.3 s\n",
               (double)(k1 - k0) / CLOCKS_PER_SEC);
        exit(1);
    }

    /* Another thread's CPU-time clock, from pthread_getcpuclockid */
    check(pthread_create(&thread, NULL, waiter, NULL) == 0, "pthread_create");
    check(pthread_getcpuclockid(thread, &tid_clock) == 0, "pthread_getcpuclockid");
    a = ns(tid_clock);
    check(a >= 0, "other thread's CPU time");
    __atomic_store_n(&thread_go, 1, __ATOMIC_RELEASE);
    check(pthread_join(thread, NULL) == 0, "pthread_join");
    check(pthread_getcpuclockid(pthread_self(), &tid_clock) == 0, "pthread_getcpuclockid self");
    a = ns(tid_clock);
    b = ns(CLOCK_THREAD_CPUTIME_ID);
    check(b >= a, "own thread clock matches CLOCK_THREAD_CPUTIME_ID");
    /* clock_nanosleep fails on the calling thread's CPU-time clock */
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    check(clock_nanosleep(CLOCK_THREAD_CPUTIME_ID, 0, &ts, NULL) == EINVAL,
          "clock_nanosleep on the thread CPU-time clock");

    /* nanosleep: at least the time asked for, measured on CLOCK_REALTIME */
    a = ns(CLOCK_REALTIME);
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;
    check(nanosleep(&ts, NULL) == 0, "nanosleep");
    check(ns(CLOCK_REALTIME) - a >= 100000000, "nanosleep slept long enough");

    /* EINVAL for a bad tv_nsec */
    ts.tv_nsec = 1000000000L;
    errno = 0;
    check(nanosleep(&ts, NULL) == -1 && errno == EINVAL, "nanosleep EINVAL big");
    ts.tv_nsec = -1;
    errno = 0;
    check(nanosleep(&ts, NULL) == -1 && errno == EINVAL, "nanosleep EINVAL negative");

    /* EINTR with the time left; rqtp and rmtp may be the same object */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    check(sigaction(SIGALRM, &act, NULL) == 0, "sigaction");
    alarm(1);
    ts.tv_sec = 30;
    ts.tv_nsec = 0;
    errno = 0;
    check(nanosleep(&ts, &ts) == -1 && errno == EINTR && caught, "nanosleep EINTR");
    check(ts.tv_sec < 30 && ts.tv_sec >= 15 && ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L,
          "time remaining");

    /* nanosleep doesn't change the signal mask or actions */
    {
        sigset_t         before, after;
        struct sigaction now;
        check(sigprocmask(SIG_SETMASK, NULL, &before) == 0, "sigprocmask");
        rem.tv_sec = 0;
        rem.tv_nsec = 1000;
        check(nanosleep(&rem, NULL) == 0, "nanosleep");
        check(sigprocmask(SIG_SETMASK, NULL, &after) == 0, "sigprocmask");
        check(sigismember(&after, SIGALRM) == sigismember(&before, SIGALRM), "mask unchanged");
        check(sigaction(SIGALRM, NULL, &now) == 0 && now.sa_handler == handler, "action unchanged");
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
