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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * C: standard error is not fully buffered. picolibc writes it a byte
 * at a time unless built with -Dstderr-buffered=true, which line
 * buffers it: a partial line waits, a newline writes the line, and
 * exit writes whatever is left.
 */

#if defined(__PICOLIBC__) && defined(__STDERR_BUFFERED)
#define PARTIAL_WAITS 1
#else
#define PARTIAL_WAITS 0
#endif

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
    char  name[] = "stderr-buf-XXXXXX";
    int   fd, status;
    pid_t pid;
    off_t size;

    if (argc > 1) {
        /* The child, run with stderr on the file */
        if (fprintf(stderr, "ab") != 2)
            return 10;
        if (size_of(2) != (PARTIAL_WAITS ? 0 : 2))
            return 11;
        if (fprintf(stderr, "c\n") != 2)
            return 12;
        if (size_of(2) != 4)
            return 13;
        if (fprintf(stderr, "tail") != 4)
            return 14;
        exit(0);
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

        if (dup2(fd, 2) != 2)
            _exit(20);
        execv(argv[0], args);
        _exit(21);
    }
    if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status)) {
        printf("child failed\n");
        close(fd);
        unlink(name);
        return 1;
    }
    size = size_of(fd);
    close(fd);
    unlink(name);
    if (WEXITSTATUS(status) != 0) {
        printf("child status %d\n", WEXITSTATUS(status));
        return 1;
    }
    if (size != 8) {
        printf("stderr held %ld bytes after exit, not 8\n", (long)size);
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
