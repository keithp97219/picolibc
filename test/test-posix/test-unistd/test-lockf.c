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
/* lockf is an XSI interface */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                            \
    || (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0 \
        && defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* lockf locks, tests and unlocks sections of a file */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

/* Runs 'func' in a child process, returning its exit status */
static int
in_child(int (*func)(int), int fd)
{
    pid_t pid = fork();
    int   status;

    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(func(fd));
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    return WEXITSTATUS(status);
}

static int
test_lock(int fd)
{
    if (lseek(fd, 0, SEEK_SET) != 0)
        return 2;
    if (lockf(fd, F_TEST, 4) == 0)
        return 0;
    if (errno != EACCES && errno != EAGAIN)
        return 3;
    return lockf(fd, F_TLOCK, 4) == -1 && (errno == EACCES || errno == EAGAIN) ? 1 : 4;
}

int
main(void)
{
    int fd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0 && write(fd, "0123456789", 10) == 10, "create file");
    check(lseek(fd, 0, SEEK_SET) == 0, "lseek");
    check(lockf(fd, F_LOCK, 4) == 0, "F_LOCK");
    check(lockf(fd, F_TEST, 4) == 0, "own lock");
    check(in_child(test_lock, fd) == 1, "locked for another process");
    check(lockf(fd, F_ULOCK, 4) == 0, "F_ULOCK");
    check(in_child(test_lock, fd) == 0, "unlocked");
    check(lockf(fd, F_TLOCK, 0) == 0, "F_TLOCK to the end");
    check(lockf(fd, 42, 0) == -1 && errno == EINVAL, "bad command");
    check(close(fd) == 0 && unlink("file") == 0 && chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
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
