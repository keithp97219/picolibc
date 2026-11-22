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
/* glibc declares pipe2 only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FD_MGMT && TEST_POSIX_PIPE && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * pipe and pipe2: the descriptors allocated, their flags, ownership,
 * order of data, and EMFILE leaving fildes alone
 */

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* The lowest descriptor not open */
static int
lowest_free(void)
{
    int fd;

    for (fd = 0; fcntl(fd, F_GETFD) != -1; fd++)
        ;
    return fd;
}

/* In a child, leave 'avail' descriptors free and try both functions */
static int
emfile(int avail)
{
    struct rlimit rl;
    int           fds[2] = { -5, -5 };
    int           top = lowest_free();

    if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
        return 2;
    rl.rlim_cur = (rlim_t)(top + avail);
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
        return 3;
    errno = 0;
    if (pipe(fds) != -1 || errno != EMFILE)
        return 4;
    if (fds[0] != -5 || fds[1] != -5)
        return 5;
    errno = 0;
    if (pipe2(fds, O_CLOEXEC) != -1 || errno != EMFILE)
        return 6;
    if (fds[0] != -5 || fds[1] != -5)
        return 7;
    /* Nothing was left open */
    if (lowest_free() != top)
        return 8;
    return 0;
}

static int
in_child(int avail)
{
    pid_t pid = fork();
    int   status;

    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(emfile(avail));
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    return WEXITSTATUS(status);
}

int
main(void)
{
    struct stat st;
    char        buf[16];
    int         fds[2], low;
    time_t      before;

    /* The two lowest free descriptors, read end first */
    low = lowest_free();
    before = time(NULL);
    check(pipe(fds) == 0, "pipe");
    check((fds[0] == low && fds[1] == lowest_free() - 1) || fds[1] == low, "lowest descriptors");
    check(fds[0] != fds[1], "two descriptors");

    /* No FD_CLOEXEC, no O_NONBLOCK; read end and write end */
    check(fcntl(fds[0], F_GETFD) == 0 && fcntl(fds[1], F_GETFD) == 0, "FD_CLOEXEC clear");
    check(!(fcntl(fds[0], F_GETFL) & O_NONBLOCK) && !(fcntl(fds[1], F_GETFL) & O_NONBLOCK),
          "O_NONBLOCK clear");
    check(write(fds[1], "first", 5) == 5 && write(fds[1], "second", 6) == 6, "write");
    check(read(fds[0], buf, 5) == 5 && memcmp(buf, "first", 5) == 0, "first in, first out");
    check(read(fds[0], buf, 6) == 6 && memcmp(buf, "second", 6) == 0, "then the second");

    /* Owned by the effective IDs, timestamps set at creation */
    check(fstat(fds[0], &st) == 0, "fstat");
    check(S_ISFIFO(st.st_mode), "a FIFO");
    check(st.st_uid == geteuid() && st.st_gid == getegid(), "owner");
    check(st.st_mtime >= before - 1 && st.st_ctime >= before - 1 && st.st_atime >= before - 1,
          "timestamps");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* pipe2: the flags set exactly what they name */
    check(pipe2(fds, 0) == 0, "pipe2 with no flags");
    check(fcntl(fds[0], F_GETFD) == 0 && fcntl(fds[1], F_GETFD) == 0, "no FD_CLOEXEC");
    check(!(fcntl(fds[0], F_GETFL) & O_NONBLOCK), "no O_NONBLOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    check(pipe2(fds, O_NONBLOCK) == 0, "pipe2 O_NONBLOCK");
    check((fcntl(fds[0], F_GETFL) & O_NONBLOCK) && (fcntl(fds[1], F_GETFL) & O_NONBLOCK),
          "O_NONBLOCK on both");
    check(fcntl(fds[0], F_GETFD) == 0 && fcntl(fds[1], F_GETFD) == 0, "still no FD_CLOEXEC");
    errno = 0;
    check(read(fds[0], buf, 1) == -1 && errno == EAGAIN, "non-blocking read end");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    check(pipe2(fds, O_CLOEXEC) == 0, "pipe2 O_CLOEXEC");
    check(fcntl(fds[0], F_GETFD) == FD_CLOEXEC && fcntl(fds[1], F_GETFD) == FD_CLOEXEC,
          "FD_CLOEXEC on both");
    check(!(fcntl(fds[0], F_GETFL) & O_NONBLOCK), "no O_NONBLOCK");
    check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");

    /* An invalid flag (may fail): nothing allocated, fildes alone */
    fds[0] = fds[1] = -5;
    low = lowest_free();
    if (pipe2(fds, O_APPEND) == -1) {
        check(errno == EINVAL, "EINVAL for a bad flag");
        check(fds[0] == -5 && fds[1] == -5 && lowest_free() == low, "nothing allocated");
    } else {
        check(close(fds[0]) == 0 && close(fds[1]) == 0, "close");
    }

    /* EMFILE with no descriptor free, and with just one */
    check(in_child(0) == 0, "EMFILE with none free (child status)");
    check(in_child(1) == 0, "EMFILE with one free (child status)");

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
