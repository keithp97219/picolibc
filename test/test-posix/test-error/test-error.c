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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * error and error_at_line print "program: message[: strerror]" on
 * stderr, count their messages, and exit when given a status;
 * program_invocation_name is argv[0]
 */

static char dir[] = "tmp-XXXXXX";

static void
cleanup(void)
{
    (void)unlink("err");
    if (chdir("..") == 0)
        (void)rmdir(dir);
}

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            cleanup();                                 \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
hook(void)
{
    fputs("hook ", stderr);
}

int
main(int argc, char **argv)
{
    static char  got[1024], want[1024];
    const char  *pin, *slash;
    int          fd, saved, status;
    ssize_t      n;
    unsigned int count;
    pid_t        pid;

    (void)argc;
    pin = program_invocation_name;
    check(pin != NULL && strcmp(pin, argv[0]) == 0, "program_invocation_name");
    slash = strrchr(argv[0], '/');
    check(strcmp(program_invocation_short_name, slash ? slash + 1 : argv[0]) == 0,
          "program_invocation_short_name");

    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");

    /* Send stderr to a file */
    fflush(stderr);
    fd = open("err", O_RDWR | O_CREAT | O_TRUNC, 0600);
    check(fd >= 0, "open");
    saved = dup(2);
    check(saved >= 0 && dup2(fd, 2) == 2, "redirect stderr");

    count = error_message_count;
    error(0, ENOENT, "one %d", 1);
    error(0, 0, "two");
    error_at_line(0, EINVAL, "file.c", 12, "three");
    error_at_line(0, 0, NULL, 0, "four");
    /* Once per place: the second "five" is dropped */
    error_one_per_line = 1;
    error_at_line(0, 0, "file.c", 13, "five");
    error_at_line(0, 0, "file.c", 13, "five");
    error_at_line(0, 0, "file.c", 14, "six");
    error_one_per_line = 0;
    error_print_progname = hook;
    error(0, 0, "seven");
    error_print_progname = NULL;

    fflush(stderr);
    check(dup2(saved, 2) == 2 && close(saved) == 0, "restore stderr");
    check(error_message_count - count == 7, "error_message_count");

    snprintf(want, sizeof(want),
             "%s: one 1: %s\n"
             "%s: two\n"
             "%s:file.c:12: three: %s\n"
             "%s: four\n"
             "%s:file.c:13: five\n"
             "%s:file.c:14: six\n"
             "hook seven\n",
             pin, strerror(ENOENT), pin, pin, strerror(EINVAL), pin, pin, pin);
    n = pread(fd, got, sizeof(got) - 1, 0);
    check(n >= 0, "read the messages");
    got[n] = '\0';
    if (strcmp(got, want) != 0)
        printf("got:\n%swant:\n%s", got, want);
    check(strcmp(got, want) == 0, "messages");
    close(fd);

    /* A status ends the process with it */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int null = open("/dev/null", O_WRONLY);
        if (null >= 0)
            dup2(null, 2);
        error(3, 0, "exit");
        _exit(1);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 3, "exit status");

    cleanup();
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
