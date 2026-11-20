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
#define _XOPEN_SOURCE 700
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                \
    || (defined(__PICOLIBC_XSI_IPC) && __PICOLIBC_XSI_IPC > 0                             \
        && defined(__PICOLIBC_POSIX_MULTI_PROCESS) && __PICOLIBC_POSIX_MULTI_PROCESS > 0)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

/* shmget, shmat, shmdt and shmctl share System V memory segments */

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
    struct shmid_ds ds;
    char           *p;
    pid_t           pid;
    int             id = shmget(IPC_PRIVATE, 8192, IPC_CREAT | 0600), status;

    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "shmget");
    p = shmat(id, NULL, 0);
    check(p != (void *)-1, "shmat");
    strcpy(p, "parent");
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_segsz == 8192 && ds.shm_nattch == 1 && ds.shm_cpid == getpid(), "segment");

    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int ok = strcmp(p, "parent") == 0;

        strcpy(p, "child");
        _exit(ok ? 0 : 1);
    }
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "child saw the data");
    check(strcmp(p, "child") == 0, "child's change seen");

    check(shmdt(p) == 0, "shmdt");
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 0, "detached");
    p = shmat(id, NULL, SHM_RDONLY);
    check(p != (void *)-1 && strcmp(p, "child") == 0, "read-only attach");
    check(shmdt(p) == 0, "shmdt");
    check(shmctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    check(shmat(id, NULL, 0) == (void *)-1, "removed");
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
