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
#if !defined(__PICOLIBC__)                                                                  \
    || ((defined(_POSIX_SPAWN) && _POSIX_SPAWN > 0)                                         \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)      \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0))

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* posix_spawn runs a program and reports failures to start it */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

extern char **environ;

/* Runs 'script' with /bin/sh and returns its wait status */
static int
run(const char *script, const posix_spawn_file_actions_t *fa, const posix_spawnattr_t *attr)
{
    char *argv[] = { "sh", "-c", (char *)script, NULL };
    pid_t pid;
    int   status, ret;

    ret = posix_spawn(&pid, "/bin/sh", fa, attr, argv, environ);
    check(ret == 0, "posix_spawn");
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

static int
exited(int status, int code)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

/* The <spawn.h> prototypes, with the qualifiers POSIX gives them */
static int (* const spawn_fn)(pid_t * restrict, const char * restrict,
                              const posix_spawn_file_actions_t * restrict,
                              const posix_spawnattr_t * restrict, char * const[restrict],
                              char * const[restrict])
    = posix_spawn;
static int (* const getsigmask_fn)(const posix_spawnattr_t * restrict, sigset_t * restrict)
    = posix_spawnattr_getsigmask;
static int (* const setpgroup_fn)(posix_spawnattr_t *, pid_t) = posix_spawnattr_setpgroup;
static int (* const addopen_fn)(posix_spawn_file_actions_t * restrict, int, const char * restrict,
                                int, mode_t)
    = posix_spawn_file_actions_addopen;

static const short spawn_flags[] = {
    POSIX_SPAWN_RESETIDS,     POSIX_SPAWN_SETPGROUP, POSIX_SPAWN_SETSCHEDPARAM,
    POSIX_SPAWN_SETSCHEDULER, POSIX_SPAWN_SETSIGDEF, POSIX_SPAWN_SETSIGMASK,
/* glibc only defines POSIX_SPAWN_SETSID for _GNU_SOURCE */
#if defined(__PICOLIBC__) || defined(POSIX_SPAWN_SETSID)
    POSIX_SPAWN_SETSID,
#endif
};

#define NFLAGS (sizeof(spawn_flags) / sizeof(spawn_flags[0]))

static void
caught(int sig)
{
    (void)sig;
}

static char dir[] = "tmp-XXXXXX";

/* Creates 'name' holding 'contents' with permissions 'mode' */
static void
make_file(const char *name, const char *contents, mode_t mode)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, mode);

    check(fd >= 0, "create");
    check(write(fd, contents, strlen(contents)) == (ssize_t)strlen(contents), "write");
    check(close(fd) == 0, "close");
}

int
main(void)
{
    char    *argv[] = { "nothing", NULL };
    char    *args[] = { "sh", "-c", "test \"$0$1:$FOO\" = onetwo:bar", "one", "two", NULL };
    char    *envp[] = { "FOO=bar", NULL };
    pid_t    pid = 0;
    sigset_t set, old;
    unsigned i, j;
    int      status;

    for (i = 0; i < NFLAGS; i++) {
        check(spawn_flags[i] != 0, "flag non-zero");
        for (j = 0; j < i; j++)
            check((spawn_flags[i] & spawn_flags[j]) == 0, "flags distinct");
    }
    (void)spawn_fn;
    (void)getsigmask_fn;
    (void)setpgroup_fn;
    (void)addopen_fn;

    if (access("/bin/sh", X_OK) != 0) {
        printf("no /bin/sh, skipping\n");
        return 77;
    }
    check(exited(run("exit 7", NULL, NULL), 7), "exit status");
    check(exited(run("test \"$SPAWN_TEST\" = yes", NULL, NULL), 1), "environment");
    check(setenv("SPAWN_TEST", "yes", 1) == 0, "setenv");
    check(exited(run("test \"$SPAWN_TEST\" = yes", NULL, NULL), 0), "environment passed");
    check(posix_spawn(&pid, "/nonexistent/program", NULL, NULL, argv, environ) == ENOENT, "ENOENT");
    /* A failure leaves no child behind */
    check(waitpid(-1, &status, WNOHANG) < 0 && errno == ECHILD, "no child");

    /* argv and envp reach the new process image */
    check(posix_spawn(&pid, "/bin/sh", NULL, NULL, args, envp) == 0, "argv and envp");
    check(waitpid(pid, &status, 0) == pid && exited(status, 0), "argv and envp seen");

    /* With a null pid, the child still runs */
    check(posix_spawn(NULL, "/bin/sh", NULL, NULL, args, envp) == 0, "null pid");
    check(wait(&status) > 0 && exited(status, 0), "null pid child");

    /* Exec failures are reported, with no fallback to the shell */
    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    make_file("noexec", "exit 0\n", 0700);
    make_file("noperm", "exit 0\n", 0600);
    check(posix_spawn(&pid, "noexec", NULL, NULL, argv, environ) == ENOEXEC, "ENOEXEC");
    if (geteuid() != 0)
        check(posix_spawn(&pid, "noperm", NULL, NULL, argv, environ) == EACCES, "EACCES");
    check(unlink("noexec") == 0 && unlink("noperm") == 0, "unlink");
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");

    /* The child inherits the signal mask */
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    check(sigprocmask(SIG_BLOCK, &set, &old) == 0, "block");
    check(exited(run("kill -USR1 $$; exit 3", NULL, NULL), 3), "mask inherited");
    check(sigprocmask(SIG_SETMASK, &old, NULL) == 0, "unblock");

    /* A caught signal has the default action in the child */
    check(signal(SIGUSR1, caught) != SIG_ERR, "catch SIGUSR1");
    status = run("kill -USR1 $$; exit 3", NULL, NULL);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1, "caught becomes default");
    check(signal(SIGUSR1, SIG_DFL) != SIG_ERR, "default SIGUSR1");
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
