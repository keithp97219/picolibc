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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FD_MGMT && TEST_POSIX_PIPE && TEST_POSIX_SIGNALS

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

/*
 * fcntl record locks: what F_GETLK reports, ranges, waiting, signals,
 * inheritance, release on close and exit, and errors. Another process
 * holds or probes the locks; pipes keep the two in step.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define OFF_MAX ((off_t)(((uintmax_t)1 << (sizeof(off_t) * 8 - 1)) - 1))

static char dir[] = "/tmp/setlkw-XXXXXX";
static char path[64];

static int
set_lock(int fd, int cmd, short type, short whence, off_t start, off_t len)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));
    fl.l_type = type;
    fl.l_whence = whence;
    fl.l_start = start;
    fl.l_len = len;
    return fcntl(fd, cmd, &fl);
}

/* Child: the lock type that blocks a write lock on [start, start + len) */
static int
probe_child(int fd, off_t start, off_t len)
{
    struct flock fl;

    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = len;
    if (fcntl(fd, F_GETLK, &fl) != 0)
        return 9;
    if (fl.l_type == F_UNLCK)
        return 0;
    if (fl.l_pid != getppid())
        return 8;
    return fl.l_type == F_RDLCK ? 1 : 2;
}

/* Runs probe_child in another process, returning 0 (free), 1 (read lock) or 2 (write lock) */
static int
probe(int fd, off_t start, off_t len)
{
    pid_t pid = fork();
    int   status;

    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(probe_child(fd, start, len));
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "probe child");
    check(WEXITSTATUS(status) <= 2, "probe child result");
    return WEXITSTATUS(status);
}

static volatile sig_atomic_t alarms;

static void
on_alarm(int sig)
{
    (void)sig;
    alarms++;
}

/* Interrupts the calling process every 20 ms */
static void
ticker(int on)
{
    struct itimerval it;

    memset(&it, 0, sizeof(it));
    if (on) {
        it.it_value.tv_usec = 20000;
        it.it_interval.tv_usec = 20000;
    }
    check(setitimer(ITIMER_REAL, &it, NULL) == 0, "setitimer");
}

