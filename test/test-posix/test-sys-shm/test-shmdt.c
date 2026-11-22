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
#include "../test-flags.h"

/* Without the system support this test needs, it is skipped */
#if TEST_XSI_IPC && TEST_POSIX_MULTI_PROCESS

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/* shmdt detaches a segment only from its start address */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Every segment made here, removed on every way out */
static int ids[8];
static int nids;

static void
cleanup(void)
{
    while (nids > 0)
        shmctl(ids[--nids], IPC_RMID, NULL);
}

static void
watchdog(int sig)
{
    (void)sig;
    cleanup();
    _exit(1);
}

static int
keep(int id)
{
    if (id >= 0)
        ids[nids++] = id;
    return id;
}
#define SIZE (3 * 4096)

int
main(void)
{
    struct shmid_ds ds;
    char           *p, *other;
    pid_t           pid;
    time_t          before;
    long            page = sysconf(_SC_PAGESIZE);
    int             id, status;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's shmid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's shmid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(cleanup) == 0, "atexit");

    id = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "shmget");
    p = shmat(id, NULL, 0);
    check(p != (void *)-1, "shmat");
    strcpy(p, "attached");

    /* EINVAL for anything but the start address, detaching nothing */
    check(shmdt(p + 1) == -1 && errno == EINVAL, "EINVAL inside the first page");
    if (page < SIZE)
        check(shmdt(p + page) == -1 && errno == EINVAL, "EINVAL at a later page");
    check(shmdt(NULL) == -1 && errno == EINVAL, "EINVAL for a null pointer");
    other = mmap(NULL, (size_t)page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(other != MAP_FAILED, "mmap");
    check(shmdt(other) == -1 && errno == EINVAL, "EINVAL for memory that isn't shared");
    check(munmap(other, (size_t)page) == 0, "munmap");
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 1, "still attached");
    check(strcmp(p, "attached") == 0, "and readable");

    /* Success: shm_nattch counts down, shm_dtime and shm_lpid are set */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(shmdt(p) == 0 ? 0 : 1);
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the child's shmdt");
    before = time(NULL);
    check(shmdt(p) == 0, "shmdt returns 0");
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_nattch == 0, "shm_nattch");
    check(ds.shm_lpid == getpid(), "shm_lpid");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.shm_dtime >= before - 1 && ds.shm_dtime <= time(NULL), "shm_dtime");

    /* Once detached, the address is no longer a segment's */
    check(shmdt(p) == -1 && errno == EINVAL, "EINVAL detaching twice");

    /* The memory is gone: touching it faults */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        signal(SIGSEGV, SIG_DFL);
        (void)*(volatile char *)p;
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV,
          "detached memory faults");

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
