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
#if TEST_POSIX_FD_MGMT && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_PIPE && TEST_POSIX_MULTI_PROCESS \
    && TEST_XSI_SIGNALS && TEST_POSIX_SYMBOLIC_LINKS && TEST_POSIX_FIFO

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * ftruncate and truncate: shrinking, zero-filled growth, offsets, times,
 * the file size limit, and the errors leaving the file alone
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("file");
    (void)unlink("link");
    (void)unlink("loop");
    (void)unlink("fifo");
    (void)chmod("sub", 0700);
    (void)unlink("sub/file");
    (void)rmdir("sub");
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

static off_t
size_of(const char *path)
{
    struct stat st;

    check(stat(path, &st) == 0, "stat");
    return st.st_size;
}

static volatile sig_atomic_t signals;

static void
on_signal(int sig)
{
    (void)sig;
    signals++;
}

static char long_name[NAME_MAX + 2];

int
main(void)
{
    struct stat     st;
    struct timespec old[2] = {
        { 1000, 0 },
        { 1000, 0 }
    };
    char  buf[32];
    int   fd, rfd, p[2], status, i;
    pid_t pid;

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    fd = open("file", O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");
    check(write(fd, "0123456789", 10) == 10, "write");

    /* Shrink: the data past the length is gone; the offset doesn't move */
    check(ftruncate(fd, 4) == 0, "shrink");
    check(size_of("file") == 4, "size after shrinking");
    check(lseek(fd, 0, SEEK_CUR) == 10, "ftruncate keeps the offset");
    check(pread(fd, buf, sizeof(buf), 0) == 4 && memcmp(buf, "0123", 4) == 0, "data kept");

    /* Grow: the new part reads as zeros */
    check(ftruncate(fd, 12) == 0, "grow");
    check(size_of("file") == 12, "size after growing");
    memset(buf, 'z', sizeof(buf));
    check(pread(fd, buf, sizeof(buf), 0) == 12 && memcmp(buf, "0123", 4) == 0, "read grown file");
    for (i = 4; i < 12; i++)
        check(buf[i] == 0, "zero filled");

    /* Times marked for update */
    check(futimens(fd, old) == 0, "set old times");
    check(ftruncate(fd, 6) == 0, "ftruncate");
    check(fstat(fd, &st) == 0 && st.st_mtime > 1000 && st.st_ctime > 1000, "times updated");

    /* truncate does the same by name, leaving open offsets alone */
    check(truncate("file", 2) == 0 && size_of("file") == 2, "truncate shrinks");
    check(truncate("file", 8) == 0 && size_of("file") == 8, "truncate grows");
    check(pread(fd, buf, 8, 0) == 8 && buf[2] == 0 && buf[7] == 0, "truncate zero fills");
    check(lseek(fd, 0, SEEK_CUR) == 10, "truncate keeps open offsets");
    check(futimens(fd, old) == 0, "set old times");
    check(truncate("file", 3) == 0, "truncate");
    check(fstat(fd, &st) == 0 && st.st_mtime > 1000 && st.st_ctime > 1000, "times updated");

    /* EINVAL: negative length, file unchanged */
    check_err(ftruncate(fd, -1), EINVAL, "ftruncate to a negative length");
    check_err(truncate("file", -1), EINVAL, "truncate to a negative length");
    check(size_of("file") == 3, "unchanged after EINVAL");

    /*
     * EFBIG or EINVAL: more than the maximum file size, if 2^62 bytes
     * is (some file systems, like ZFS, take files that large)
     */
    errno = 0;
    if (ftruncate(fd, (off_t)1 << 62) == 0) {
        check(size_of("file") == (off_t)1 << 62 && ftruncate(fd, 3) == 0, "a huge file");
    } else {
        check(errno == EFBIG || errno == EINVAL, "ftruncate past the maximum file size");
        errno = 0;
        check(truncate("file", (off_t)1 << 62) == -1 && (errno == EFBIG || errno == EINVAL),
              "truncate past the maximum file size");
    }
    check(size_of("file") == 3, "unchanged after EFBIG");

    /* Not open for writing: EBADF or EINVAL */
    rfd = open("file", O_RDONLY);
    check(rfd >= 0, "open for reading");
    errno = 0;
    check(ftruncate(rfd, 1) == -1 && (errno == EBADF || errno == EINVAL),
          "ftruncate on a read-only descriptor");
    check(close(rfd) == 0, "close");
    check_err(ftruncate(rfd, 1), EBADF, "ftruncate on a closed descriptor");
    check(size_of("file") == 3, "unchanged after EBADF");

    /* EINVAL: a pipe, a FIFO */
    check(pipe(p) == 0, "pipe");
    check_err(ftruncate(p[1], 0), EINVAL, "ftruncate on a pipe");
    check(close(p[0]) == 0 && close(p[1]) == 0, "close pipe");
    check(mkfifo("fifo", 0600) == 0, "mkfifo");
    check_err(truncate("fifo", 0), EINVAL, "truncate on a FIFO");

    /* The file size limit: EFBIG and SIGXFSZ, file unchanged */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit    rl = { 100, 100 };
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_signal;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGXFSZ, &sa, NULL) != 0 || setrlimit(RLIMIT_FSIZE, &rl) != 0)
            _exit(2);
        if (ftruncate(fd, 100) != 0)
            _exit(3);
        errno = 0;
        if (ftruncate(fd, 101) != -1 || errno != EFBIG)
            _exit(4);
        if (signals != 1)
            _exit(5);
        errno = 0;
        if (truncate("file", 200) != -1 || errno != EFBIG)
            _exit(6);
        if (signals != 2)
            _exit(7);
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    check(WEXITSTATUS(status) == 0, "file size limit (child status)");
    check(size_of("file") == 100, "unchanged past the limit");

    /* Pathname errors for truncate */
    check(mkdir("sub", 0700) == 0, "mkdir");
    check_err(truncate("sub", 0), EISDIR, "truncate a directory");
    check_err(truncate("missing", 0), ENOENT, "missing file");
    check_err(truncate("", 0), ENOENT, "empty path");
    check_err(truncate("file/x", 0), ENOTDIR, "prefix not a directory");
    check_err(truncate("file/", 0), ENOTDIR, "trailing slash on a file");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(truncate("loop", 0), ELOOP, "symbolic link loop");
    check(symlink("file", "link") == 0, "symlink");
    check(truncate("link", 5) == 0 && size_of("file") == 5, "truncate follows links");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(truncate(long_name, 0), ENAMETOOLONG, "component longer than NAME_MAX");

    /* EACCES: no write permission, or no search permission (not as root) */
    if (geteuid() != 0) {
        check(close(open("sub/file", O_WRONLY | O_CREAT, 0400)) == 0, "create read-only file");
        check_err(truncate("sub/file", 0), EACCES, "no write permission");
        check(chmod("sub", 0600) == 0, "remove search permission");
        check_err(truncate("sub/file", 0), EACCES, "no search permission");
        check(chmod("sub", 0700) == 0, "restore search permission");
    }

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