int
main(void)
{
    struct sigaction act;
    struct flock     fl;
    char             buf[100], c;
    int              fd, fd2, to_child[2], to_parent[2], status;
    pid_t            child;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/file", dir);
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    memset(buf, 'x', sizeof(buf));
    check(fd >= 0 && write(fd, buf, sizeof(buf)) == (ssize_t)sizeof(buf), "create file");

    /* A child holds a write lock on bytes 10-19 until told to exit */
    check(pipe(to_child) == 0 && pipe(to_parent) == 0, "pipes");
    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        if (set_lock(fd, F_SETLK, F_WRLCK, SEEK_SET, 10, 10) != 0)
            _exit(1);
        if (write(to_parent[1], "L", 1) != 1 || read(to_child[0], &c, 1) != 1)
            _exit(2);
        _exit(0);
    }
    check(read(to_parent[0], &c, 1) == 1, "child locked");

    /* F_GETLK reports the blocking lock, with l_whence SEEK_SET */
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_END;
    fl.l_start = -100;
    fl.l_len = 0;
    check(fcntl(fd, F_GETLK, &fl) == 0, "F_GETLK");
    check(fl.l_type == F_WRLCK && fl.l_whence == SEEK_SET && fl.l_start == 10 && fl.l_len == 10
              && fl.l_pid == child,
          "F_GETLK describes the blocking lock");

    /* Without a blocking lock only l_type changes */
    check(lseek(fd, 50, SEEK_SET) == 50, "lseek");
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_CUR;
    fl.l_start = -45;
    fl.l_len = 3;
    fl.l_pid = 12345;
    check(fcntl(fd, F_GETLK, &fl) == 0, "F_GETLK unblocked");
    check(fl.l_type == F_UNLCK && fl.l_whence == SEEK_CUR && fl.l_start == -45 && fl.l_len == 3
              && fl.l_pid == 12345,
          "F_GETLK leaves the rest unchanged");

    /* A negative l_len covers l_start + l_len to l_start - 1 */
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 25;
    fl.l_len = -6;
    check(fcntl(fd, F_GETLK, &fl) == 0 && fl.l_type == F_WRLCK, "negative l_len reaches byte 19");
    fl.l_type = F_WRLCK;
    fl.l_start = 10;
    fl.l_len = -5;
    check(fcntl(fd, F_GETLK, &fl) == 0 && fl.l_type == F_UNLCK, "negative l_len ends at byte 9");

    /* F_SETLK fails at once */
    errno = 0;
    check(set_lock(fd, F_SETLK, F_RDLCK, SEEK_SET, 15, 1) == -1
              && (errno == EACCES || errno == EAGAIN),
          "F_SETLK blocked");
    check(set_lock(fd, F_SETLK, F_RDLCK, SEEK_SET, 20, 10) == 0, "F_SETLK next to the lock");
    check(set_lock(fd, F_SETLK, F_UNLCK, SEEK_SET, 20, 10) == 0, "unlock");

    /* A signal interrupts F_SETLKW, which then fails with EINTR */
    memset(&act, 0, sizeof(act));
    act.sa_handler = on_alarm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    check(sigaction(SIGALRM, &act, NULL) == 0, "sigaction");
    ticker(1);
    errno = 0;
    check(set_lock(fd, F_SETLKW, F_WRLCK, SEEK_SET, 0, 0) == -1 && errno == EINTR,
          "F_SETLKW EINTR");
    ticker(0);
    check(alarms > 0, "the signal arrived");

    /* F_SETLKW waits until the child exits, which releases its lock */
    check(write(to_child[1], "X", 1) == 1, "release child");
    check(set_lock(fd, F_SETLKW, F_WRLCK, SEEK_SET, 0, 0) == 0, "F_SETLKW");
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child exit");

    /* Our lock isn't inherited: a child sees it as ours */
    check(probe(fd, 50, 1) == 2, "a write lock on the whole file");

    /* A new lock replaces ours over its range, and an unlock can split a lock */
    check(set_lock(fd, F_SETLK, F_RDLCK, SEEK_SET, 0, 10) == 0, "read lock replaces write lock");
    check(probe(fd, 5, 1) == 1 && probe(fd, 10, 1) == 2, "split by type");
    check(set_lock(fd, F_SETLK, F_UNLCK, SEEK_SET, 3, 3) == 0, "unlock bytes 3-5");
    check(probe(fd, 2, 1) == 1 && probe(fd, 4, 1) == 0 && probe(fd, 6, 1) == 1, "split by unlock");

    /* l_len 0 extends past the end of the file */
    check(set_lock(fd, F_SETLK, F_UNLCK, SEEK_SET, 0, 0) == 0, "unlock all");
    check(probe(fd, 0, 0) == 0, "all free");
    check(set_lock(fd, F_SETLK, F_WRLCK, SEEK_SET, 50, 0) == 0, "lock to EOF and beyond");
    check(probe(fd, 1000000, 1) == 2 && probe(fd, 49, 1) == 0, "l_len 0 reaches past EOF");
    /* Unlocking up to the largest offset is unlocking to the end */
    check(set_lock(fd, F_SETLK, F_UNLCK, SEEK_SET, 60, OFF_MAX - 60 + 1) == 0, "unlock to OFF_MAX");
    check(probe(fd, 55, 1) == 2 && probe(fd, 60, 1) == 0 && probe(fd, 1000000, 0) == 0,
          "unlocked from 60 on");

    /* Closing any descriptor for the file releases all our locks on it */
    fd2 = open(path, O_RDONLY);
    check(fd2 >= 0 && close(fd2) == 0, "open and close another descriptor");
    check(probe(fd, 0, 0) == 0, "close released the locks");

    /* The lock type must match the access mode */
    fd2 = open(path, O_RDONLY);
    check(fd2 >= 0, "open read-only");
    errno = 0;
    check(set_lock(fd2, F_SETLK, F_WRLCK, SEEK_SET, 0, 1) == -1 && errno == EBADF,
          "EBADF write lock on a read-only descriptor");
    check(close(fd2) == 0, "close");
    fd2 = open(path, O_WRONLY);
    check(fd2 >= 0, "open write-only");
    errno = 0;
    check(set_lock(fd2, F_SETLK, F_RDLCK, SEEK_SET, 0, 1) == -1 && errno == EBADF,
          "EBADF read lock on a write-only descriptor");
    check(close(fd2) == 0, "close");

    /* Invalid data */
    errno = 0;
    check(set_lock(fd, F_SETLK, F_WRLCK, 42, 0, 1) == -1 && errno == EINVAL, "EINVAL l_whence");
    errno = 0;
    check(set_lock(fd, F_SETLK, F_WRLCK, SEEK_SET, -1, 1) == -1 && errno == EINVAL,
          "EINVAL before the start of the file");
    errno = 0;
    check(set_lock(fd, F_SETLK, F_WRLCK, SEEK_SET, 5, -10) == -1 && errno == EINVAL,
          "EINVAL negative l_len before the start");
    errno = 0;
    check(set_lock(fd, F_SETLK, F_WRLCK, SEEK_SET, OFF_MAX - 5, 10) == -1 && errno == EOVERFLOW,
          "EOVERFLOW past the largest offset");
    errno = 0;
    check(set_lock(fd, F_GETLK, F_WRLCK, SEEK_SET, OFF_MAX - 5, 10) == -1 && errno == EOVERFLOW,
          "F_GETLK EOVERFLOW");
    errno = 0;
    check(set_lock(-1, F_SETLK, F_WRLCK, SEEK_SET, 0, 1) == -1 && errno == EBADF, "EBADF");

