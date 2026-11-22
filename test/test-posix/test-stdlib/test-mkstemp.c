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
/* glibc declares mkostemp only with _GNU_SOURCE */
#ifdef TEST_GLIBC
#define _GNU_SOURCE
#endif
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FILE_SYSTEM_EXT && TEST_POSIX_FD_MGMT \
    && TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FIFO && TEST_POSIX_PIPE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * mkstemp, mkostemp and mkdtemp, under a fresh mkdtemp directory:
 * the names, the modes and flags, the errors, FIFOs in the way, and
 * that they leave rand's sequence alone.
 */

static char dir[] = "test-mkstemp-XXXXXX";

static void
cleanup(void)
{
    DIR           *d = opendir(dir);
    struct dirent *e;
    char           path[sizeof(dir) + 300];

    if (d) {
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
            if (unlink(path) != 0)
                (void)rmdir(path);
        }
        closedir(d);
    }
    (void)rmdir(dir);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The characters of the portable filename character set */
static int
portable(const char *s)
{
    for (; *s; s++)
        if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9')
              || *s == '.' || *s == '_' || *s == '-'))
            return 0;
    return 1;
}

/* Check a name made from dir/<prefix>XXXXXX... */
static void
check_name(const char *name, const char *prefix, size_t nx)
{
    size_t dl = strlen(dir) + 1;
    size_t pl = strlen(prefix);

    check(strncmp(name, dir, dl - 1) == 0 && name[dl - 1] == '/', "the directory is kept");
    check(strncmp(name + dl, prefix, pl) == 0, "the prefix is kept");
    check(strlen(name + dl + pl) == nx, "the same number of characters");
    check(portable(name + dl + pl), "portable filename characters");
}

