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
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                \
    || (defined(__PICOLIBC_XSI_IPC) && __PICOLIBC_XSI_IPC > 0                             \
        && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/wait.h>

/* semget, semctl and semop manage a System V semaphore set */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

int
main(void)
{
    struct semid_ds ds;
    struct sembuf   op;
    union semun     arg;
    unsigned short  vals[2] = { 3, 5 }, got[2];
    pid_t           pid;
    int             id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600), status;

    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "semget");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0 && ds.sem_nsems == 2, "IPC_STAT");
    check((ds.sem_perm.mode & 0777) == 0600, "permissions");
    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == 0, "SETALL");
    arg.array = got;
    check(semctl(id, 0, GETALL, arg) == 0 && got[0] == 3 && got[1] == 5, "GETALL");
    arg.val = 1;
    check(semctl(id, 1, SETVAL, arg) == 0 && semctl(id, 1, GETVAL) == 1, "SETVAL");

    op.sem_num = 0;
    op.sem_op = -3;
    op.sem_flg = 0;
    check(semop(id, &op, 1) == 0 && semctl(id, 0, GETVAL) == 0, "decrement");
    check(semctl(id, 0, GETPID) == getpid(), "GETPID");
    op.sem_flg = IPC_NOWAIT;
    op.sem_op = -1;
    check(semop(id, &op, 1) == -1 && errno == EAGAIN, "would block");

    /* Another process's increment wakes the waiter */
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf up = { 0, 1, 0 };

        _exit(semop(id, &up, 1) == 0 ? 0 : 1);
    }
    op.sem_flg = 0;
    check(semop(id, &op, 1) == 0, "waited");
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child");
    check(semctl(id, 0, IPC_RMID) == 0, "IPC_RMID");
    check(semop(id, &op, 1) == -1 && (errno == EINVAL || errno == EIDRM), "removed");
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