#ifdef F_OFD_SETLK
    /* glibc has the open file description locks only for _GNU_SOURCE */
    if (set_lock(fd, F_OFD_SETLK, F_WRLCK, SEEK_SET, 0, 1) == 0) {
        /* A child sharing the open file description shares the lock */
        child = fork();
        check(child >= 0, "fork");
        if (child == 0) {
            int fd3 = open(path, O_RDWR);

            if (set_lock(fd, F_OFD_SETLK, F_WRLCK, SEEK_SET, 0, 1) != 0)
                _exit(1);
            if (fd3 < 0 || set_lock(fd3, F_OFD_SETLK, F_WRLCK, SEEK_SET, 0, 1) != -1
                || errno != EAGAIN)
                _exit(2);
            _exit(0);
        }
        check(waitpid(child, &status, 0) == child && WIFEXITED(status), "OFD child");
        check(WEXITSTATUS(status) == 0, "OFD locks belong to the open file description");

        /* They conflict with process locks, even in the same process */
        fd2 = open(path, O_RDWR);
        check(fd2 >= 0, "open another description");
        errno = 0;
        check(set_lock(fd2, F_SETLK, F_WRLCK, SEEK_SET, 0, 1) == -1
                  && (errno == EACCES || errno == EAGAIN),
              "a process lock conflicts with an OFD lock");
        memset(&fl, 0, sizeof(fl));
        fl.l_type = F_RDLCK;
        fl.l_whence = SEEK_SET;
        fl.l_len = 1;
        check(fcntl(fd2, F_GETLK, &fl) == 0 && fl.l_type == F_WRLCK && fl.l_pid == -1,
              "F_GETLK reports an OFD lock with l_pid -1");
        /* F_OFD_SETLKW can be interrupted too */
        ticker(1);
        errno = 0;
        check(set_lock(fd2, F_OFD_SETLKW, F_WRLCK, SEEK_SET, 0, 1) == -1 && errno == EINTR,
              "F_OFD_SETLKW EINTR");
        ticker(0);
        /* Closing the only descriptor releases the lock */
        check(close(fd) == 0, "close");
        check(set_lock(fd2, F_OFD_SETLK, F_WRLCK, SEEK_SET, 0, 1) == 0,
              "closing the description released its lock");
        check(close(fd2) == 0, "close");
        fd = open(path, O_RDWR);
        check(fd >= 0, "reopen");
    } else {
        /* Open file description locks came with POSIX.1-2024's FD_MGMT */
        check(TEST_POSIX_FD_MGMT < 202405L && errno == EINVAL, "no open file description locks");
    }
#endif

    check(close(fd) == 0 && unlink(path) == 0 && rmdir(dir) == 0, "clean up");
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
