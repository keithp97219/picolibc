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

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                  \
    || ((defined(_POSIX_SEMAPHORES) && _POSIX_SEMAPHORES > 0)                               \
        && (defined(_POSIX_THREADS) && _POSIX_THREADS > 0)                                  \
        && (defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static char name[64];

/* Named semaphores are shared by name, between opens and between processes */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            if (name[0])                               \
                (void)sem_unlink(name);                \
            exit(1);                                   \
        }                                              \
    } while (0)

static void
wait_child(pid_t pid)
{
    int status;

    check(waitpid(pid, &status, 0) == pid, "waitpid");
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status");
}

int
main(void)
{
    sem_t *named, *again;
    int    value;
    pid_t  pid;

    snprintf(name, sizeof(name), "/picolibc-test-sem-%ld", (long)getpid());
    (void)sem_unlink(name);
    named = sem_open(name, O_CREAT | O_EXCL, 0600, 1);
    check(named != SEM_FAILED, "sem_open create");
    check(sem_open(name, O_CREAT | O_EXCL, 0600, 1) == SEM_FAILED && errno == EEXIST,
          "sem_open O_EXCL existing");
    again = sem_open(name, 0);
    check(again == named, "sem_open again returns the same semaphore");
    check(sem_getvalue(named, &value) == 0 && value == 1, "named initial value");
    check(sem_wait(named) == 0, "sem_wait named");

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        sem_t *child = sem_open(name, 0);
        if (child == SEM_FAILED || sem_post(child) != 0 || sem_close(child) != 0)
            _exit(1);
        _exit(0);
    }
    check(sem_wait(named) == 0, "sem_wait for child post");
    wait_child(pid);

    check(sem_close(again) == 0, "sem_close 1");
    check(sem_getvalue(named, &value) == 0 && value == 0, "still mapped after one close");
    check(sem_close(named) == 0, "sem_close 2");
    check(sem_unlink(name) == 0, "sem_unlink");
    check(sem_open(name, 0) == SEM_FAILED && errno == ENOENT, "sem_open after unlink");
    check(sem_unlink(name) == -1 && errno == ENOENT, "sem_unlink missing");
    name[0] = '\0';

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
