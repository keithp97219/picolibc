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
/* getitimer and setitimer, XSI interfaces POSIX.1-2024 removed */
#define _XOPEN_SOURCE 800
#define _DEFAULT_SOURCE
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_SIGNALS \
    && TEST_POSIX_FILE_ATTRIBUTES

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The child of fork is a copy of its parent except for the things fork resets */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "FORK.TXT"
#endif

static volatile int copied = 1;

static void
handler(int sig)
{
    (void)sig;
}

/* Uses some processor time, so the counts fork resets aren't zero in the parent */
static void
burn(void)
{
    struct timespec start, now;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    do {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
    } while ((now.tv_sec - start.tv_sec) * 1000000000L + (now.tv_nsec - start.tv_nsec)
             < 200000000L);
}

static int
child(pid_t parent, int fd, timer_t timer)
{
    struct sigaction  sa;
    sigset_t          set;
    struct itimerval  it;
    struct itimerspec ts;
    struct tms        t;
    struct flock      lock;
    struct timespec   cpu;

    if (getppid() != parent || getpid() == parent)
        return 10;
    /* The descriptor refers to the parent's open file description */
    if (lseek(fd, 0, SEEK_CUR) != 5 || write(fd, "child", 5) != 5)
        return 11;
    /* The mask and the actions are copied; pending signals are not */
    if (sigprocmask(SIG_BLOCK, NULL, &set) != 0 || sigismember(&set, SIGUSR1) != 1)
        return 12;
    if (sigpending(&set) != 0 || sigismember(&set, SIGUSR1) != 0)
        return 13;
    if (sigaction(SIGUSR2, NULL, &sa) != 0 || sa.sa_handler != handler)
        return 14;
    /* No alarm, no interval timers and no per-process timers */
    if (alarm(0) != 0)
        return 15;
    if (getitimer(ITIMER_VIRTUAL, &it) != 0 || it.it_value.tv_sec != 0 || it.it_value.tv_usec != 0)
        return 16;
    errno = 0;
    if (timer_gettime(timer, &ts) == 0) {
        if (ts.it_value.tv_sec != 0 || ts.it_value.tv_nsec != 0)
            return 17;
    }
    /* The times of children and the CPU-time clock start again */
    if (times(&t) == (clock_t)-1 || t.tms_cutime != 0 || t.tms_cstime != 0)
        return 18;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu) != 0 || cpu.tv_sec != 0
        || cpu.tv_nsec > 100000000L)
        return 19;
    /* The parent's record lock isn't the child's */
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 1;
    errno = 0;
    if (fcntl(fd, F_SETLK, &lock) != -1 || (errno != EAGAIN && errno != EACCES))
        return 20;
    if (fcntl(fd, F_GETLK, &lock) != 0 || lock.l_type != F_WRLCK || lock.l_pid != parent)
        return 21;
    /* Memory is a copy */
    if (copied != 1)
        return 22;
    copied = 2;
    return 0;
}

int
main(void)
{
    struct sigaction  sa;
    sigset_t          set;
    struct itimerval  it;
    struct itimerspec ts;
    struct sigevent   sev;
    struct tms        t;
    struct flock      lock;
    timer_t           timer;
    pid_t             pid, self = getpid();
    int               fd, status;
    char              buf[16];

    /* A child that uses processor time, so this process has children's times */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        burn();
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "first child");
    check(times(&t) != (clock_t)-1 && t.tms_cutime + t.tms_cstime > 0, "children's times");
    burn();

    (void)unlink(TEST_FILE_NAME);
    fd = open(TEST_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "open");
    check(write(fd, "hello", 5) == 5, "write");
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 1;
    check(fcntl(fd, F_SETLK, &lock) == 0, "lock");

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    check(sigaction(SIGUSR2, &sa, NULL) == 0, "sigaction");
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &set, NULL) == 0 && raise(SIGUSR1) == 0, "pending signal");
    alarm(100);
    memset(&it, 0, sizeof(it));
    it.it_value.tv_sec = 100;
    check(setitimer(ITIMER_VIRTUAL, &it, NULL) == 0, "setitimer");
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR2;
    check(timer_create(CLOCK_MONOTONIC, &sev, &timer) == 0, "timer_create");
    memset(&ts, 0, sizeof(ts));
    ts.it_value.tv_sec = 100;
    check(timer_settime(timer, 0, &ts, NULL) == 0, "timer_settime");

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(child(self, fd, timer));
    check(pid != self, "a new process ID");
    /* The child's ID names no process group */
    errno = 0;
    check(getpgid(pid) == getpgrp() && kill(-pid, 0) == -1 && errno == ESRCH, "child's group");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child exited");
    if (WEXITSTATUS(status) != 0) {
        printf("child check %d failed\n", WEXITSTATUS(status));
        return 1;
    }
    /* The parent keeps its own state */
    check(copied == 1, "memory");
    check(lseek(fd, 0, SEEK_CUR) == 10, "the child moved the shared offset");
    check(pread(fd, buf, 10, 0) == 10 && memcmp(buf, "hellochild", 10) == 0, "contents");
    check(sigpending(&set) == 0 && sigismember(&set, SIGUSR1) == 1, "still pending");
    check(alarm(0) > 0, "the parent's alarm");
    check(getitimer(ITIMER_VIRTUAL, &it) == 0 && it.it_value.tv_sec > 0, "the parent's timer");
    check(timer_gettime(timer, &ts) == 0 && ts.it_value.tv_sec > 0, "the parent's POSIX timer");
    check(timer_delete(timer) == 0, "timer_delete");
    memset(&it, 0, sizeof(it));
    check(setitimer(ITIMER_VIRTUAL, &it, NULL) == 0, "stop the timer");

    check(close(fd) == 0 && unlink(TEST_FILE_NAME) == 0, "clean up");
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
