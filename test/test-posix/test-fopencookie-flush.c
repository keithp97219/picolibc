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

#define _GNU_SOURCE
#include <stdio.h>

/* fflush(NULL) flushes only the standard streams without exit flush */
#if !defined(__PICOLIBC__) || defined(__STDIO_EXIT_FLUSH)

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-fopencookie-flush.txt"
#endif

/*
 * Streams made by fopencookie and funopen are flushed by fflush(NULL)
 * and at exit like any other
 */

#if defined(__PICOLIBC__) || defined(__FreeBSD__)
#define TEST_FUNOPEN
#endif

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
check_file(const char *name, const char *contents)
{
    char    buf[64];
    int     fd = open(name, O_RDONLY);
    ssize_t len;

    check(fd >= 0, "open");
    len = read(fd, buf, sizeof(buf) - 1);
    check(len >= 0 && close(fd) == 0, "read");
    buf[len] = '\0';
    if (strcmp(buf, contents) != 0)
        printf("%s holds \"%s\", expected \"%s\"\n", name, buf, contents);
    check(strcmp(buf, contents) == 0, "contents");
}

static int
open_out(const char *name)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    check(fd >= 0, "open");
    return fd;
}

static ssize_t
cookie_write(void *cookie, const char *buf, size_t n)
{
    return write((int)(intptr_t)cookie, buf, n);
}

static FILE *
cookie_open(const char *name)
{
    cookie_io_functions_t io = { .write = cookie_write };
    FILE                 *f = fopencookie((void *)(intptr_t)open_out(name), "w", io);

    check(f != NULL && setvbuf(f, NULL, _IOFBF, 256) == 0, "fopencookie");
    return f;
}

#ifdef TEST_FUNOPEN
static ssize_t
fun_write(void *cookie, const void *buf, size_t n)
{
    return write((int)(intptr_t)cookie, buf, n);
}

static FILE *
fun_open(const char *name)
{
    FILE *f = funopen((void *)(intptr_t)open_out(name), NULL, fun_write, NULL, NULL);

    check(f != NULL && setvbuf(f, NULL, _IOFBF, 256) == 0, "funopen");
    return f;
}
#endif

int
main(void)
{
    const char *dir = TEST_FILE_NAME;
    FILE       *c;
#ifdef TEST_FUNOPEN
    FILE *u;
#endif
    pid_t child;
    int   status;

    check((mkdir(dir, 0700) == 0 || errno == EEXIST) && chdir(dir) == 0, "scratch directory");

    /* fflush(NULL) */
    c = cookie_open("cookie");
    check(fputs("cookie", c) >= 0, "fputs");
    check_file("cookie", "");
#ifdef TEST_FUNOPEN
    u = fun_open("funopen");
    check(fputs("funopen", u) >= 0, "fputs");
    check_file("funopen", "");
#endif
    check(fflush(NULL) == 0, "fflush(NULL)");
    check_file("cookie", "cookie");
    check(fclose(c) == 0, "fclose");
#ifdef TEST_FUNOPEN
    check_file("funopen", "funopen");
    check(fclose(u) == 0, "fclose");
#endif

    /* exit */
    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        c = cookie_open("cookie");
        fputs("at exit", c);
#ifdef TEST_FUNOPEN
        u = fun_open("funopen");
        fputs("at exit", u);
#endif
        exit(0);
    }
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    check_file("cookie", "at exit");
    check(unlink("cookie") == 0, "cleanup");
#ifdef TEST_FUNOPEN
    check_file("funopen", "at exit");
    check(unlink("funopen") == 0, "cleanup");
#endif
    check(chdir("..") == 0 && rmdir(dir) == 0, "cleanup");
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no exit flush, skipping\n");
    return 77;
}

#endif
