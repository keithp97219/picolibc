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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_SYMBOLIC_LINKS

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "../is-qemu.h"

/*
 * sysconf(_SC_ARG_MAX) is the real limit on exec arguments: with
 * different stack limits, arguments a little under it reach the new
 * image, and a little over it give E2BIG. The new image is this
 * program again, through /proc/self/exe.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

#define CHILD_MODE "arg-max-child"
/* Each argument; the kernel limits single strings to 128KiB */
#define CHUNK (16 * 1024)
/* How far inside or outside the limit the arguments are */
#define MARGIN (8 * 1024)

static char exe[PATH_MAX];

/*
 * In a child with the stack limit 'stack' (0 to keep the current one),
 * exec this program with arguments taking sysconf(_SC_ARG_MAX) + delta
 * bytes, counting the strings, their terminators and the argv
 * pointers, with an empty environment. Stores the child's
 * sysconf(_SC_ARG_MAX) and returns its exit status: 0 when the new
 * image ran, 100 for E2BIG, others for other failures.
 */
static int
exec_with(rlim_t stack, long delta, long *arg_max)
{
    int   status, pipefd[2];
    pid_t pid;
    long  reported = 0;

    check(pipe(pipefd) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct rlimit rl;
        long          max, total, n, i;
        char        **argv;
        char         *chunk;
        char         *envp[] = { NULL };

        close(pipefd[0]);
        if (stack != 0) {
            if (getrlimit(RLIMIT_STACK, &rl) != 0)
                _exit(120);
            if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < stack)
                _exit(121);
            rl.rlim_cur = stack;
            if (setrlimit(RLIMIT_STACK, &rl) != 0)
                _exit(122);
        }
        max = sysconf(_SC_ARG_MAX);
        if (write(pipefd[1], &max, sizeof(max)) != (ssize_t)sizeof(max))
            _exit(123);
        if (max < 0)
            _exit(0);
        total = max + delta;
        /* argv[0], the mode, then chunks; each costs its bytes and a pointer */
        n = total / (CHUNK + (long)sizeof(char *)) + 3;
        argv = calloc((size_t)n + 1, sizeof(char *));
        chunk = malloc(CHUNK);
        if (argv == NULL || chunk == NULL)
            _exit(124);
        memset(chunk, 'x', CHUNK - 1);
        chunk[CHUNK - 1] = '\0';
        argv[0] = exe;
        argv[1] = CHILD_MODE;
        total -= (long)(strlen(exe) + 1 + sizeof(CHILD_MODE) + 3 * sizeof(char *));
        for (i = 2; total > 0 && i < n; i++) {
            long len = total - (long)sizeof(char *);

            if (len >= CHUNK) {
                argv[i] = chunk;
                total -= CHUNK + (long)sizeof(char *);
            } else {
                argv[i] = chunk + CHUNK - (len > 1 ? len : 1);
                total = 0;
            }
        }
        argv[i] = NULL;
        execve(exe, argv, envp);
        _exit(errno == E2BIG ? 100 : 101);
    }
    close(pipefd[1]);
    if (read(pipefd[0], &reported, sizeof(reported)) != (ssize_t)sizeof(reported))
        reported = -2;
    close(pipefd[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status), "child exited");
    *arg_max = reported;
    return WEXITSTATUS(status);
}

/* Run with one stack limit: under ARG_MAX execs, over it is E2BIG */
static void
check_stack(rlim_t stack, const char *what)
{
    long arg_max;
    int  ret;

#ifdef __GLIBC__
    /* glibc reports a quarter of the stack, without the kernel's 6MiB cap */
    if (stack == RLIM_INFINITY || stack > 24 * 1024 * 1024) {
        printf("%s: glibc ignores the kernel's cap, skipped\n", what);
        return;
    }
#endif
    ret = exec_with(stack, -MARGIN, &arg_max);
    if (ret == 121) {
        printf("%s: hard stack limit too low, skipped\n", what);
        return;
    }
    if (arg_max == -1) {
        /* No limit: nothing to cross */
        printf("%s: no argument limit reported\n", what);
        return;
    }
    printf("%s: _SC_ARG_MAX %ld\n", what, arg_max);
    if (ret != 0)
        printf("%s: exec under _SC_ARG_MAX: child status %d\n", what, ret);
    check(arg_max >= _POSIX_ARG_MAX, "_SC_ARG_MAX at least the minimum");
    check(ret == 0, "arguments under _SC_ARG_MAX reach the new image");
    ret = exec_with(stack, MARGIN, &arg_max);
    if (ret != 100)
        printf("%s: exec over _SC_ARG_MAX: child status %d\n", what, ret);
    check(ret == 100, "arguments over _SC_ARG_MAX give E2BIG");
}

int
main(int argc, char **argv)
{
    ssize_t len;

    if (argc >= 2 && strcmp(argv[1], CHILD_MODE) == 0)
        return 0;

    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len < 0) {
        printf("no /proc/self/exe, skipping\n");
        return 77;
    }
    exe[len] = '\0';
    if (is_qemu()) {
        /* qemu-user puts its own arguments and limits around the exec */
        printf("qemu-user: exec limits are qemu's, skipping\n");
        return 77;
    }

    /* The current stack limit */
    check_stack(0, "current stack limit");
    /* A small stack, where Linux still allows 128KiB */
    check_stack(256 * 1024, "256KiB stack");
    /* A stack large enough that the kernel's 6MiB cap may apply */
    check_stack(64 * 1024 * 1024, "64MiB stack");
    check_stack(RLIM_INFINITY, "unlimited stack");

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
