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
/* lockf and F_LOCK are XSI interfaces */
#define _XOPEN_SOURCE 800
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_FD_MGMT) \
    && defined(__PICOLIBC_POSIX_PIPE) && defined(__PICOLIBC_POSIX_SIGNALS)       \
    && defined(__PICOLIBC_XSI_FILE_SYSTEM)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * lockf: sections, merging and splitting, negative and zero sizes,
 * other processes, deadlock, signals, release on close, and errors
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("file");
    if (chdir("..") == 0)
        (void)rmdir(dir);
}

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            cleanup();                                                                 \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

#define OFF_MAX                       ((off_t)INT64_MAX)

/*
 * In another process: is [start, start + len) free? 1 if lockable, 0
 * if another process has it, -1 on a surprise
 */
static int
free_in_child(off_t start, off_t len)
{
    pid_t pid = fork();
    int   status;

    check(pid >= 0, "fork");
    if (pid == 0) {
        int fd = open("file", O_RDWR);

        if (fd < 0 || lseek(fd, start, SEEK_SET) != start)
            _exit(2);
        errno = 0;
        if (lockf(fd, F_TEST, len) == 0) {
            if (lockf(fd, F_TLOCK, len) != 0)
                _exit(3);
            _exit(1);
        }
        if (errno != EACCES && errno != EAGAIN)
            _exit(4);
        errno = 0;
        if (lockf(fd, F_TLOCK, len) == 0 || (errno != EACCES && errno != EAGAIN))
            _exit(5);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    return WEXITSTATUS(status) <= 1 ? WEXITSTATUS(status) : -WEXITSTATUS(status);
}

static void
lock_at(int fd, int cmd, off_t start, off_t len, const char *message)
{
    check(lseek(fd, start, SEEK_SET) == start, "lseek");
    check(lockf(fd, cmd, len) == 0, message);
}

static volatile sig_atomic_t got_alarm;

static void
on_alarm(int sig)
{
    (void)sig;
    got_alarm = 1;
}

/* Lock 'mine', tell the parent, wait, then try for 'want' with F_LOCK */
static int
deadlock_child(int ready, int go, off_t mine, off_t want)
{
    int  fd = open("file", O_RDWR);
    char c = 0;
    int  ret;

    if (fd < 0 || lseek(fd, mine, SEEK_SET) != mine || lockf(fd, F_LOCK, 1) != 0)
        return 2;
    if (write(ready, &c, 1) != 1 || read(go, &c, 1) != 1)
        return 3;
    if (lseek(fd, want, SEEK_SET) != want)
        return 4;
    errno = 0;
    ret = lockf(fd, F_LOCK, 1);
    if (ret == -1 && errno == EDEADLK) {
        /* Let the other one go on */
        (void)lseek(fd, mine, SEEK_SET);
        (void)lockf(fd, F_ULOCK, 1);
        return 1;
    }
    return ret == 0 ? 0 : 5;
}

int
main(void)
{
    int              fd, fd2, ready[2], go[2], st1, st2, r1, r2;
    pid_t            p1, p2;
    struct sigaction sa;
    char             c[2] = { 0, 0 };

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0 && write(fd, "0123456789", 10) == 10, "create file");

    /* A section from the offset, seen by other processes, not by this one */
    lock_at(fd, F_LOCK, 2, 4, "F_LOCK");
    check(lockf(fd, F_TEST, 4) == 0, "own lock isn't a conflict");
    check(lseek(fd, 0, SEEK_CUR) == 2, "lockf leaves the offset");
    check(free_in_child(2, 4) == 0, "locked for another process");
    check(free_in_child(5, 1) == 0, "last byte locked");
    check(free_in_child(6, 1) == 1, "byte after free");
    check(free_in_child(0, 2) == 1, "bytes before free");

    /* Adjacent and overlapping sections merge; unlocking the middle splits */
    lock_at(fd, F_TLOCK, 6, 4, "adjacent F_TLOCK");
    lock_at(fd, F_LOCK, 0, 3, "overlapping F_LOCK");
    check(free_in_child(0, 10) == 0, "all locked");
    lock_at(fd, F_ULOCK, 4, 2, "unlock the middle");
    check(free_in_child(4, 2) == 1, "middle free");
    check(free_in_child(3, 1) == 0 && free_in_child(6, 1) == 0, "both ends still locked");
    lock_at(fd, F_ULOCK, 0, 0, "unlock to the end");
    check(free_in_child(0, 100) == 1, "all free");

    /* A negative size locks the bytes before the offset */
    lock_at(fd, F_LOCK, 8, -3, "negative size");
    check(free_in_child(5, 3) == 0, "bytes before the offset locked");
    check(free_in_child(8, 1) == 1 && free_in_child(4, 1) == 1, "the offset itself is free");
    lock_at(fd, F_ULOCK, 5, 3, "unlock");

    /* Size 0: from the offset through any future end of the file */
    lock_at(fd, F_LOCK, 20, 0, "size 0");
    check(free_in_child(1000000, 1) == 0, "far past the end locked");
    check(free_in_child(19, 1) == 1, "before the offset free");

    /* Unlocking up to the maximum offset trims a lock of size 0 */
    lock_at(fd, F_ULOCK, 30, OFF_MAX - 29, "unlock to OFF_MAX");
    check(free_in_child(30, 1) == 1 && free_in_child((off_t)1 << 40, 1) == 1, "tail released");
    check(free_in_child(20, 10) == 0, "start kept");
    lock_at(fd, F_ULOCK, 0, 0, "unlock everything");

    /* Errors, which leave the existing locks alone */
    lock_at(fd, F_LOCK, 0, 1, "lock byte 0");
    check(lseek(fd, 2, SEEK_SET) == 2, "lseek");
    check_err(lockf(fd, 42, 1), EINVAL, "bad function");
    check_err(lockf(fd, F_LOCK, -3), EINVAL, "size plus offset below 0");
    check_err(lockf(fd, F_TEST, -3), EINVAL, "F_TEST below 0");
    check(lseek(fd, (off_t)1 << 40, SEEK_SET) == (off_t)1 << 40, "lseek far out");
    check_err(lockf(fd, F_LOCK, OFF_MAX), EOVERFLOW, "last byte past the maximum offset");
    check(free_in_child(0, 1) == 0, "byte 0 still locked");
    check_err(lockf(-1, F_LOCK, 0), EBADF, "bad descriptor");
    fd2 = open("file", O_RDONLY);
    check(fd2 >= 0, "open for reading");
    check_err(lockf(fd2, F_LOCK, 1), EBADF, "F_LOCK on a read-only descriptor");
    check_err(lockf(fd2, F_TLOCK, 1), EBADF, "F_TLOCK on a read-only descriptor");
    check(lseek(fd2, 5, SEEK_SET) == 5 && lockf(fd2, F_TEST, 1) == 0,
          "F_TEST on a read-only descriptor");

    /* Closing any descriptor for the file releases the locks */
    check(close(fd2) == 0, "close the other descriptor");
    check(free_in_child(0, 1) == 1, "released on close");

    /* Deadlock: of two processes waiting for each other, one gets EDEADLK */
    check(pipe(ready) == 0 && pipe(go) == 0, "pipes");
    p1 = fork();
    check(p1 >= 0, "fork");
    if (p1 == 0)
        _exit(deadlock_child(ready[1], go[0], 0, 1));
    p2 = fork();
    check(p2 >= 0, "fork");
    if (p2 == 0)
        _exit(deadlock_child(ready[1], go[0], 1, 0));
    check(read(ready[0], c, 1) == 1 && read(ready[0], c, 1) == 1, "both locked");
    check(write(go[1], c, 2) == 2, "go");
    check(waitpid(p1, &st1, 0) == p1 && waitpid(p2, &st2, 0) == p2, "wait");
    check(WIFEXITED(st1) && WIFEXITED(st2), "children exited");
    r1 = WEXITSTATUS(st1);
    r2 = WEXITSTATUS(st2);
    check((r1 == 1 && r2 == 0) || (r1 == 0 && r2 == 1), "exactly one EDEADLK");
    check(close(ready[0]) == 0 && close(ready[1]) == 0 && close(go[0]) == 0 && close(go[1]) == 0,
          "close pipes");

    /* A signal interrupts F_LOCK while it waits */
    check(pipe(ready) == 0 && pipe(go) == 0, "pipes");
    p1 = fork();
    check(p1 >= 0, "fork");
    if (p1 == 0) {
        int cfd = open("file", O_RDWR);

        (void)close(ready[0]);
        (void)close(go[1]);
        if (cfd < 0 || lockf(cfd, F_LOCK, 0) != 0 || write(ready[1], c, 1) != 1)
            _exit(2);
        /* Hold the lock until the parent closes the pipe */
        (void)read(go[0], c, 1);
        _exit(0);
    }
    check(close(ready[1]) == 0 && close(go[0]) == 0, "close");
    check(read(ready[0], c, 1) == 1, "child holds the lock");
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    check(lseek(fd, 0, SEEK_SET) == 0, "lseek");
    alarm(1);
    check_err(lockf(fd, F_LOCK, 0), EINTR, "interrupted F_LOCK");
    check(got_alarm, "handler ran");
    check(close(go[1]) == 0 && close(ready[0]) == 0, "release the child");
    check(waitpid(p1, &st1, 0) == p1 && WIFEXITED(st1) && WEXITSTATUS(st1) == 0, "lock holder");

    check(close(fd) == 0, "close");
    cleanup();
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
