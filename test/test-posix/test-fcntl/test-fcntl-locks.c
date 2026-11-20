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
#if !defined(__PICOLIBC__)                                                            \
    || (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0 \
        && defined(__PICOLIBC_POSIX_FD_MGMT) && __PICOLIBC_POSIX_FD_MGMT > 0)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/* fcntl record and open file description locks exclude other processes */

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

/* Tries a write lock on bytes 0-9, and reports what blocks it */
static int
probe(int fd)
{
    struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 10 };

    if (fcntl(fd, F_SETLK, &fl) == 0)
        return 0;
    if (errno != EACCES && errno != EAGAIN)
        return 2;
    if (fcntl(fd, F_GETLK, &fl) < 0)
        return 3;
    return fl.l_type == F_RDLCK && fl.l_pid == getppid() ? 1 : 4;
}

int
main(void)
{
    struct flock fl;
    int          fd;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0 && write(fd, "0123456789abcdef", 16) == 16, "create file");

    /* A read lock on bytes 5-9 blocks another process's write lock */
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 5;
    fl.l_len = 5;
    check(fcntl(fd, F_SETLK, &fl) == 0, "F_SETLK");
    check(in_child(probe, fd) == 1, "blocked, and F_GETLK reports the holder");

    /* Our own locks don't block us */
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_len = 10;
    check(fcntl(fd, F_GETLK, &fl) == 0 && fl.l_type == F_UNLCK, "F_GETLK own lock");

    fl.l_type = F_UNLCK;
    fl.l_start = 5;
    fl.l_len = 5;
    check(fcntl(fd, F_SETLKW, &fl) == 0, "unlock");
    check(in_child(probe, fd) == 0, "free after unlock");

    fl.l_type = 42;
    check(fcntl(fd, F_SETLK, &fl) == -1 && errno == EINVAL, "bad lock type");

#ifdef F_OFD_SETLK
    /* Open file description locks */
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_len = 1;
    if (fcntl(fd, F_OFD_SETLK, &fl) == 0) {
        int fd2 = open("file", O_RDWR);

        check(fd2 >= 0, "second open");
        /* A different open file description conflicts, even in this process */
        check(fcntl(fd2, F_OFD_SETLK, &fl) == -1 && (errno == EAGAIN || errno == EACCES),
              "F_OFD_SETLK conflict");
        check(fcntl(fd2, F_OFD_GETLK, &fl) == 0 && fl.l_type == F_WRLCK && fl.l_pid == -1,
              "F_OFD_GETLK");
        check(close(fd2) == 0, "close");
    } else {
        check(errno == EINVAL, "no open file description locks");
    }
#endif
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
