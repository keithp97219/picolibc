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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_FILE_SYSTEM && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * C: standard output is fully buffered when it can be determined not
 * to refer to an interactive device. With stdout on a regular file, a
 * complete line stays in the buffer until fflush. errno starts at
 * zero regardless.
 */

static off_t
size_of(int fd)
{
    struct stat st;

    if (fstat(fd, &st) != 0)
        return -1;
    return st.st_size;
}

int
main(int argc, char **argv)
{
    char  name[] = "stdout-buf-XXXXXX";
    int   fd, status;
    pid_t pid;

    if (argc > 1) {
        /* The child, run with stdout on the file */
        off_t before, after;

        if (errno != 0)
            return 10;
        if (printf("line\n") != 5)
            return 11;
        before = size_of(1);
        if (fflush(stdout) != 0)
            return 12;
        after = size_of(1);
        /* report through the exit status: 0 buffered, 1 written at once */
        return before == 0 && after == 5 ? 0 : before == 5 ? 1 : 13;
    }

    fd = mkstemp(name);
    if (fd < 0) {
        printf("mkstemp failed\n");
        return 1;
    }
    fflush(stdout);
    pid = fork();
    if (pid == 0) {
        char *args[] = { argv[0], "child", NULL };

        if (dup2(fd, 1) != 1)
            _exit(20);
        execv(argv[0], args);
        _exit(21);
    }
    close(fd);
    if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status)) {
        printf("child failed\n");
        unlink(name);
        return 1;
    }
    unlink(name);
    if (WEXITSTATUS(status) != 0) {
        printf("child status %d (1: stdout on a file was line buffered)\n", WEXITSTATUS(status));
        return 1;
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
