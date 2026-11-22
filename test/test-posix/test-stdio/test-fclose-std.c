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
#if TEST_POSIX_MULTI_PROCESS && TEST_POSIX_SIGNALS && TEST_POSIX_FD_MGMT

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * fclose on the standard streams, then exit: fclose closes the
 * descriptor, and the streams are still flushed at exit, which must
 * not touch what fclose released (gnulib's close_stdout does exactly
 * this)
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* volatile, so the compiler can't drop the malloc/free pair */
static void * volatile blocks[3];

static void
run(int which)
{
    pid_t pid;
    int   status;

    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        alarm(10);
        /*
         * Lock the stream so it has a lock, and allocate after it so
         * that freeing blocks[1] links the freed lock into the malloc
         * free list, scribbling over it as a reused block would. The
         * guards keep the free blocks from merging.
         */
        flockfile(which == 0 ? stdout : which == 1 ? stderr : stdin);
        funlockfile(which == 0 ? stdout : which == 1 ? stderr : stdin);
        blocks[0] = malloc(64);
        blocks[1] = malloc(64);
        blocks[2] = malloc(64);
        switch (which) {
        case 0:
            /* Use the stream, so its lock exists, then close it */
            fputs("", stdout);
            fflush(stdout);
            if (fclose(stdout) != 0)
                _exit(2);
            break;
        case 1:
            fputs("", stderr);
            if (fclose(stderr) != 0)
                _exit(2);
            break;
        case 2:
            (void)ferror(stdin);
            (void)fileno(stdin);
            (void)getc(stdin);
            fclose(stdin);
            break;
        }
        /* fclose closes the descriptor too */
        if (fcntl(which == 0 ? 1 : which == 1 ? 2 : 0, F_GETFD) != -1 || errno != EBADF)
            _exit(3);
        free(blocks[1]);
        exit(0);
    }
    check(waitpid(pid, &status, 0) == pid, "waitpid");
    if (WIFSIGNALED(status))
        printf("stream %d: child killed by signal %d\n", which, WTERMSIG(status));
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "fclose then exit");
}

int
main(void)
{
    int which;

    for (which = 0; which < 3; which++)
        run(which);
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
