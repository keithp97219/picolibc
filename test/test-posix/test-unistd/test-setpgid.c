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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE) \
    && defined(__PICOLIBC_POSIX_SYMBOLIC_LINKS)

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * setpgid moves a process into a new or existing group of the
 * session; it refuses session leaders, other sessions, exec'd
 * children and processes that aren't the caller's children
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define check_err(call, err)                                                           \
    do {                                                                               \
        errno = 0;                                                                     \
        int _r = (call);                                                               \
        if (_r != -1 || errno != (err)) {                                              \
            printf("%s: returned %d errno %d, expected %s\n", #call, _r, errno, #err); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

/* A child that runs fn, then waits until the pipe closes */
struct child {
    pid_t pid;
    int   fd;
};

static int
helper_wait(int fd)
{
    char c;

    return read(fd, &c, 1) < 0;
}

static struct child
start(int (*fn)(int report))
{
    struct child child;
    int          hold[2], report[2];
    char         c;

    check(pipe(hold) == 0 && pipe(report) == 0, "pipe");
    fflush(stdout);
    child.pid = fork();
    check(child.pid >= 0, "fork");
    if (child.pid == 0) {
        (void)close(hold[1]);
        (void)close(report[0]);
        if (fn && fn(report[1]) != 0)
            _exit(2);
        (void)write(report[1], "r", 1);
        _exit(helper_wait(hold[0]));
    }
    (void)close(hold[0]);
    (void)close(report[1]);
    /* Wait until fn has run */
    check(read(report[0], &c, 1) == 1, "child ready");
    (void)close(report[0]);
    child.fd = hold[1];
    return child;
}

static void
finish(struct child child)
{
    int status;

    check(close(child.fd) == 0, "close");
    check(waitpid(child.pid, &status, 0) == child.pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
}

static int
new_session(int report)
{
    (void)report;
    return setsid() < 0;
}

static int
session_leader_self(int report)
{
    (void)report;
    if (setsid() < 0)
        return 1;
    /* A session leader can't change its group */
    errno = 0;
    if (setpgid(0, 0) != -1 || errno != EPERM)
        return 1;
    return getpgrp() != getpid();
}

static char exe[PATH_MAX];

int
main(int argc, char **argv)
{
    struct child a, b, c;
    pid_t        pgrp = getpgrp(), sid = getsid(0), g;
    int          hold[2], ready[2], status;
    char         ch;
    ssize_t      len;

    if (argc > 2 && strcmp(argv[1], "wait") == 0) {
        /* The exec'd child: say it is running, then wait */
        (void)write(atoi(argv[2]), "r", 1);
        return helper_wait(0);
    }

    /* A child can be given its own group, then put back */
    a = start(NULL);
    check(setpgid(a.pid, 0) == 0, "setpgid(pid, 0)");
    check(getpgid(a.pid) == a.pid, "a new group named for the child");
    check(setpgid(a.pid, pgrp) == 0, "back to the parent's group");
    check(getpgid(a.pid) == pgrp, "in the parent's group");
    check(setpgid(a.pid, a.pid) == 0 && getpgid(a.pid) == a.pid, "setpgid(pid, pid)");
    /* The session stays the same */
    check(getsid(a.pid) == sid, "session unchanged");

    /* A negative group */
    check_err(setpgid(a.pid, -1), EINVAL);
    /* A group that doesn't exist in this session */
    check_err(setpgid(a.pid, INT_MAX), EPERM);

    /* A child in another session, and that session's group */
    b = start(new_session);
    check_err(setpgid(b.pid, 0), EPERM);
    check_err(setpgid(a.pid, b.pid), EPERM);
    finish(b);

    /* A session leader can't move itself */
    b = start(session_leader_self);
    finish(b);

    /* pid 0 is the caller */
    fflush(stdout);
    g = fork();
    check(g >= 0, "fork");
    if (g == 0) {
        if (setpgid(0, 0) != 0 || getpgrp() != getpid())
            _exit(1);
        _exit(0);
    }
    check(waitpid(g, &status, 0) == g && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "setpgid(0, 0)");

    /* Neither this process nor one of its children */
    check_err(setpgid(INT_MAX, 0), ESRCH);
    finish(a);

    /* A grandchild isn't a child; it exits when this process closes hold */
    check(pipe(hold) == 0 && pipe(ready) == 0, "pipe");
    fflush(stdout);
    c.pid = fork();
    check(c.pid >= 0, "fork");
    if (c.pid == 0) {
        (void)close(hold[1]);
        (void)close(ready[0]);
        g = fork();
        if (g < 0)
            _exit(1);
        if (g == 0) {
            (void)close(ready[1]);
            _exit(helper_wait(hold[0]));
        }
        (void)write(ready[1], &g, sizeof(g));
        _exit(0);
    }
    (void)close(hold[0]);
    (void)close(ready[1]);
    check(read(ready[0], &g, sizeof(g)) == sizeof(g), "grandchild pid");
    check(waitpid(c.pid, &status, 0) == c.pid, "waitpid");
    check_err(setpgid(g, 0), ESRCH);
    check(close(hold[1]) == 0 && close(ready[0]) == 0, "close");

    /* A child that has run an exec function */
    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len <= 0) {
        printf("no /proc/self/exe, skipping the EACCES check\n");
    } else {
        char fdname[16];

        exe[len] = '\0';
        check(pipe(hold) == 0 && pipe(ready) == 0, "pipe");
        fflush(stdout);
        g = fork();
        check(g >= 0, "fork");
        if (g == 0) {
            (void)close(hold[1]);
            (void)close(ready[0]);
            if (dup2(hold[0], 0) != 0)
                _exit(1);
            snprintf(fdname, sizeof(fdname), "%d", ready[1]);
            execl(exe, exe, "wait", fdname, (char *)NULL);
            _exit(1);
        }
        (void)close(hold[0]);
        (void)close(ready[1]);
        check(read(ready[0], &ch, 1) == 1, "exec'd child ready");
        check_err(setpgid(g, 0), EACCES);
        check(close(hold[1]) == 0 && close(ready[0]) == 0, "close");
        check(waitpid(g, &status, 0) == g && WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "exec'd child");
    }

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
