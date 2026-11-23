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
#include <stdio.h>

/* fflush(NULL) flushes only the standard streams without exit flush */
#if !defined(__PICOLIBC__) || defined(__STDIO_EXIT_FLUSH)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef TEST_FILE_NAME
#define TEST_FILE_NAME "test-fflush-all.txt"
#endif

/*
 * fflush(NULL) flushes every open stream and leaves them open: later
 * fflush(NULL) calls and exit still flush them, and fclose still closes
 * them (XSH fflush, exit)
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* lock-valid.c, used by the tests, has room for 32 locks */
#define NSTREAM 20

static void
check_file(const char *name, const char *contents)
{
    char    buf[128];
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

static FILE *
open_buffered(const char *name)
{
    FILE *f = fopen(name, "w");

    check(f != NULL, "fopen");
    check(setvbuf(f, NULL, _IOFBF, 256) == 0, "setvbuf");
    return f;
}

static void
name_of(char *name, int i)
{
    snprintf(name, 16, "many-%d", i);
}

/* Write, fflush(NULL), write more, then exit without fclose */
static void
exit_child(void)
{
    FILE *f[NSTREAM];
    FILE *late;
    char  name[16];
    int   i;

    for (i = 0; i < NSTREAM; i++) {
        name_of(name, i);
        f[i] = open_buffered(name);
        fputs("one", f[i]);
    }
    if (fflush(NULL) != 0)
        _exit(2);
    for (i = 0; i < NSTREAM; i++)
        fputs(" two", f[i]);
    if (fflush(NULL) != 0)
        _exit(3);
    for (i = 0; i < NSTREAM; i++)
        fputs(" three", f[i]);
    /* A stream opened after fflush(NULL) is flushed at exit too */
    late = open_buffered("late");
    fputs("late", late);
    exit(0);
}

int
main(void)
{
    const char *dir = TEST_FILE_NAME;
    char        name[16];
    FILE       *f[NSTREAM];
    FILE       *g;
    pid_t       child;
    int         status;
    int         i;

    check((mkdir(dir, 0700) == 0 || errno == EEXIST) && chdir(dir) == 0, "scratch directory");

    /* fflush(NULL) twice, with writes in between */
    f[0] = open_buffered("twice");
    check(fputs("first", f[0]) >= 0, "fputs");
    check_file("twice", "");
    check(fflush(NULL) == 0, "fflush(NULL)");
    check_file("twice", "first");
    check(fputs(" second", f[0]) >= 0, "fputs");
    check_file("twice", "first");
    check(fflush(NULL) == 0, "second fflush(NULL)");
    check_file("twice", "first second");

    /* fclose after fflush(NULL) writes the rest and closes the stream */
    check(fputs(" third", f[0]) >= 0, "fputs");
    check(fclose(f[0]) == 0, "fclose");
    check_file("twice", "first second third");
    /* The closed stream is gone from the list fflush(NULL) walks */
    g = open_buffered("after");
    check(fputs("after", g) >= 0, "fputs");
    check(fflush(NULL) == 0, "fflush(NULL) after fclose");
    check_file("after", "after");

    /* Many streams, each flushed every time */
    for (i = 0; i < NSTREAM; i++) {
        name_of(name, i);
        f[i] = open_buffered(name);
        check(fprintf(f[i], "%d", i) > 0, "fprintf");
    }
    check(fflush(NULL) == 0, "fflush(NULL) many");
    for (i = 0; i < NSTREAM; i++) {
        char want[16];

        name_of(name, i);
        snprintf(want, sizeof(want), "%d", i);
        check_file(name, want);
        check(fprintf(f[i], " %d", i) > 0, "fprintf");
    }
    /* Close every other one, the rest are still flushed */
    for (i = 0; i < NSTREAM; i += 2)
        check(fclose(f[i]) == 0, "fclose many");
    for (i = 1; i < NSTREAM; i += 2)
        check(fprintf(f[i], " %d", i) > 0, "fprintf");
    check(fflush(NULL) == 0, "fflush(NULL) after closing some");
    for (i = 0; i < NSTREAM; i++) {
        char want[32];

        name_of(name, i);
        if (i & 1) {
            snprintf(want, sizeof(want), "%d %d %d", i, i, i);
            check(fclose(f[i]) == 0, "fclose rest");
        } else
            snprintf(want, sizeof(want), "%d %d", i, i);
        check_file(name, want);
    }
    check(fclose(g) == 0, "fclose after");

    /* exit flushes streams that fflush(NULL) already flushed */
    check(fflush(NULL) == 0, "fflush(NULL) before fork");
    child = fork();
    check(child >= 0, "fork");
    if (child == 0)
        exit_child();
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    for (i = 0; i < NSTREAM; i++) {
        name_of(name, i);
        check_file(name, "one two three");
    }
    check_file("late", "late");

    /* Clean up */
    for (i = 0; i < NSTREAM; i++) {
        name_of(name, i);
        check(unlink(name) == 0, "cleanup");
    }
    check(unlink("twice") == 0 && unlink("after") == 0 && unlink("late") == 0, "cleanup");
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
