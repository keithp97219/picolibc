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
#if defined(__PICOLIBC_POSIX_MULTI_PROCESS) && defined(__PICOLIBC_POSIX_PIPE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/*
 * exit, quick_exit and _Exit, in children that report through a pipe:
 * the order of the atexit and at_quick_exit functions (including one
 * registered while they run), repeated registrations, 32 of them, a
 * function that doesn't return, which functions each one calls, stdio
 * flushing, and the status & 0377 seen by waitpid.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static int out[2];

static void
put(char c)
{
    if (write(out[1], &c, 1) != 1)
        _exit(99);
}

static void
f_a(void)
{
    put('a');
}

static void
f_b(void)
{
    put('b');
}

static void
f_c(void)
{
    put('c');
}

/* Registered while the exit functions run */
static void
f_late(void)
{
    put('L');
}

static void
f_registers(void)
{
    put('r');
    if (atexit(f_late) != 0)
        put('!');
}

static void
q_registers(void)
{
    put('r');
    if (at_quick_exit(f_late) != 0)
        put('!');
}

static void
f_exits(void)
{
    put('x');
    _exit(7);
}

static void
f_count(void)
{
    put('.');
}

enum test {
    ORDER,
    LATE,
    TWICE,
    MANY,
    NO_RETURN,
    QUICK_ORDER,
    QUICK_LATE,
    EXIT_ONLY_ATEXIT,
    QUICK_ONLY_AT_QUICK_EXIT,
    UNDERSCORE_EXIT,
    STATUS,
    QUICK_STDIO,
};

/* Run a test in a child; return its status and what it wrote */
static int
run(enum test test, char *buf, size_t size)
{
    pid_t   pid;
    int     status;
    size_t  len = 0;
    ssize_t n;
    int     i;

    fflush(stdout);
    check(pipe(out) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(out[0]);
        switch (test) {
        case ORDER:
            atexit(f_a);
            atexit(f_b);
            atexit(f_c);
            exit(0);
        case LATE:
            atexit(f_a);
            atexit(f_registers);
            atexit(f_c);
            exit(0);
        case TWICE:
            atexit(f_a);
            atexit(f_b);
            atexit(f_a);
            exit(0);
        case MANY:
            for (i = 0; i < 32; i++)
                if (atexit(f_count) != 0)
                    put('!');
            exit(0);
        case NO_RETURN:
            atexit(f_a);
            atexit(f_exits);
            atexit(f_c);
            exit(0);
        case QUICK_ORDER:
            at_quick_exit(f_a);
            at_quick_exit(f_b);
            at_quick_exit(f_c);
            quick_exit(0);
        case QUICK_LATE:
            at_quick_exit(f_a);
            at_quick_exit(q_registers);
            at_quick_exit(f_c);
            quick_exit(0);
        case EXIT_ONLY_ATEXIT:
            atexit(f_a);
            at_quick_exit(f_b);
            exit(3);
        case QUICK_ONLY_AT_QUICK_EXIT:
            atexit(f_a);
            at_quick_exit(f_b);
            quick_exit(4);
        case UNDERSCORE_EXIT:
            atexit(f_a);
            at_quick_exit(f_b);
            _Exit(5);
        case STATUS:
            exit(0x1234);
        case QUICK_STDIO: {
            /* quick_exit ends as _Exit does: stdout isn't flushed */
            static char vbuf[256];

            if (dup2(out[1], 1) != 1)
                _exit(98);
            if (setvbuf(stdout, vbuf, _IOFBF, sizeof(vbuf)) != 0)
                _exit(97);
            printf("unflushed");
            quick_exit(6);
        }
        }
        _exit(100);
    }
    close(out[1]);
    while (len + 1 < size && (n = read(out[0], buf + len, size - 1 - len)) > 0)
        len += (size_t)n;
    buf[len] = '\0';
    close(out[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

static int
exited(int status, int code)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

int
main(void)
{
    char buf[128];
    int  status;

    status = run(ORDER, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "cba") == 0, "atexit functions in reverse order");

    status = run(LATE, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "crLa") == 0, "a function registered during exit");

    status = run(TWICE, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "aba") == 0, "a function registered twice");

    status = run(MANY, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "................................") == 0,
          "32 atexit functions");

    status = run(NO_RETURN, buf, sizeof(buf));
    check(exited(status, 7) && strcmp(buf, "cx") == 0,
          "a function that doesn't return ends the processing");

    status = run(QUICK_ORDER, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "cba") == 0, "at_quick_exit functions in reverse order");

    status = run(QUICK_LATE, buf, sizeof(buf));
    check(exited(status, 0) && strcmp(buf, "crLa") == 0, "a function registered during quick_exit");

    status = run(EXIT_ONLY_ATEXIT, buf, sizeof(buf));
    check(exited(status, 3) && strcmp(buf, "a") == 0, "exit calls only the atexit functions");

    status = run(QUICK_ONLY_AT_QUICK_EXIT, buf, sizeof(buf));
    check(exited(status, 4) && strcmp(buf, "b") == 0,
          "quick_exit calls only the at_quick_exit functions");

    status = run(UNDERSCORE_EXIT, buf, sizeof(buf));
    check(exited(status, 5) && strcmp(buf, "") == 0, "_Exit calls no functions");

    status = run(STATUS, buf, sizeof(buf));
    check(exited(status, 0x34), "waitpid sees status & 0377");

    status = run(QUICK_STDIO, buf, sizeof(buf));
    check(exited(status, 6) && strcmp(buf, "") == 0, "quick_exit doesn't flush streams");

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
