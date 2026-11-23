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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * A FIFO with the name mkstemp tries first doesn't stop it. Two
 * children start from the same generator state, so the second tries
 * the first one's name first, now a FIFO.
 */

static const char tmpl[] = "test-mkstemp-fifo-XXXXXX";
static char       first[sizeof(tmpl)];
static char       second[sizeof(tmpl)];

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            (void)unlink(first);                       \
            (void)unlink(second);                      \
            exit(1);                                   \
        }                                              \
    } while (0)

/* In a child, run mkstemp on the template and send back the name */
static void
child_name(char *name, size_t size)
{
    int     p[2];
    pid_t   pid;
    int     status;
    ssize_t n;

    check(pipe(p) == 0, "pipe");
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        char t[sizeof(tmpl)];
        int  fd;

        close(p[0]);
        alarm(20);
        strcpy(t, tmpl);
        fd = mkstemp(t);
        if (fd < 0)
            _exit(2);
        if (write(p[1], t, strlen(t) + 1) < 0)
            _exit(3);
        _exit(0);
    }
    close(p[1]);
    n = read(p[0], name, size);
    close(p[0]);
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "mkstemp in a child, without hanging");
    check(n > 0 && name[n - 1] == '\0', "the child's name");
}

int
main(void)
{
    struct stat st;
    char        cmd[sizeof(tmpl) + 16];

    child_name(first, sizeof(first));
    check(unlink(first) == 0, "remove the first file");
    /* With the shell's mkfifo, as the C library may not have one */
    snprintf(cmd, sizeof(cmd), "mkfifo %s", first);
    check(system(cmd) == 0, "make a FIFO with that name");
    child_name(second, sizeof(second));
    check(strcmp(first, second) != 0, "a different name");
    check(stat(first, &st) == 0 && S_ISFIFO(st.st_mode), "the FIFO is still there");
    (void)unlink(first);
    (void)unlink(second);
    printf("success\n");
    return 0;
}
