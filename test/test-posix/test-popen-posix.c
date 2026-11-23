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

#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-popen-posix.txt"
#endif

/*
 * popen and pclose: the pipe ends and their flags, earlier streams closed
 * in the child, the shell's arguments, exit status 127, EINTR in pclose
 * and other children left alone. Every command touches only the test's
 * own directory.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char line[256];

static const char *
read_line(FILE *f)
{
    if (!fgets(line, sizeof(line), f))
        return "";
    return line;
}

static void
check_exit(int status, int code, const char *message)
{
    if (!WIFEXITED(status) || WEXITSTATUS(status) != code)
        printf("status 0x%x, expected exit %d\n", status, code);
    check(WIFEXITED(status) && WEXITSTATUS(status) == code, message);
}

static volatile sig_atomic_t released;

/* Let the command waiting on the FIFO finish */
static void
on_alarm(int sig)
{
    int fd;

    (void)sig;
    fd = open("fifo", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        alarm(1);
        return;
    }
    (void)!write(fd, "x\n", 2);
    close(fd);
    released = 1;
}

int
main(void)
{
    const char      *dir = TEST_FILE_NAME;
    char             cwd[PATH_MAX];
    char             cmd[PATH_MAX + 64];
    FILE            *p, *q;
    int              status;
    pid_t            other;
    struct sigaction sa;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check((mkdir(dir, 0700) == 0 || errno == EEXIST) && chdir(dir) == 0, "scratch directory");
    check(getcwd(cwd, sizeof(cwd)) != NULL, "getcwd");

    /* 'r': the command's standard output is the pipe */
    p = popen("echo hello", "r");
    check(p != NULL, "popen r");
    /* The pipe may also be open the other way (FreeBSD's pipes are) */
    check((fcntl(fileno(p), F_GETFL) & O_ACCMODE) == O_RDONLY
              || (fcntl(fileno(p), F_GETFL) & O_ACCMODE) == O_RDWR,
          "readable end");
    check(!(fcntl(fileno(p), F_GETFD) & FD_CLOEXEC), "no FD_CLOEXEC without e");
    check(strcmp(read_line(p), "hello\n") == 0, "read the output");
    check(pclose(p) == 0, "pclose r");

    /* 'w': the command's standard input is the pipe */
    p = popen("cat > out", "w");
    check(p != NULL, "popen w");
    /* The pipe may also be open the other way (FreeBSD's pipes are) */
    check((fcntl(fileno(p), F_GETFL) & O_ACCMODE) == O_WRONLY
              || (fcntl(fileno(p), F_GETFL) & O_ACCMODE) == O_RDWR,
          "writable end");
    check(fputs("to the command\n", p) >= 0, "write");
    check(pclose(p) == 0, "pclose w");
    q = fopen("out", "r");
    check(q != NULL && strcmp(read_line(q), "to the command\n") == 0, "command read the input");
    check(fclose(q) == 0, "close out");

    /* pclose returns the shell's termination status */
    p = popen("exit 3", "w");
    check(p != NULL, "popen exit");
    status = pclose(p);
    check_exit(status, 3, "exit status");

    /* 'e' sets FD_CLOEXEC on the stream's descriptor */
    p = popen("true", "re");
    check(p != NULL && (fcntl(fileno(p), F_GETFD) & FD_CLOEXEC), "re");
    check(pclose(p) == 0, "pclose re");
    p = popen("cat > /dev/null", "we");
    check(p != NULL && (fcntl(fileno(p), F_GETFD) & FD_CLOEXEC), "we");
    check(pclose(p) == 0, "pclose we");

    /* Streams from earlier popen calls are closed in the new child */
    p = popen("cat > one", "w");
    check(p != NULL, "popen first");
    snprintf(cmd, sizeof(cmd), "if [ -e /dev/fd/%d ]; then echo open; else echo closed; fi",
             fileno(p));
    q = popen(cmd, "r");
    check(q != NULL, "popen second");
    check(strcmp(read_line(q), "closed\n") == 0, "earlier stream closed in the child");
    check(pclose(q) == 0, "pclose second");
    check(pclose(p) == 0, "pclose first");

    /* The command may start with '-' */
    q = fopen("-cmd", "w");
    check(q != NULL && fputs("#!/bin/sh\necho ran\n", q) >= 0 && fclose(q) == 0, "script");
    check(chmod("-cmd", 0755) == 0, "chmod script");
    snprintf(cmd, sizeof(cmd), "%s:%s", cwd, getenv("PATH") ? getenv("PATH") : "/bin:/usr/bin");
    check(setenv("PATH", cmd, 1) == 0, "setenv PATH");
    p = popen("-cmd", "r");
    check(p != NULL, "popen -cmd");
    check(strcmp(read_line(p), "ran\n") == 0, "command starting with -");
    check(pclose(p) == 0, "pclose -cmd");

    /*
     * A shell that can't be run gives status 127; a long enough
     * environment string makes execve fail
     */
    other = fork();
    check(other >= 0, "fork");
    if (other == 0) {
        /* More than Linux allows in one string, whatever ARG_MAX is */
        long  arg_max = 1024L * 1024;
        char *big;

        big = malloc((size_t)arg_max + 1);
        if (big == NULL)
            _exit(2);
        memset(big, 'b', (size_t)arg_max);
        big[arg_max] = '\0';
        if (setenv("BIG", big, 1) != 0)
            _exit(2);
        p = popen("true", "r");
        if (p == NULL) {
#ifdef __GLIBC__
            /* glibc reports the failed exec from popen */
            _exit(0);
#else
            _exit(3);
#endif
        }
        status = pclose(p);
        _exit(WIFEXITED(status) && WEXITSTATUS(status) == 127 ? 0 : 4);
    }
    check(waitpid(other, &status, 0) == other, "waitpid");
    check_exit(status, 0, "status 127 when the shell can't be run");

    /* pclose waits for the command, even when a signal interrupts it */
    /* With the shell's mkfifo, as the C library may not have one */
    (void)unlink("fifo");
    check(system("mkfifo fifo") == 0, "mkfifo");
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART */
    check(sigaction(SIGALRM, &sa, NULL) == 0, "sigaction");
    p = popen("read x < fifo", "w");
    check(p != NULL, "popen fifo");
    alarm(1);
    errno = 0;
    status = pclose(p);
    check(released, "the alarm released the command");
    check(status == 0, "pclose waits through EINTR");

    /* pclose leaves other children alone */
    other = fork();
    check(other >= 0, "fork");
    if (other == 0)
        _exit(5);
    p = popen("exit 2", "r");
    check(p != NULL, "popen with another child");
    status = pclose(p);
    check_exit(status, 2, "pclose's status");
    check(waitpid(other, &status, 0) == other, "the other child is still there");
    check_exit(status, 5, "the other child's status");

    check(unlink("out") == 0 && unlink("one") == 0 && unlink("-cmd") == 0 && unlink("fifo") == 0,
          "cleanup");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
}