/* In a child, run mkstemp on the template and send back the name */
static void
child_name(const char *tmpl, char *name, size_t size)
{
    int     p[2];
    pid_t   pid;
    int     status;
    ssize_t n;

    check(pipe(p) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        char t[256];
        int  fd;

        close(p[0]);
        alarm(20);
        strcpy(t, tmpl);
        fd = mkstemp(t);
        if (fd < 0)
            _exit(2);
        if (write(p[1], t, strlen(t) + 1) < 0)
            _exit(3);
        _exit(0);
    }
    close(p[1]);
    n = read(p[0], name, size);
    close(p[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "mkstemp in a child, without hanging");
    check(n > 0 && name[n - 1] == '\0', "the child's name");
}

int
main(void)
{
    char        path[sizeof(dir) + 64];
    char        tmpl[sizeof(dir) + 64];
    char        first[sizeof(dir) + 64];
    char        second[sizeof(dir) + 64];
    struct stat st;
    int         fd;
    int         flags;
    int         i, j;
    int         r1, r2;

    check(mkdtemp(dir) == dir, "mkdtemp");
    (void)umask(0);

    /* mkstemp: a new regular file, open for reading and writing, mode 0600 */
    snprintf(path, sizeof(path), "%s/fileXXXXXX", dir);
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp");
    check_name(path, "file", 6);
    check(fstat(fd, &st) == 0 && S_ISREG(st.st_mode), "a regular file");
    check((st.st_mode & 07777) == (S_IRUSR | S_IWUSR), "mode 0600");
    check(st.st_size == 0, "empty");
    flags = fcntl(fd, F_GETFL);
    check(flags >= 0 && (flags & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDWR, "O_RDWR");
    check(stat(path, &st) == 0 && S_ISREG(st.st_mode), "the file has that name");
    check(write(fd, "x", 1) == 1 && lseek(fd, 0, SEEK_SET) == 0, "write");
    {
        char c;
        check(read(fd, &c, 1) == 1 && c == 'x', "read");
    }
    close(fd);

    /* More than six Xs are all replaced with the same number of characters */
    snprintf(path, sizeof(path), "%s/longXXXXXXXXXX", dir);
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp with ten Xs");
    check_name(path, "long", 10);
    close(fd);

    /* The template is only Xs */
    snprintf(path, sizeof(path), "%s/XXXXXX", dir);
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp with only Xs");
    check_name(path, "", 6);
    close(fd);

    /* Every name is new */
    {
        static char names[64][sizeof(dir) + 64];

        for (i = 0; i < 64; i++) {
            snprintf(names[i], sizeof(names[i]), "%s/uXXXXXX", dir);
            fd = mkstemp(names[i]);
            check(fd >= 0, "mkstemp many");
            close(fd);
            for (j = 0; j < i; j++)
                check(strcmp(names[i], names[j]) != 0, "distinct names");
        }
    }

#if TEST_POSIX_FILE_SYSTEM >= 202405L
    /* mkostemp: the flags it accepts */
    snprintf(path, sizeof(path), "%s/appendXXXXXX", dir);
    fd = mkostemp(path, O_APPEND);
    check(fd >= 0, "mkostemp O_APPEND");
    check_name(path, "append", 6);
    flags = fcntl(fd, F_GETFL);
    check(flags >= 0 && (flags & O_APPEND) && (flags & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDWR,
          "O_APPEND and O_RDWR");
    check(fcntl(fd, F_GETFD) == 0, "no FD_CLOEXEC");
    check(fstat(fd, &st) == 0 && (st.st_mode & 07777) == (S_IRUSR | S_IWUSR), "mode 0600");
    close(fd);

    snprintf(path, sizeof(path), "%s/cloexecXXXXXX", dir);
    fd = mkostemp(path, O_CLOEXEC);
    check(fd >= 0, "mkostemp O_CLOEXEC");
    check(fcntl(fd, F_GETFD) == FD_CLOEXEC, "FD_CLOEXEC");
    flags = fcntl(fd, F_GETFL);
    check(flags >= 0 && !(flags & O_APPEND), "no O_APPEND");
    close(fd);

    snprintf(path, sizeof(path), "%s/syncXXXXXX", dir);
    fd = mkostemp(path, O_SYNC);
    check(fd >= 0, "mkostemp O_SYNC");
    flags = fcntl(fd, F_GETFL);
    check(flags >= 0 && (flags & O_SYNC) == O_SYNC, "O_SYNC");
    close(fd);
#endif

    /* mkdtemp: a directory with mode 0700 */
    snprintf(path, sizeof(path), "%s/dirXXXXXXX", dir);
    check(mkdtemp(path) == path, "mkdtemp");
    check_name(path, "dir", 7);
    check(stat(path, &st) == 0 && S_ISDIR(st.st_mode), "a directory");
    {
        struct stat parent;

        /* A new directory may inherit a set-group-ID parent's bit */
        check(stat(dir, &parent) == 0, "stat parent");
        check((st.st_mode & 07777) == (S_IRWXU | (parent.st_mode & S_ISGID)), "mode 0700");
    }

    /* Fewer than six trailing Xs: EINVAL */
    snprintf(path, sizeof(path), "%s/shortXXXXX", dir);
    strcpy(tmpl, path);
    errno = 0;
    check(mkstemp(path) == -1 && errno == EINVAL, "mkstemp with five Xs");
#if TEST_POSIX_FILE_SYSTEM >= 202405L
    errno = 0;
    check(mkostemp(path, 0) == -1 && errno == EINVAL, "mkostemp with five Xs");
#endif
    errno = 0;
    check(mkdtemp(path) == NULL && errno == EINVAL, "mkdtemp with five Xs");
    snprintf(path, sizeof(path), "%s/XXXXXXa", dir);
    errno = 0;
    check(mkstemp(path) == -1 && errno == EINVAL, "Xs not at the end");

    /* The errors of open and mkdir */
    snprintf(path, sizeof(path), "%s/missing/fileXXXXXX", dir);
    errno = 0;
    check(mkstemp(path) == -1 && errno == ENOENT, "mkstemp in a missing directory");
    snprintf(path, sizeof(path), "%s/missing/dirXXXXXX", dir);
    errno = 0;
    check(mkdtemp(path) == NULL && errno == ENOENT, "mkdtemp in a missing directory");
    snprintf(path, sizeof(path), "%s/plain", dir);
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create a plain file");
    close(fd);
    snprintf(path, sizeof(path), "%s/plain/fileXXXXXX", dir);
    errno = 0;
    check(mkstemp(path) == -1 && errno == ENOTDIR, "mkstemp under a file");

    /*
     * A FIFO with the name mkstemp tries first doesn't stop it. Two
     * children start from the same generator state, so the second
     * tries the first one's name first, now a FIFO.
     */
    snprintf(tmpl, sizeof(tmpl), "%s/fifoXXXXXX", dir);
    child_name(tmpl, first, sizeof(first));
    check(unlink(first) == 0 && mkfifo(first, 0600) == 0, "make a FIFO with that name");
    child_name(tmpl, second, sizeof(second));
    check(strcmp(first, second) != 0, "a different name");
    check(stat(first, &st) == 0 && S_ISFIFO(st.st_mode), "the FIFO is still there");

    /* POSIX: as if no function calls rand or srand */
    srand(7);
    r1 = rand();
    srand(7);
    snprintf(path, sizeof(path), "%s/randXXXXXX", dir);
    fd = mkstemp(path);
    check(fd >= 0, "mkstemp between srand and rand");
    close(fd);
    r2 = rand();
    check(r1 == r2, "mkstemp leaves rand's sequence alone");

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
