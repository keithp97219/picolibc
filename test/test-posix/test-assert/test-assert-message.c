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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* assert, NDEBUG and static_assert */

/* Active: a false assertion reports itself and aborts */
#undef NDEBUG
#include <assert.h>

/* assert is a macro, not a function */
#ifndef assert
#error "assert isn't a macro"
#endif

static const int fail_line = __LINE__ + 4;
static void
failing_assertion(int value)
{
    assert(value == 12345);
}

/* assert expands to a void expression */
static void
void_expression(int value)
{
#ifdef __GNUC__
    _Static_assert(__builtin_types_compatible_p(__typeof__(assert(value)), void), "void");
#endif
    (void)(assert(value), 0);
}

/* Any scalar type */
static void
scalar_types(void)
{
    static int x;
    double     d = 0.5;
    int       *p = &x;
    _Bool      b = 1;

    assert(p);
    assert(d);
    assert(b);
    assert('c');
}

/* With NDEBUG defined, assert does nothing and evaluates nothing */
#define NDEBUG
#include <assert.h>

static int evaluated;

static void
disabled(void)
{
    assert(++evaluated == 0);
    (void)(assert(0), 0);
}

/* Including <assert.h> again without NDEBUG turns it back on */
#undef NDEBUG
#include <assert.h>

static const int again_line = __LINE__ + 4;
static void
enabled_again(int value)
{
    assert(value != value);
}

/* ISO C defines static_assert in <assert.h> (C23 makes it a keyword) */
#if !defined(static_assert) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
#error "static_assert isn't defined"
#endif
static_assert(sizeof(int) >= 2, "static_assert at file scope");

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_POSIX_PIPE) && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <signal.h>
#include <sys/wait.h>

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char err[1024], out[1024];

static void
collect(int fd, char *buf, size_t size)
{
    size_t  len = 0;
    ssize_t n;

    while ((n = read(fd, buf + len, size - 1 - len)) > 0)
        len += (size_t)n;
    buf[len] = '\0';
    close(fd);
}

/* Runs which() in a child, collecting what it writes to stdout and stderr */
static int
run(void (*which)(int), int value)
{
    int   errfds[2], outfds[2], status;
    pid_t pid;

    fflush(stdout);
    check(pipe(errfds) == 0 && pipe(outfds) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        check(dup2(errfds[1], 2) == 2 && dup2(outfds[1], 1) == 1, "redirect");
        close(errfds[0]);
        close(outfds[0]);
        which(value);
        exit(0);
    }
    close(errfds[1]);
    close(outfds[1]);
    collect(outfds[0], out, sizeof(out));
    collect(errfds[0], err, sizeof(err));
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    return status;
}

static void
check_report(const char *expr, int line, const char *func)
{
    char num[16];

    snprintf(num, sizeof(num), "%d", line);
    check(strstr(err, expr) != NULL, "the expression");
    check(strstr(err, __FILE__) != NULL, "the file name");
    check(strstr(err, num) != NULL, "the line number");
    check(strstr(err, func) != NULL, "the function name");
    check(out[0] == '\0', "nothing on stdout");
}

int
main(void)
{
    int status;

    /* A true assertion returns */
    status = run(failing_assertion, 12345);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "true assertion");
    check(err[0] == '\0' && out[0] == '\0', "nothing written");
    void_expression(1);
    scalar_types();

    /* A false one writes to stderr and calls abort */
    status = run(failing_assertion, 0);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "abort");
    check_report("value == 12345", fail_line, "failing_assertion");

    /* With NDEBUG, the expression isn't evaluated */
    disabled();
    check(evaluated == 0, "NDEBUG");

    status = run(enabled_again, 1);
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "abort after re-including");
    check_report("value != value", again_line, "enabled_again");

    printf("success\n");
    return 0;
}

#else

int
main(void)
{
    void_expression(1);
    scalar_types();
    disabled();
    if (evaluated != 0) {
        printf("NDEBUG: the expression was evaluated\n");
        return 1;
    }
    (void)failing_assertion;
    (void)enabled_again;
    (void)fail_line;
    (void)again_line;
    printf("no system support, skipping\n");
    return 77;
}

#endif
