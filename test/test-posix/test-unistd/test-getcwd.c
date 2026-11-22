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
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_SYMBOLIC_LINKS && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * getcwd, chdir and fchdir: absolute names without dot, dot-dot or
 * symbolic links, the buffer size limits, and errors that leave the
 * working directory alone. The directory changes happen in a child.
 */

static char dir[] = "tmp-XXXXXX";
static char top[4096];

#define check(condition, message)                                                      \
    do {                                                                               \
        if (!(condition)) {                                                            \
            printf("%d: %s: %s (errno %d)\n", __LINE__, (message), #condition, errno); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define check_err(expr, err, message) check((errno = 0, (expr) == -1) && errno == (err), message)

/* The working directory is still 'want' */
static void
check_cwd(const char *want, int line)
{
    static char buf[4096];

    if (getcwd(buf, sizeof(buf)) == NULL || strcmp(buf, want) != 0) {
        printf("%d: working directory is %s, expected %s\n", line, buf, want);
        exit(1);
    }
}

static char long_name[NAME_MAX + 2];

static void
child(void)
{
    static char sub[4096], buf[4096], fit[4096];
    size_t      len;
    char       *ret;
    int         fd, ffd, i;

    check(chdir(dir) == 0, "chdir to the scratch directory");
    check(getcwd(sub, sizeof(sub)) != NULL, "getcwd");
    len = strlen(top);
    check(strncmp(sub, top, len) == 0 && sub[len] == '/' && strcmp(sub + len + 1, dir) == 0,
          "absolute name of the new directory");

    /* No dot, dot-dot, symbolic links or extra slashes */
    check(mkdir("real", 0700) == 0 && symlink("real", "sym") == 0, "directory and link");
    check(chdir("sym/./../sym//") == 0, "chdir through a link, dot and dot-dot");
    check(getcwd(buf, sizeof(buf)) == buf, "getcwd returns buf");
    check(strncmp(buf, sub, strlen(sub)) == 0 && strcmp(buf + strlen(sub), "/real") == 0,
          "the real directory's name");
    check(buf[0] == '/' && buf[1] != '/', "a single leading slash");
    for (i = 0; buf[i]; i++)
        check(!(buf[i] == '/' && buf[i + 1] == '/'), "no doubled slashes");
    check(strstr(buf, "/./") == NULL && strstr(buf, "/../") == NULL, "no dot or dot-dot");

    /* The size: exactly the length plus the null, else ERANGE; 0 is EINVAL */
    len = strlen(buf);
    memset(fit, 0, sizeof(fit));
    check(getcwd(fit, len + 1) == fit && strcmp(fit, buf) == 0, "an exact fit");
    errno = 0;
    check(getcwd(fit, len) == NULL && errno == ERANGE, "one byte short");
    errno = 0;
    check(getcwd(fit, 1) == NULL && errno == ERANGE, "one byte");
    errno = 0;
    ret = getcwd(fit, 0);
    check(ret == NULL && errno == EINVAL, "a zero size");

    /* chdir errors leave the directory alone */
    check(close(open("file", O_WRONLY | O_CREAT, 0600)) == 0, "create file");
    check_err(chdir("missing"), ENOENT, "missing");
    check_err(chdir(""), ENOENT, "empty path");
    check_err(chdir("file"), ENOTDIR, "a file");
    check_err(chdir("file/x"), ENOTDIR, "prefix not a directory");
    check(symlink("loop", "loop") == 0, "symlink loop");
    check_err(chdir("loop"), ELOOP, "loop");
    memset(long_name, 'n', NAME_MAX + 1);
    check_err(chdir(long_name), ENAMETOOLONG, "long component");
    check_cwd(buf, __LINE__);
    if (geteuid() != 0) {
        check(mkdir("closed", 0600) == 0, "directory without search permission");
        check_err(chdir("closed"), EACCES, "no search permission");
        check_cwd(buf, __LINE__);
    }

    /* fchdir: to a directory descriptor; errors leave the directory alone */
    fd = open("..", O_RDONLY | O_DIRECTORY);
    check(fd >= 0, "open dot-dot");
    check(fchdir(fd) == 0, "fchdir");
    check_cwd(sub, __LINE__);
    check(chdir("real") == 0, "chdir back");
    ffd = open("file", O_RDONLY);
    check(ffd >= 0, "open file");
    check_err(fchdir(ffd), ENOTDIR, "fchdir to a file");
    check(close(ffd) == 0, "close");
    check_err(fchdir(ffd), EBADF, "fchdir to a closed descriptor");
    check_err(fchdir(-1), EBADF, "fchdir to -1");
    check_cwd(buf, __LINE__);
    if (geteuid() != 0) {
        ffd = open("closed", O_RDONLY | O_DIRECTORY);
        if (ffd < 0) {
            /* No read permission either; open it before removing search */
            check(chmod("closed", 0700) == 0, "chmod");
            ffd = open("closed", O_RDONLY | O_DIRECTORY);
            check(ffd >= 0 && chmod("closed", 0600) == 0, "open closed");
        }
        check_err(fchdir(ffd), EACCES, "fchdir without search permission");
        check(close(ffd) == 0, "close");
        check_cwd(buf, __LINE__);
        check(rmdir("closed") == 0, "rmdir");
    }
    check(fchdir(fd) == 0 && close(fd) == 0, "fchdir");

    /* Remove everything from inside */
    check(unlink("real/file") == 0 && unlink("real/loop") == 0 && rmdir("real") == 0
              && unlink("sym") == 0,
          "cleanup");
    _exit(0);
}

int
main(void)
{
    pid_t pid;
    int   status;

    check(getcwd(top, sizeof(top)) != NULL && top[0] == '/', "getcwd");
#if defined(__PICOLIBC__) || defined(TEST_GLIBC)
    /* Not POSIX: a null buffer is allocated, as big as needed for size 0 */
    {
        char *alloc = getcwd(NULL, 0);

        check(alloc != NULL && strcmp(alloc, top) == 0, "getcwd(NULL, 0)");
        free(alloc);
        alloc = getcwd(NULL, strlen(top) + 1);
        check(alloc != NULL && strcmp(alloc, top) == 0, "getcwd(NULL, size)");
        free(alloc);
        errno = 0;
        check(getcwd(NULL, 1) == NULL && errno == ERANGE, "getcwd(NULL, too small)");
    }
#endif
    check(mkdtemp(dir) != NULL, "scratch directory");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        child();
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    check_cwd(top, __LINE__);
    (void)rmdir(dir);
    check(WEXITSTATUS(status) == 0, "child status");
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
