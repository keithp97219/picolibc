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
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(_POSIX_ADVISORY_INFO) && _POSIX_ADVISORY_INFO > 0)                            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)         \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_PIPE) && __PICOLIBC_POSIX_PIPE > 0)                       \
        && (defined(__PICOLIBC_POSIX_FIFO) && __PICOLIBC_POSIX_FIFO > 0)                       \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0)     \
        && (defined(__PICOLIBC_POSIX_SIGNALS) && __PICOLIBC_POSIX_SIGNALS > 0))

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_fallocate reserves space, extending the file */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "tmp-XXXXXX";

#define OFF_MAX ((off_t)(((uintmax_t)1 << (sizeof(off_t) * 8 - 1)) - 1))

static volatile sig_atomic_t xfsz;

static void
on_xfsz(int sig)
{
    (void)sig;
    xfsz = 1;
}

int
main(void)
{
    struct stat st;
    int         fd, fd2, fds[2], ret, status;
    pid_t       pid;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    check(chdir(dir) == 0, "chdir");
    fd = open("file", O_RDWR | O_CREAT, 0600);
    check(fd >= 0, "create file");
    ret = posix_fallocate(fd, 0, 8192);
    if (ret == EINVAL || ret == ENOTSUP) {
        printf("file system can't allocate, skipping\n");
        check(close(fd) == 0 && unlink("file") == 0 && chdir("..") == 0 && rmdir(dir) == 0,
              "cleanup");
        return 77;
    }
    check(ret == 0, "posix_fallocate");
    check(fstat(fd, &st) == 0 && st.st_size == 8192, "extended");
    check(posix_fallocate(fd, 0, 100) == 0, "inside the file");
    check(fstat(fd, &st) == 0 && st.st_size == 8192, "not shrunk");
    check(posix_fallocate(fd, 0, 0) == EINVAL, "zero length");
    check(posix_fallocate(fd, -1, 10) == EINVAL, "negative offset");
    check(posix_fallocate(-1, 0, 10) == EBADF, "EBADF");
    check(pipe(fds) == 0, "pipe");
    check(posix_fallocate(fds[1], 0, 10) == ESPIPE, "ESPIPE");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close pipe");

    /* Past the end, the file grows to offset + len */
    check(posix_fallocate(fd, 10000, 100) == 0 && fstat(fd, &st) == 0 && st.st_size == 10100,
          "grows to offset + len");
    /* Writes into the allocated space succeed */
    check(pwrite(fd, "x", 1, 10050) == 1, "write into the space");
    /* Opening with O_TRUNC frees it */
    fd2 = open("file", O_WRONLY | O_TRUNC);
    check(fd2 >= 0 && fstat(fd2, &st) == 0 && st.st_size == 0, "O_TRUNC");
    check(close(fd2) == 0, "close");

    /* A descriptor without write permission */
    fd2 = open("file", O_RDONLY);
    check(fd2 >= 0 && posix_fallocate(fd2, 0, 10) == EBADF, "EBADF read-only");
    check(close(fd2) == 0, "close");

    /* Past the largest file size */
    check(posix_fallocate(fd, OFF_MAX - 5, 10) == EFBIG, "EFBIG past OFF_MAX");
    check(posix_fallocate(fd, 0, -1) == EINVAL, "negative length");

    /* Past the process's file size limit: EFBIG and SIGXFSZ */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl = { 4096, 4096 };

        signal(SIGXFSZ, on_xfsz);
        if (setrlimit(RLIMIT_FSIZE, &rl) != 0)
            _exit(1);
        if (posix_fallocate(fd, 0, 8192) != EFBIG)
            _exit(2);
        _exit(xfsz ? 0 : 3);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    check(WEXITSTATUS(status) == 0, "EFBIG and SIGXFSZ past RLIMIT_FSIZE");

    /* Something other than a regular file */
    fd2 = open("/dev/null", O_WRONLY);
    if (fd2 >= 0) {
        check(posix_fallocate(fd2, 0, 10) == ENODEV, "ENODEV");
        check(close(fd2) == 0, "close");
    }
    check(mkfifo("fifo", 0600) == 0, "mkfifo");
    fds[0] = open("fifo", O_RDONLY | O_NONBLOCK);
    fds[1] = open("fifo", O_WRONLY | O_NONBLOCK);
    check(fds[0] >= 0 && fds[1] >= 0, "open FIFO");
    check(posix_fallocate(fds[1], 0, 10) == ESPIPE, "FIFO ESPIPE");
    check(close(fds[0]) == 0 && close(fds[1]) == 0 && unlink("fifo") == 0, "close FIFO");

#ifdef __PICOLIBC__
    /*
     * procfs can't allocate space for its regular files. glibc emulates
     * posix_fallocate with writes there, which would change the name.
     */
    fd2 = open("/proc/self/comm", O_WRONLY);
    if (fd2 >= 0) {
        check(posix_fallocate(fd2, 0, 1) == ENOTSUP, "ENOTSUP");
        check(close(fd2) == 0, "close");
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
