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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_FILE_SYSTEM) && defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT)      \
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS) \
    && defined(__PICOLIBC_POSIX_FIFO) && defined(__PICOLIBC_POSIX_FD_MGMT)                  \
    && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* open and openat: the oflag values, the descriptor, the mode and the errors */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "/tmp/open-XXXXXX";

/* Two names can be in use at once */
static const char *
name(const char *n)
{
    static char paths[2][PATH_MAX];
    static int  which;

    which = !which;
    snprintf(paths[which], sizeof(paths[which]), "%s/%s", dir, n);
    return paths[which];
}

static void
check_fails(const char *path, int oflag, int err, const char *message)
{
    errno = 0;
    if (open(path, oflag, 0600) != -1 || errno != err) {
        printf("%s: errno %d, wanted %d\n", message, errno, err);
        exit(1);
    }
}

int
main(void)
{
    struct stat     st;
    struct timespec past[2] = {
        { 1000000000, 0 },
        { 1000000000, 0 }
    };
    char   buf[16], longname[NAME_MAX + 2];
    mode_t old = umask(022);
    int    fd, fd2, dfd, status;
    pid_t  pid;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0, "open directory");

    /* O_CREAT makes a regular file with the masked mode, owned by us, and marks times */
    check(futimens(dfd, past) == 0, "backdate directory");
    fd = open(name("file"), O_RDWR | O_CREAT | O_EXCL, 0666);
    check(fd >= 0, "O_CREAT");
    check(fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 07777) == 0644,
          "created mode is masked");
    check(st.st_uid == geteuid() && st.st_size == 0, "owner and size");
    {
        struct stat dst;

        check(fstat(dfd, &dst) == 0 && (st.st_gid == dst.st_gid || st.st_gid == getegid()),
              "group");
    }
    check(fstat(dfd, &st) == 0 && st.st_mtim.tv_sec > 1000000000, "parent marked for update");

    /* The new descriptor is the lowest available one, without FD_CLOEXEC */
    fd2 = open(name("file"), O_RDONLY);
    check(fd2 == fd + 1 || fd2 >= 0, "second open");
    check(fcntl(fd2, F_GETFD) == 0, "FD_CLOEXEC clear");
    check(close(fd2) == 0, "close");
    check(open(name("file"), O_RDONLY) == fd2, "the lowest descriptor is reused");
    check(close(fd2) == 0, "close");

    /* O_CLOEXEC sets FD_CLOEXEC */
    fd2 = open(name("file"), O_RDONLY | O_CLOEXEC);
    check(fd2 >= 0 && (fcntl(fd2, F_GETFD) & FD_CLOEXEC), "O_CLOEXEC");
    check(close(fd2) == 0, "close");

    /* A new open file description: its own offset, starting at 0 */
    check(write(fd, "0123456789", 10) == 10, "write");
    fd2 = open(name("file"), O_RDONLY);
    check(fd2 >= 0 && lseek(fd2, 0, SEEK_CUR) == 0, "offset starts at 0");
    check(read(fd2, buf, 4) == 4 && memcmp(buf, "0123", 4) == 0, "read");
    check(lseek(fd, 0, SEEK_CUR) == 10, "other offset unchanged");
    check(close(fd2) == 0, "close");

    /* The access mode limits what the descriptor can do */
    fd2 = open(name("file"), O_RDONLY);
    errno = 0;
    check(fd2 >= 0 && write(fd2, "x", 1) == -1 && errno == EBADF, "O_RDONLY can't write");
    check((fcntl(fd2, F_GETFL) & O_ACCMODE) == O_RDONLY, "O_RDONLY reported");
    check(close(fd2) == 0, "close");
    fd2 = open(name("file"), O_WRONLY);
    errno = 0;
    check(fd2 >= 0 && read(fd2, buf, 1) == -1 && errno == EBADF, "O_WRONLY can't read");
    check((fcntl(fd2, F_GETFL) & O_ACCMODE) == O_WRONLY, "O_WRONLY reported");
    check(close(fd2) == 0, "close");
    check((fcntl(fd, F_GETFL) & O_ACCMODE) == O_RDWR, "O_RDWR reported");

    /* O_CREAT on an existing file changes nothing; O_EXCL makes it fail */
    fd2 = open(name("file"), O_RDWR | O_CREAT, 0600);
    check(fd2 >= 0 && fstat(fd2, &st) == 0 && st.st_size == 10 && (st.st_mode & 0777) == 0644,
          "O_CREAT on an existing file");
    check(close(fd2) == 0, "close");
    check_fails(name("file"), O_RDWR | O_CREAT | O_EXCL, EEXIST, "O_EXCL EEXIST");

    /* O_APPEND writes at the end */
    fd2 = open(name("file"), O_WRONLY | O_APPEND);
    check(fd2 >= 0 && lseek(fd2, 0, SEEK_SET) == 0 && write(fd2, "ab", 2) == 2, "O_APPEND write");
    check(lseek(fd2, 0, SEEK_CUR) == 12, "O_APPEND moved to the end");
    check(fcntl(fd2, F_GETFL) & O_APPEND, "O_APPEND reported");
    check(close(fd2) == 0, "close");

    /* O_TRUNC empties a regular file, keeping its mode and owner, and marks its times */
    check(futimens(fd, past) == 0, "backdate file");
    fd2 = open(name("file"), O_WRONLY | O_TRUNC);
    check(fd2 >= 0 && fstat(fd2, &st) == 0 && st.st_size == 0 && (st.st_mode & 0777) == 0644
              && st.st_uid == geteuid(),
          "O_TRUNC");
    check(st.st_mtim.tv_sec > 1000000000 && st.st_ctim.tv_sec > 1000000000,
          "O_TRUNC marks the times");
    check(close(fd2) == 0, "close");

    /* O_SYNC, O_DSYNC and O_RSYNC are accepted on regular files */
    fd2 = open(name("file"), O_WRONLY | O_SYNC);
    check(fd2 >= 0 && (fcntl(fd2, F_GETFL) & O_SYNC) == O_SYNC, "O_SYNC");
    check(write(fd2, "s", 1) == 1 && close(fd2) == 0, "O_SYNC write");
    fd2 = open(name("file"), O_RDWR | O_DSYNC | O_RSYNC);
    check(fd2 >= 0 && (fcntl(fd2, F_GETFL) & O_DSYNC) == O_DSYNC, "O_DSYNC and O_RSYNC");
    check(close(fd2) == 0, "close");

    /* O_NOCTTY and O_NONBLOCK are ignored for regular files */
    fd2 = open(name("file"), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    check(fd2 >= 0 && close(fd2) == 0, "O_NOCTTY and O_NONBLOCK");
#ifndef __GLIBC__
    /* glibc doesn't define O_TTY_INIT, which is ignored for other files */
    fd2 = open(name("file"), O_RDONLY | O_TTY_INIT);
    check(fd2 >= 0 && close(fd2) == 0, "O_TTY_INIT");
#endif

    /* Symbolic links */
    check(symlink("file", name("link")) == 0 && symlink("nowhere", name("dangling")) == 0,
          "symlinks");
    fd2 = open(name("link"), O_RDONLY);
    check(fd2 >= 0 && close(fd2) == 0, "open follows the link");
    check_fails(name("link"), O_RDONLY | O_NOFOLLOW, ELOOP, "O_NOFOLLOW ELOOP");
    check_fails(name("link"), O_RDWR | O_CREAT | O_EXCL, EEXIST, "O_EXCL on a link");
    check_fails(name("dangling"), O_RDWR | O_CREAT | O_EXCL, EEXIST, "O_EXCL on a dangling link");
    check(lstat(name("nowhere"), &st) == -1 && errno == ENOENT, "nothing made at the target");
    check(symlink("loop", name("loop")) == 0, "symlink loop");
    check_fails(name("loop"), O_RDONLY, ELOOP, "ELOOP");

    /* Directories */
    check_fails(name("file"), O_RDONLY | O_DIRECTORY, ENOTDIR, "O_DIRECTORY ENOTDIR");
    check_fails(dir, O_WRONLY, EISDIR, "EISDIR O_WRONLY");
    check_fails(dir, O_RDWR, EISDIR, "EISDIR O_RDWR");
    check_fails(dir, O_RDONLY | O_CREAT, EISDIR, "EISDIR O_CREAT");
    fd2 = open(dir, O_RDONLY | O_DIRECTORY);
    check(fd2 >= 0 && close(fd2) == 0, "O_DIRECTORY");

#ifndef __GLIBC__
    /* glibc defines neither O_SEARCH nor O_EXEC; picolibc gives them one value */
    fd2 = open(dir, O_SEARCH);
    check(fd2 >= 0, "O_SEARCH");
    check((fcntl(fd2, F_GETFL) & O_ACCMODE) == O_SEARCH, "O_SEARCH reported");
    {
        int fd3 = openat(fd2, "file", O_RDONLY);

        check(fd3 >= 0 && close(fd3) == 0, "openat through an O_SEARCH descriptor");
    }
    check(close(fd2) == 0, "close");
    fd2 = open(name("file"), O_EXEC);
    errno = 0;
    check(fd2 >= 0 && read(fd2, buf, 1) == -1 && errno == EBADF, "O_EXEC can't read");
    check((fcntl(fd2, F_GETFL) & O_ACCMODE) == O_EXEC, "O_EXEC reported");
    check(close(fd2) == 0, "close");
#endif

    /* FIFOs: O_NONBLOCK readers don't wait; writers without a reader fail */
    check(mkfifo(name("fifo"), 0600) == 0, "mkfifo");
    check_fails(name("fifo"), O_WRONLY | O_NONBLOCK, ENXIO, "ENXIO");
    fd2 = open(name("fifo"), O_RDONLY | O_NONBLOCK);
    check(fd2 >= 0, "O_NONBLOCK reader");
    {
        int fd3 = open(name("fifo"), O_WRONLY | O_NONBLOCK | O_TRUNC);

        check(fd3 >= 0 && write(fd3, "f", 1) == 1, "writer with a reader, O_TRUNC ignored");
        check(read(fd2, buf, 1) == 1 && buf[0] == 'f', "through the FIFO");
        check(close(fd3) == 0, "close writer");
    }
    check(close(fd2) == 0, "close reader");
    /* Without O_NONBLOCK, a reader waits for a writer */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int wfd = open(name("fifo"), O_WRONLY);

        _exit(wfd >= 0 && write(wfd, "w", 1) == 1 ? 0 : 1);
    }
    fd2 = open(name("fifo"), O_RDONLY);
    check(fd2 >= 0 && read(fd2, buf, 1) == 1 && buf[0] == 'w', "blocking open of a FIFO");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "writer");
    check(close(fd2) == 0, "close");

    /* openat */
    fd2 = openat(dfd, "file", O_RDONLY);
    check(fd2 >= 0 && close(fd2) == 0, "openat");
    fd2 = openat(-1, name("file"), O_RDONLY);
    check(fd2 >= 0 && close(fd2) == 0, "an absolute path ignores the descriptor");
    errno = 0;
    check(openat(-1, "file", O_RDONLY) == -1 && errno == EBADF, "openat EBADF");
    errno = 0;
    check(openat(fd, "file", O_RDONLY) == -1 && errno == ENOTDIR, "openat ENOTDIR descriptor");

    /* Other errors */
    check_fails("", O_RDONLY, ENOENT, "ENOENT empty path");
    check_fails(name("missing"), O_RDONLY, ENOENT, "ENOENT");
    check_fails(name("missing/file"), O_RDWR | O_CREAT, ENOENT, "ENOENT O_CREAT prefix");
    check_fails(name("file/x"), O_RDONLY, ENOTDIR, "ENOTDIR prefix");
    check_fails(name("file/"), O_RDONLY, ENOTDIR, "ENOTDIR trailing slash");
    errno = 0;
    check(open(name("new/"), O_RDWR | O_CREAT, 0600) == -1
              && (errno == ENOENT || errno == ENOTDIR || errno == EISDIR),
          "O_CREAT trailing slash");
    memset(longname, 'n', sizeof(longname) - 1);
    longname[sizeof(longname) - 1] = '\0';
    check_fails(name(longname), O_RDWR | O_CREAT, ENAMETOOLONG, "ENAMETOOLONG");
    if (geteuid() != 0) {
        /* Root has every permission */
        check(fchmod(fd, 0) == 0, "chmod 0");
        check_fails(name("file"), O_RDONLY, EACCES, "EACCES read");
        check_fails(name("file"), O_WRONLY, EACCES, "EACCES write");
        check(fchmod(fd, 0400) == 0, "chmod 0400");
        check_fails(name("file"), O_RDONLY | O_TRUNC, EACCES, "EACCES O_TRUNC");
        check(fchmod(fd, 0644) == 0, "chmod back");
        check(chmod(dir, 0500) == 0, "remove write permission");
        check_fails(name("new"), O_WRONLY | O_CREAT, EACCES, "EACCES creating");
        check(chmod(dir, 0600) == 0, "remove search permission");
        check_fails(name("file"), O_RDONLY, EACCES, "EACCES search");
        errno = 0;
        check(openat(dfd, "file", O_RDONLY) == -1 && errno == EACCES, "openat EACCES search");
        check(chmod(dir, 0700) == 0, "restore permission");
    }
    /* No file is made by a failed open */
    check(lstat(name("new"), &st) == -1 && errno == ENOENT, "nothing created");

    /* EMFILE once every descriptor is in use */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl = { 16, 16 };

        if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
            _exit(1);
        while ((fd2 = open(name("file"), O_RDONLY)) >= 0)
            if (fd2 >= 16)
                _exit(2);
        _exit(errno == EMFILE ? 0 : 3);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "EMFILE");

    check(close(fd) == 0, "close");
    check(unlink(name("fifo")) == 0 && unlink(name("loop")) == 0 && unlink(name("dangling")) == 0
              && unlink(name("link")) == 0 && unlink(name("file")) == 0,
          "unlink");
    check(close(dfd) == 0 && rmdir(dir) == 0, "clean up");
    umask(old);
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
