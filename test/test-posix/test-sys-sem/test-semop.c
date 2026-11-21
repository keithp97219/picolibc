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
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/* semop applies arrays of operations atomically, in order */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* The application declares union semun */
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

/* Every set made here, removed on every way out */
static int ids[8];
static int nids;

static void
cleanup(void)
{
    while (nids > 0)
        semctl(ids[--nids], 0, IPC_RMID);
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

static void
set_all(int id, unsigned short a, unsigned short b)
{
    unsigned short vals[2] = { a, b };
    union semun    arg;

    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == 0, "SETALL");
}

static int
value(int id, int num)
{
    return semctl(id, num, GETVAL);
}

/* The third field of /proc/sys/kernel/sem is the most operations per call */
static long
semopm(void)
{
    FILE *f = fopen("/proc/sys/kernel/sem", "r");
    long  msl, mns, opm = -1;

    if (f) {
        if (fscanf(f, "%ld %ld %ld", &msl, &mns, &opm) != 3)
            opm = -1;
        fclose(f);
    }
    return opm;
}

int
main(void)
{
    struct semid_ds ds;
    struct sembuf   ops[3], copy[3];
    union semun     arg;
    pid_t           pid;
    time_t          before;
    long            opm;
    int             id;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's semid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's semid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(cleanup) == 0, "atexit");

    id = keep(semget(IPC_PRIVATE, 2, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "semget");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0 && ds.sem_otime == 0, "no sem_otime yet");

    /* A negative sem_op subtracts, a positive one adds, zero waits for zero */
    set_all(id, 3, 0);
    ops[0] = (struct sembuf) { 0, -2, 0 };
    check(semop(id, ops, 1) == 0 && value(id, 0) == 1, "subtract");
    ops[0] = (struct sembuf) { 1, 5, 0 };
    check(semop(id, ops, 1) == 0 && value(id, 1) == 5, "add");
    ops[0] = (struct sembuf) { 0, -1, 0 };
    ops[1] = (struct sembuf) { 0, 0, 0 };
    check(semop(id, ops, 2) == 0 && value(id, 0) == 0, "subtract to zero, then zero");

    /* IPC_NOWAIT gives EAGAIN instead of waiting */
    ops[0] = (struct sembuf) { 0, -1, IPC_NOWAIT };
    check(semop(id, ops, 1) == -1 && errno == EAGAIN, "EAGAIN for a decrement");
    ops[0] = (struct sembuf) { 1, 0, IPC_NOWAIT };
    check(semop(id, ops, 1) == -1 && errno == EAGAIN, "EAGAIN waiting for zero");

    /* The array is done in order: an increment lets a later decrement through */
    set_all(id, 0, 0);
    ops[0] = (struct sembuf) { 0, 1, 0 };
    ops[1] = (struct sembuf) { 0, -1, IPC_NOWAIT };
    check(semop(id, ops, 2) == 0 && value(id, 0) == 0, "increment, then decrement");
    ops[0] = (struct sembuf) { 0, -1, IPC_NOWAIT };
    ops[1] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, 2) == -1 && errno == EAGAIN && value(id, 0) == 0,
          "a decrement first can't wait for the increment after it");

    /* All or nothing: a failure undoes the operations before it */
    ops[0] = (struct sembuf) { 0, 2, 0 };
    ops[1] = (struct sembuf) { 1, 3, 0 };
    ops[2] = (struct sembuf) { 1, -4, IPC_NOWAIT };
    memcpy(copy, ops, sizeof(ops));
    check(semop(id, ops, 3) == -1 && errno == EAGAIN, "the last operation fails");
    check(value(id, 0) == 0 && value(id, 1) == 0, "nothing changed");
    check(memcmp(copy, ops, sizeof(ops)) == 0, "the array is not modified");

    /* Success sets sempid for each semaphore named, and sem_otime */
    set_all(id, 0, 0);
    before = time(NULL);
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf both[2] = {
            { 0, 1, 0 },
            { 1, 1, 0 }
        };

        _exit(semop(id, both, 2) == 0 ? 0 : 1);
    }
    check(child_status(pid) == 0, "the child's semop");
    check(semctl(id, 0, GETPID) == pid && semctl(id, 1, GETPID) == pid, "sempid");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.sem_otime >= before - 1 && ds.sem_otime <= time(NULL), "sem_otime");

    /* A failed semop leaves sempid alone */
    ops[0] = (struct sembuf) { 0, -5, IPC_NOWAIT };
    check(semop(id, ops, 1) == -1 && errno == EAGAIN, "EAGAIN");
    check(semctl(id, 0, GETPID) == pid, "sempid unchanged");

    /* EFBIG for a semaphore number past the set */
    ops[0] = (struct sembuf) { 2, 1, 0 };
    check(semop(id, ops, 1) == -1 && errno == EFBIG, "EFBIG");

    /* E2BIG for more operations than the system allows */
    opm = semopm();
    if (opm > 0 && opm < 100000) {
        struct sembuf *many = calloc((size_t)opm + 1, sizeof(*many));

        check(many != NULL, "calloc");
        check(semop(id, many, (size_t)opm + 1) == -1 && errno == E2BIG, "E2BIG");
        free(many);
    } else {
        printf("no SEMOPM: not checking E2BIG past it\n");
    }
