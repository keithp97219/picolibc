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
#if !defined(__PICOLIBC__)                                                              \
    || ((defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0) \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0))

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* the exec functions run a program, searching PATH for the p variants and running scripts with the
 * shell */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

enum how { EXECL, EXECLP, EXECV, EXECVP };

static char * const sh_argv[] = { "sh", "-c", "exit 3", NULL };

/* Runs file in a child the given way, returning its exit status or 100 + errno on failure */
static int
run(enum how how, const char *file)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        switch (how) {
        case EXECL:
            execl(file, "sh", "-c", "exit 3", (char *)NULL);
            break;
        case EXECLP:
            execlp(file, "sh", "-c", "exit 3", (char *)NULL);
            break;
        case EXECV:
            execv(file, sh_argv);
            break;
        case EXECVP:
            execvp(file, sh_argv);
            break;
        }
        _exit(100 + errno);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status), "child");
    return WEXITSTATUS(status);
}

int
main(void)
{
    char dir[] = "execvp-XXXXXX";
    char script[64], plain[64], path[128];
    int  fd;

    check(run(EXECL, "/bin/sh") == 3, "execl");
    check(run(EXECV, "/bin/sh") == 3, "execv");
    check(run(EXECLP, "sh") == 3, "execlp searches PATH");
    check(run(EXECVP, "sh") == 3, "execvp searches PATH");
    /* Without the search, a name with no slash is relative to the working directory */
    check(run(EXECV, "sh") == 100 + ENOENT, "execv doesn't search");

    /* A file without a #! line is run by the shell for the p variants */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(script, sizeof(script), "%s/script", dir);
    fd = open(script, O_WRONLY | O_CREAT | O_EXCL, 0755);
    check(fd >= 0, "create script");
    check(write(fd, "exit 5\n", 7) == 7 && close(fd) == 0, "write script");
    snprintf(path, sizeof(path), "/nonexistent:%s", dir);
    check(setenv("PATH", path, 1) == 0, "setenv");
    check(run(EXECVP, "script") == 5, "script via PATH");
    check(run(EXECLP, "script") == 5, "execlp script");
    check(run(EXECVP, "missing") == 100 + ENOENT, "ENOENT");

    /* A file without execute permission */
    snprintf(plain, sizeof(plain), "%s/plain", dir);
    fd = open(plain, O_WRONLY | O_CREAT | O_EXCL, 0644);
    check(fd >= 0 && close(fd) == 0, "create plain");
    check(run(EXECV, plain) == 100 + EACCES, "EACCES");
    check(run(EXECVP, "plain") == 100 + EACCES, "execvp EACCES");

    check(unlink(script) == 0 && unlink(plain) == 0 && rmdir(dir) == 0, "clean up");
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
