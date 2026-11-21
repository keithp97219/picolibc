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
    && defined(__PICOLIBC_POSIX_FILE_SYSTEM)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* perror's message on stderr, and remove for files and directories */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

int
main(void)
{
    char    dir[] = "perror-XXXXXX";
    char    buf[256];
    char    want[256];
    int     p[2];
    pid_t   child;
    int     status;
    ssize_t len, total = 0;

    /* perror writes "s: message\n", or just the message for a null or empty s */
    check(pipe(p) == 0, "pipe");
    child = fork();
    check(child >= 0, "fork");
    if (child == 0) {
        if (dup2(p[1], 2) != 2)
            _exit(2);
        errno = ENOENT;
        perror("first");
        errno = EACCES;
        perror(NULL);
        errno = EINVAL;
        perror("");
        _exit(0);
    }
    check(close(p[1]) == 0, "close");
    while ((len = read(p[0], buf + total, sizeof(buf) - 1 - total)) > 0)
        total += len;
    buf[total] = '\0';
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    snprintf(want, sizeof(want), "first: %s\n%s\n%s\n", strerror(ENOENT), strerror(EACCES),
             strerror(EINVAL));
    if (strcmp(buf, want) != 0)
        printf("got \"%s\", want \"%s\"\n", buf, want);
    check(strcmp(buf, want) == 0, "perror output");

    /* remove: unlink for files, rmdir for directories */
    check(mkdtemp(dir) != NULL && chdir(dir) == 0, "scratch directory");
    {
        FILE *f = fopen("file", "w");

        check(f != NULL && fclose(f) == 0, "file");
    }
    check(mkdir("sub", 0700) == 0, "mkdir");
    check(remove("file") == 0 && access("file", F_OK) != 0, "remove a file");
    check(remove("sub") == 0 && access("sub", F_OK) != 0, "remove a directory");
    errno = 0;
    check(remove("file") == -1 && errno == ENOENT, "remove ENOENT");
    check(chdir("..") == 0 && remove(dir) == 0, "cleanup");

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