#if SIZE_MAX > 0xffffffff && !defined(__GLIBC__)
    /* A count the kernel's 32 bits can't hold; glibc 2.43 truncates it */
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, ((size_t)1 << 32) | 1) == -1 && errno == E2BIG, "E2BIG past 32 bits");
    check(value(id, 0) == 1, "nothing done");
#endif

    /* ERANGE when semval would pass the limit, 32767 on Linux */
#ifdef __linux__
    set_all(id, 32767, 0);
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, 1) == -1 && errno == ERANGE && value(id, 0) == 32767, "ERANGE for semval");

    /* or when semadj would pass it */
    set_all(id, 0, 0);
    ops[0] = (struct sembuf) { 1, 32767, SEM_UNDO };
    ops[1] = (struct sembuf) { 1, -32767, 0 };
    check(semop(id, ops, 2) == 0, "semadj -32767");
    ops[0] = (struct sembuf) { 1, 2, SEM_UNDO };
    check(semop(id, ops, 1) == -1 && errno == ERANGE && value(id, 1) == 0, "ERANGE for semadj");
    ops[0] = (struct sembuf) { 1, 1, SEM_UNDO };
    ops[1] = (struct sembuf) { 1, -1, 0 };
    check(semop(id, ops, 2) == 0, "semadj -32768 is allowed");
#endif

    /* EINVAL for an identifier that isn't valid */
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(-1, ops, 1) == -1 && errno == EINVAL, "EINVAL");

    /* EACCES: decrements and increments need alter permission, zero read */
    if (geteuid() != 0) {
        int ro = keep(semget(IPC_PRIVATE, 1, IPC_CREAT | 0400));
        int wo = keep(semget(IPC_PRIVATE, 1, IPC_CREAT | 0200));

        check(ro >= 0 && wo >= 0, "semget");
        ops[0] = (struct sembuf) { 0, 1, IPC_NOWAIT };
        check(semop(ro, ops, 1) == -1 && errno == EACCES, "EACCES to add");
        ops[0] = (struct sembuf) { 0, -1, IPC_NOWAIT };
        check(semop(ro, ops, 1) == -1 && errno == EACCES, "EACCES to subtract");
        ops[0] = (struct sembuf) { 0, 0, IPC_NOWAIT };
        check(semop(ro, ops, 1) == 0, "read is enough to wait for zero");
        check(semop(wo, ops, 1) == -1 && errno == EACCES, "EACCES to wait for zero");
        ops[0] = (struct sembuf) { 0, 1, IPC_NOWAIT };
        check(semop(wo, ops, 1) == 0, "alter is enough to add");
    } else {
        printf("running as root: not checking EACCES\n");
    }

    /* Removed: EINVAL or EIDRM */
    check(semctl(id, 0, IPC_RMID) == 0, "IPC_RMID");
    drop(id);
    ops[0] = (struct sembuf) { 0, 1, 0 };
    check(semop(id, ops, 1) == -1 && (errno == EINVAL || errno == EIDRM), "removed");

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
