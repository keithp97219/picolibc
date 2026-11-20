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
#include "../posix-groups.h"

/* Without the system support this test needs, it is skipped */
#if defined(__PICOLIBC_XSI_IPC) && defined(__PICOLIBC_POSIX_MULTI_PROCESS)

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

/* shmat attaches segments where asked, read-only or not */

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
static void
drop(int id)
{
    int i;

    for (i = 0; i < nids; i++)
        if (ids[i] == id)
            ids[i] = ids[--nids];
}

static int
child_status(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

/* SHM_FAILED is new in POSIX.1-2024 */
#ifndef SHM_FAILED
#define SHM_FAILED ((void *)-1)
#endif

/* The size of this process's address space, from /proc/self/statm */
static long
address_space(void)
{
    FILE *f = fopen("/proc/self/statm", "r");
    long  pages = -1;

    if (f) {
        if (fscanf(f, "%ld", &pages) != 1)
            pages = -1;
        fclose(f);
    }
    return pages < 0 ? -1 : pages * sysconf(_SC_PAGESIZE);
}

#define SIZE 8192

int
main(void)
{
    struct shmid_ds ds;
    char           *p, *q, *area;
    uintptr_t       aligned;
    size_t          span;
    pid_t           pid;
    time_t          before;
    int             id, status;

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(cleanup) == 0, "atexit");

    id = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "shmget");

    /* A null address lets the system choose; shm_nattch, shm_atime and shm_lpid follow */
    before = time(NULL);
    p = shmat(id, NULL, 0);
    check(p != SHM_FAILED, "shmat");
    strcpy(p, "parent");
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_segsz == SIZE && ds.shm_nattch == 1 && ds.shm_cpid == getpid(), "segment");
    check(ds.shm_lpid == getpid(), "shm_lpid");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.shm_atime >= before - 1 && ds.shm_atime <= time(NULL), "shm_atime");

    /* The memory is shared with a child */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        int ok = strcmp(p, "parent") == 0;

        strcpy(p, "child");
        _exit(ok ? 0 : 1);
    }
    check(child_status(pid) == 0, "child saw the data");
    check(strcmp(p, "child") == 0, "child's change seen");

    /* A second attachment of the same segment sees the same memory */
    q = shmat(id, NULL, 0);
    check(q != SHM_FAILED && q != p && strcmp(q, "child") == 0, "a second attachment");
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 2, "shm_nattch counts both");
    check(shmdt(q) == 0, "shmdt");
    check(shmdt(p) == 0, "shmdt");
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 0, "detached");

    /* SHM_RDONLY attaches for reading; writing faults */
    p = shmat(id, NULL, SHM_RDONLY);
    check(p != SHM_FAILED && strcmp(p, "child") == 0, "read-only attach");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        signal(SIGSEGV, SIG_DFL);
        *(volatile char *)p = 'x';
        _exit(0);
    }
    check(waitpid(pid, &status, 0) == pid && WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV,
          "a write faults");
    check(shmdt(p) == 0, "shmdt");

    /*
     * An address of our choosing: find room for the segment at a
     * multiple of SHMLBA, then give it back to the system
     */
    span = SIZE + 2 * (size_t)SHMLBA;
    area = mmap(NULL, span, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(area != MAP_FAILED, "mmap");
    aligned = ((uintptr_t)area + SHMLBA - 1) / SHMLBA * SHMLBA;
    check(munmap(area, span) == 0, "munmap");

    /* Without SHM_RND it goes exactly there */
    p = shmat(id, (void *)aligned, 0);
    check(p == (void *)aligned && strcmp(p, "child") == 0, "attached at the address");
    check(shmdt(p) == 0, "shmdt");

    /* With SHM_RND the address is rounded down to a multiple of SHMLBA */
    p = shmat(id, (char *)aligned + 123, SHM_RND);
    check(p == (char *)aligned + 123 - (aligned + 123) % SHMLBA, "rounded down to SHMLBA");
    check(shmdt(p) == 0, "shmdt");

    /* Without SHM_RND, an address that isn't page aligned is EINVAL */
    p = shmat(id, (char *)aligned + 123, 0);
    check(p == SHM_FAILED && errno == EINVAL, "EINVAL for an unaligned address");

    /* EINVAL for an identifier that isn't valid */
    check(shmat(-1, NULL, 0) == SHM_FAILED && errno == EINVAL, "EINVAL for the identifier");
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 0, "no attachment left behind");

    /* ENOMEM when the address space has no room: a child with a small limit */
    if (address_space() > 0) {
        int big = keep(shmget(IPC_PRIVATE, 64 * 1024 * 1024, IPC_CREAT | 0600));

        check(big >= 0, "shmget");
        fflush(stdout);
        pid = fork();
        check(pid >= 0, "fork");
        if (pid == 0) {
            struct rlimit rl;

            rl.rlim_cur = rl.rlim_max = (rlim_t)address_space() + 16 * 1024 * 1024;
            if (setrlimit(RLIMIT_AS, &rl) != 0)
                _exit(3);
            if (shmat(big, NULL, 0) != SHM_FAILED)
                _exit(1);
            _exit(errno == ENOMEM ? 0 : 2);
        }
        check(child_status(pid) == 0, "ENOMEM");
    } else {
        printf("no /proc/self/statm: not checking ENOMEM\n");
    }

    /* EACCES: a read-write attach needs both, a read-only one read */
    if (geteuid() != 0) {
        int ro = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0400));
        int wo = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0200));

        check(ro >= 0 && wo >= 0, "shmget");
        check(shmat(ro, NULL, 0) == SHM_FAILED && errno == EACCES, "EACCES read-write");
        p = shmat(ro, NULL, SHM_RDONLY);
        check(p != SHM_FAILED, "read-only attach of a read-only segment");
        check(shmdt(p) == 0, "shmdt");
        check(shmat(wo, NULL, SHM_RDONLY) == SHM_FAILED && errno == EACCES, "EACCES read-only");
        check(shmat(wo, NULL, 0) == SHM_FAILED && errno == EACCES, "EACCES without read");
    } else {
        printf("running as root: not checking EACCES\n");
    }

    /* A removed segment can't be attached */
    check(shmctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    drop(id);
    check(shmat(id, NULL, 0) == SHM_FAILED && errno == EINVAL, "removed");
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
