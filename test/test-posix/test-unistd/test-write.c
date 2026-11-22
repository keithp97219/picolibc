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
/* SIGXFSZ comes with XSI */
#define _XOPEN_SOURCE 800
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_DEVICE_IO && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT && TEST_POSIX_PIPE        \
    && TEST_POSIX_SIGNALS && TEST_POSIX_NETWORKING && TEST_POSIX_MULTI_PROCESS && TEST_XSI_SIGNALS

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * write and pwrite: offsets, O_APPEND, the file size limit, pipes with
 * and without O_NONBLOCK, PIPE_BUF atomicity, EPIPE and the errors
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("file");
    (void)unlink("limit");
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

/* In a child: report failure through the exit status */
#define child_check(condition, code) \
    do {                             \
        if (!(condition))            \
            _exit(code);             \
    } while (0)

static volatile sig_atomic_t signals;

static void
on_signal(int sig)
{
    (void)sig;
    signals++;
}

static void
catch(int sig)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    check(sigaction(sig, &sa, NULL) == 0, "sigaction");
}

static int
wait_child(pid_t pid)
{
    int status;

    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (!WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

#define BIG (256 * 1024)

static char big[BIG];

int
main(void)
{
    char        buf[64];
    int         fd, rfd, p[2], s[2], i, n;
    ssize_t     ret;
    pid_t       pid, pid2;
    struct stat st;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");

    /* A zero count on a regular file writes nothing and returns 0 */
    check(write(fd, "", 0) == 0, "zero count");
    check(fstat(fd, &st) == 0 && st.st_size == 0, "nothing written");

    /* Writes start at the offset, advance it and extend the file */
    check(write(fd, "0123456789", 10) == 10, "write");
    check(lseek(fd, 0, SEEK_CUR) == 10, "offset advanced");
    check(lseek(fd, 4, SEEK_SET) == 4 && write(fd, "ab", 2) == 2, "overwrite");
    check(pread(fd, buf, 10, 0) == 10 && memcmp(buf, "0123ab6789", 10) == 0, "data overwritten");
    check(fstat(fd, &st) == 0 && st.st_size == 10, "size unchanged by an overwrite");
    check(lseek(fd, 9, SEEK_SET) == 9 && write(fd, "XY", 2) == 2, "write across the end");
    check(fstat(fd, &st) == 0 && st.st_size == 11, "size is the last byte plus one");

    /* The modification and change times are marked for update */
    {
        struct timespec old[2] = {
            { 1000, 0 },
            { 1000, 0 }
        };

        check(futimens(fd, old) == 0, "set old times");
        check(write(fd, "z", 1) == 1, "write");
        check(fstat(fd, &st) == 0 && st.st_mtim.tv_sec > 1000, "modification time updated");
    }

    /* pwrite writes at the offset and leaves the file offset alone */
    check(lseek(fd, 1, SEEK_SET) == 1, "lseek");
    check(pwrite(fd, "PQ", 2, 6) == 2, "pwrite");
    check(lseek(fd, 0, SEEK_CUR) == 1, "pwrite keeps the offset");
    check(pread(fd, buf, 2, 6) == 2 && memcmp(buf, "PQ", 2) == 0, "pwrite data");
    check_err(pwrite(fd, "x", 1, -1), EINVAL, "pwrite with a negative offset");
    check(lseek(fd, 0, SEEK_CUR) == 1, "failed pwrite keeps the offset");

    /* EFBIG at or past the offset maximum (no room for any byte) */
    check_err(pwrite(fd, "x", 1, (off_t)1 << 62), EFBIG, "pwrite past the maximum file size");

    /* EBADF: not open for writing, or not open */
    rfd = open("file", O_RDONLY);
    check(rfd >= 0, "open for reading");
    check_err(write(rfd, "x", 1), EBADF, "write on a read-only descriptor");
    check_err(pwrite(rfd, "x", 1, 0), EBADF, "pwrite on a read-only descriptor");
    check(close(rfd) == 0, "close");
    check_err(write(rfd, "x", 1), EBADF, "write on a closed descriptor");

    /* O_APPEND: every write goes to the end */
    check(close(fd) == 0, "close");
    fd = open("file", O_RDWR | O_APPEND);
    check(fd >= 0, "open O_APPEND");
    check(lseek(fd, 0, SEEK_SET) == 0 && write(fd, "end", 3) == 3, "append");
    check(fstat(fd, &st) == 0 && st.st_size == 15, "appended");
    check(lseek(fd, 0, SEEK_CUR) == 15, "offset at the end after appending");
    check(pread(fd, buf, 3, 12) == 3 && memcmp(buf, "end", 3) == 0, "appended data");
    /*
     * pwrite never moves the offset, O_APPEND or not. (Linux writes the
     * data at the end of the file, not at the offset, before 6.9's
     * RWF_NOAPPEND; that part isn't checked.)
     */
    check(lseek(fd, 2, SEEK_SET) == 2 && pwrite(fd, "pp", 2, 0) == 2, "pwrite with O_APPEND");
    check(lseek(fd, 0, SEEK_CUR) == 2, "pwrite with O_APPEND keeps the offset");
    check(close(fd) == 0, "close");

    /*
     * The file size limit: a write that would pass it writes what fits;
     * the next one fails with EFBIG and SIGXFSZ
     */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl = { 20, 20 };
        int           lfd;

        catch(SIGXFSZ);
        child_check(setrlimit(RLIMIT_FSIZE, &rl) == 0, 2);
        lfd = open("limit", O_WRONLY | O_CREAT | O_TRUNC, 0600);
        child_check(lfd >= 0, 3);
        memset(big, 'L', 512);
        child_check(write(lfd, big, 512) == 20, 4);
        child_check(signals == 0, 5);
        errno = 0;
        child_check(write(lfd, big, 512) == -1 && errno == EFBIG, 6);
        child_check(signals == 1, 7);
        errno = 0;
        child_check(pwrite(lfd, big, 1, 30) == -1 && errno == EFBIG, 8);
        child_check(signals == 2, 9);
        _exit(0);
    }
    n = wait_child(pid);
    check(n == 0, "file size limit (child status)");
    check(stat("limit", &st) == 0 && st.st_size == 20, "only what fits was written");

    /* Pipes: data comes out in order; a big blocking write is complete */
    check(pipe(p) == 0, "pipe");
    check_err(pwrite(p[1], "x", 1, 0), ESPIPE, "pwrite on a pipe");
#ifndef __GLIBC__ /* glibc passes Linux's EINVAL through */
    check_err(pwrite(p[1], "x", 1, -1), ESPIPE, "pwrite on a pipe with a negative offset");
#endif
    for (i = 0; i < BIG; i++)
        big[i] = (char)(i * 7);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        static char got[BIG];
        size_t      have = 0;

        (void)close(p[1]);
        while (have < BIG) {
            ret = read(p[0], got + have, BIG - have);
            child_check(ret > 0, 2);
            have += (size_t)ret;
        }
        child_check(memcmp(got, big, BIG) == 0, 3);
        _exit(0);
    }
    check(write(p[1], big, BIG) == BIG, "blocking write returns nbyte");
    check(wait_child(pid) == 0, "reader");

    /* O_NONBLOCK: fill the pipe */
    check(fcntl(p[1], F_SETFL, O_NONBLOCK) == 0, "O_NONBLOCK");
    n = 0;
    while ((ret = write(p[1], "f", 1)) == 1)
        n++;
    check(ret == -1 && errno == EAGAIN && n > 0, "full pipe gives EAGAIN");
    /* A PIPE_BUF write is all or nothing */
    check_err(write(p[1], big, PIPE_BUF), EAGAIN, "PIPE_BUF write to a full pipe");
    /* Once everything is read, a big write transfers at least PIPE_BUF */
    while (n > 0) {
        ret = read(p[0], buf, (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf));
        check(ret > 0, "drain");
        n -= (int)ret;
    }
    ret = write(p[1], big, BIG);
    check(ret >= PIPE_BUF && ret < BIG, "partial non-blocking write");
    n = (int)ret;
    while (n > 0) {
        ret = read(p[0], big, (size_t)n < BIG ? (size_t)n : BIG);
        check(ret > 0, "drain");
        n -= (int)ret;
    }
    check(fcntl(p[1], F_SETFL, 0) == 0, "clear O_NONBLOCK");

    /* Writes of PIPE_BUF or less from several processes aren't interleaved */
#define RECORDS 2000
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        static char rec[PIPE_BUF];

        memset(rec, 'A', sizeof(rec));
        for (i = 0; i < RECORDS; i++)
            child_check(write(p[1], rec, PIPE_BUF) == PIPE_BUF, 2);
        _exit(0);
    }
    pid2 = fork();
    check(pid2 >= 0, "fork");
    if (pid2 == 0) {
        static char rec[PIPE_BUF];
        memset(rec, 'B', sizeof(rec));
        for (i = 0; i < RECORDS; i++)
            child_check(write(p[1], rec, PIPE_BUF) == PIPE_BUF, 2);
        _exit(0);
    }
    for (i = 0; i < 2 * RECORDS; i++) {
        static char rec[PIPE_BUF];
        size_t      have = 0;
        int         j;

        while (have < PIPE_BUF) {
            ret = read(p[0], rec + have, PIPE_BUF - have);
            check(ret > 0, "read records");
            have += (size_t)ret;
        }
        for (j = 1; j < PIPE_BUF; j++)
            check(rec[j] == rec[0], "a PIPE_BUF write isn't interleaved");
    }
    check(wait_child(pid) == 0 && wait_child(pid2) == 0, "writers");

    /* A caught signal interrupts a write blocked on a full pipe: EINTR */
    check(fcntl(p[1], F_SETFL, O_NONBLOCK) == 0, "O_NONBLOCK");
    while (write(p[1], "f", 1) == 1)
        ;
    check(fcntl(p[1], F_SETFL, 0) == 0, "clear O_NONBLOCK");
    catch(SIGALRM);
    signals = 0;
    alarm(1);
    check_err(write(p[1], "x", 1), EINTR, "interrupted write");
    check(signals == 1, "handler ran");

    /* No reader: EPIPE and SIGPIPE */
    catch(SIGPIPE);
    signals = 0;
    check(close(p[0]) == 0, "close read end");
    check_err(write(p[1], "x", 1), EPIPE, "write with no reader");
    check(signals == 1, "SIGPIPE");
    check(close(p[1]) == 0, "close write end");

    /* Sockets: write is send with no flags; EPIPE once shut down */
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, s) == 0, "socketpair");
    check(write(s[0], "sock", 4) == 4 && recv(s[1], buf, sizeof(buf), 0) == 4, "write a socket");
    check(shutdown(s[0], SHUT_WR) == 0, "shutdown");
    signals = 0;
    check_err(write(s[0], "x", 1), EPIPE, "write after shutdown");
    check(signals == 1, "SIGPIPE for a stream socket");
    check_err(pwrite(s[1], "x", 1, 0), ESPIPE, "pwrite on a socket");
    check(close(s[0]) == 0 && close(s[1]) == 0, "close sockets");

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
