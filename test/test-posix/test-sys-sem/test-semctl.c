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
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/*
 * semctl's commands, and every member of semid_ds. On 32-bit targets
 * the library converts the kernel's structure, so each member is
 * checked against a value that only the right field can hold.
 */

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
/* Waits until the kernel's clock has passed t, so a new timestamp differs */
static void
pass_second(time_t t)
{
    struct timespec ts = { 0, 10 * 1000 * 1000 };

    while (time(NULL) <= t + 1)
        nanosleep(&ts, NULL);
}

/* Waits until semctl's count reaches want */
static bool
wait_count(int id, int num, int cmd, int want)
{
    struct timespec ts = { 0, 1000 * 1000 };
    int             i;

    for (i = 0; i < 20000; i++) {
        if (semctl(id, num, cmd) == want)
            return true;
        nanosleep(&ts, NULL);
    }
    return false;
}

static int
child_status(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

/* A child that does one operation, then waits for the parent before exiting */
static pid_t
child_op(int id, unsigned short num, short op, short flg, int *wfd)
{
    int   up[2], down[2];
    pid_t pid;
    char  c;

    check(pipe(up) == 0 && pipe(down) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf sb = { num, op, flg };

        close(up[0]);
        close(down[1]);
        if (semop(id, &sb, 1) != 0)
            _exit(1);
        if (write(up[1], "x", 1) != 1)
            _exit(2);
        /* EOF from the parent says go */
        while (read(down[0], &c, 1) > 0)
            ;
        _exit(0);
    }
    close(up[1]);
    close(down[0]);
    check(read(up[0], &c, 1) == 1, "the child's operation");
    close(up[0]);
    *wfd = down[1];
    return pid;
}

int
main(void)
{
    struct semid_ds ds, set;
    union semun     arg;
    unsigned short  vals[3], got[3];
    int             id, fd;
    time_t          before, created, changed;
    pid_t           pid, pid2;
    uid_t           other_uid;
    gid_t           other_gid;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's semid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's semid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(60);
    check(atexit(cleanup) == 0, "atexit");

    before = time(NULL);
    id = keep(semget(IPC_PRIVATE, 3, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "semget");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    created = ds.sem_ctime;
    check(created >= before - 1 && created <= time(NULL), "sem_ctime");

    /* SETVAL and GETVAL, with sem_ctime set */
    pass_second(created);
    arg.val = 4;
    check(semctl(id, 1, SETVAL, arg) == 0, "SETVAL returns 0");
    check(semctl(id, 1, GETVAL) == 4, "GETVAL returns semval");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    check(ds.sem_ctime > created && ds.sem_ctime <= time(NULL), "SETVAL sets sem_ctime");
    check(ds.sem_otime == 0, "SETVAL leaves sem_otime");
    changed = ds.sem_ctime;

    /* SETALL and GETALL, with sem_ctime set */
    pass_second(changed);
    vals[0] = 1;
    vals[1] = 2;
    vals[2] = 3;
    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == 0, "SETALL returns 0");
    memset(got, 0, sizeof(got));
    arg.array = got;
    check(semctl(id, 0, GETALL, arg) == 0 && memcmp(got, vals, sizeof(got)) == 0, "GETALL");
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    check(ds.sem_ctime > changed && ds.sem_ctime <= time(NULL), "SETALL sets sem_ctime");
    changed = ds.sem_ctime;

    /* GETPID: the last process to operate on each semaphore */
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf op = { 2, -1, 0 };

        _exit(semop(id, &op, 1) == 0 ? 0 : 1);
    }
    check(child_status(pid) == 0, "child's semop");
    check(semctl(id, 2, GETPID) == pid, "GETPID");
    check(semctl(id, 0, GETPID) != pid, "the others untouched");

    /* Every member of semid_ds holds its own value */
    memset(&ds, 0x55, sizeof(ds));
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    check(ds.sem_perm.uid == geteuid() && ds.sem_perm.cuid == geteuid(), "uid and cuid");
    check(ds.sem_perm.gid == getegid() && ds.sem_perm.cgid == getegid(), "gid and cgid");
    check((ds.sem_perm.mode & 0777) == 0600, "mode");
    check(ds.sem_nsems == 3, "sem_nsems");
    check(ds.sem_otime >= changed && ds.sem_otime <= time(NULL), "sem_otime");
    check(ds.sem_ctime == changed, "sem_ctime");

    /* GETNCNT and GETZCNT count the processes waiting */
    arg.val = 0;
    check(semctl(id, 0, SETVAL, arg) == 0, "SETVAL");
    arg.val = 1;
    check(semctl(id, 1, SETVAL, arg) == 0, "SETVAL");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        struct sembuf op = { 0, -1, 0 };

        _exit(semop(id, &op, 1) == 0 ? 0 : 1);
    }
    pid2 = fork();
    check(pid2 >= 0, "fork");
    if (pid2 == 0) {
        struct sembuf op = { 1, 0, 0 };

        _exit(semop(id, &op, 1) == 0 ? 0 : 1);
    }
    check(wait_count(id, 0, GETNCNT, 1), "GETNCNT");
    check(wait_count(id, 1, GETZCNT, 1), "GETZCNT");
    check(semctl(id, 0, GETZCNT) == 0 && semctl(id, 1, GETNCNT) == 0, "only those");
    arg.val = 1;
    check(semctl(id, 0, SETVAL, arg) == 0, "release the decrement");
    arg.val = 0;
    check(semctl(id, 1, SETVAL, arg) == 0, "release the wait for zero");
    check(child_status(pid) == 0 && child_status(pid2) == 0, "both waiters done");
    check(semctl(id, 0, GETNCNT) == 0 && semctl(id, 1, GETZCNT) == 0, "no one waiting");

    /* SETVAL clears the semadj values: the child's undo doesn't happen */
    arg.val = 5;
    check(semctl(id, 0, SETVAL, arg) == 0, "SETVAL");
    pid = child_op(id, 0, 1, SEM_UNDO, &fd);
    check(semctl(id, 0, GETVAL) == 6, "the child's increment");
    arg.val = 10;
    check(semctl(id, 0, SETVAL, arg) == 0, "SETVAL");
    close(fd);
    check(child_status(pid) == 0, "child");
    check(semctl(id, 0, GETVAL) == 10, "SETVAL cleared the semadj");

    /* Without SETVAL the undo happens, for comparison */
    pid = child_op(id, 0, 1, SEM_UNDO, &fd);
    close(fd);
    check(child_status(pid) == 0, "child");
    check(semctl(id, 0, GETVAL) == 10, "the undo happens");

    /* SETALL clears them too */
    pid = child_op(id, 2, 2, SEM_UNDO, &fd);
    vals[0] = 7;
    vals[1] = 8;
    vals[2] = 9;
    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == 0, "SETALL");
    close(fd);
    check(child_status(pid) == 0, "child");
    check(semctl(id, 2, GETVAL) == 9, "SETALL cleared the semadj");

    /* ERANGE past the system's limit, which is 32767 on Linux */
    arg.val = INT_MAX;
    check(semctl(id, 0, SETVAL, arg) == -1 && errno == ERANGE, "ERANGE for SETVAL");
#ifdef __linux__
    arg.val = 32768;
    check(semctl(id, 0, SETVAL, arg) == -1 && errno == ERANGE, "ERANGE for SETVAL 32768");
    arg.val = 32767;
    check(semctl(id, 0, SETVAL, arg) == 0, "32767 is allowed");
    vals[0] = 1;
    vals[1] = 32768;
    vals[2] = 1;
    arg.array = vals;
    check(semctl(id, 0, SETALL, arg) == -1 && errno == ERANGE, "ERANGE for SETALL");
    check(semctl(id, 0, GETVAL) == 32767 && semctl(id, 2, GETVAL) == 9, "SETALL changed nothing");
#endif

    /* EINVAL for semnum out of range, a bad command or a bad identifier */
    arg.val = 1;
    check(semctl(id, 3, GETVAL) == -1 && errno == EINVAL, "EINVAL for semnum nsems");
    check(semctl(id, -1, GETVAL) == -1 && errno == EINVAL, "EINVAL for semnum -1");
    check(semctl(id, 3, SETVAL, arg) == -1 && errno == EINVAL, "EINVAL for SETVAL");
    check(semctl(id, 3, GETPID) == -1 && errno == EINVAL, "EINVAL for GETPID");
    check(semctl(id, 3, GETNCNT) == -1 && errno == EINVAL, "EINVAL for GETNCNT");
    check(semctl(id, 3, GETZCNT) == -1 && errno == EINVAL, "EINVAL for GETZCNT");
    check(semctl(id, 0, 0x7654) == -1 && errno == EINVAL, "EINVAL for the command");
    check(semctl(-1, 0, GETVAL) == -1 && errno == EINVAL, "EINVAL for the identifier");

    /* IPC_SET changes the owner and the mode bits, and sem_ctime */
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    pass_second(ds.sem_ctime);
    set = ds;
    set.sem_perm.mode = 0604;
    /* These are ignored */
    set.sem_perm.cuid = set.sem_perm.uid + 1;
    set.sem_perm.cgid = set.sem_perm.gid + 1;
    set.sem_nsems = 1;
    set.sem_otime = 1;
    set.sem_ctime = 1;
    arg.buf = &set;
    check(semctl(id, 0, IPC_SET, arg) == 0, "IPC_SET");
    memset(&ds, 0x55, sizeof(ds));
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    check((ds.sem_perm.mode & 0777) == 0604, "mode set");
    check(ds.sem_perm.cuid == geteuid() && ds.sem_perm.cgid == getegid(), "cuid and cgid stay");
    check(ds.sem_nsems == 3 && ds.sem_otime != 1, "the other members stay");
    check(ds.sem_ctime > changed && ds.sem_ctime <= time(NULL), "IPC_SET sets sem_ctime");

    /* The owner can give the set away and, as its creator, still change it */
    other_uid = geteuid() == 65534 ? 65533 : 65534;
    other_gid = getegid() == 65534 ? 65533 : 65534;
    set = ds;
    set.sem_perm.uid = other_uid;
    set.sem_perm.gid = other_gid;
    arg.buf = &set;
    if (semctl(id, 0, IPC_SET, arg) == 0) {
        arg.buf = &ds;
        check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
        check(ds.sem_perm.uid == other_uid && ds.sem_perm.gid == other_gid, "uid and gid set");
        check(ds.sem_perm.cuid == geteuid() && ds.sem_perm.cgid == getegid(), "cuid and cgid stay");
        set.sem_perm.uid = geteuid();
        set.sem_perm.gid = getegid();
        arg.buf = &set;
        check(semctl(id, 0, IPC_SET, arg) == 0, "the creator sets the owner back");
    } else {
        check(errno == EINVAL, "IPC_SET of another owner");
        printf("uid %ld isn't valid here: not checking a change of owner\n", (long)other_uid);
    }

    /* EACCES: reading needs read permission, and setting alter permission */
    if (geteuid() != 0) {
        set.sem_perm.mode = 0200;
        arg.buf = &set;
        check(semctl(id, 0, IPC_SET, arg) == 0, "IPC_SET write-only");
        arg.buf = &ds;
        check(semctl(id, 0, IPC_STAT, arg) == -1 && errno == EACCES, "EACCES for IPC_STAT");
        check(semctl(id, 0, GETVAL) == -1 && errno == EACCES, "EACCES for GETVAL");
        check(semctl(id, 0, GETPID) == -1 && errno == EACCES, "EACCES for GETPID");
        check(semctl(id, 0, GETNCNT) == -1 && errno == EACCES, "EACCES for GETNCNT");
        check(semctl(id, 0, GETZCNT) == -1 && errno == EACCES, "EACCES for GETZCNT");
        arg.array = got;
        check(semctl(id, 0, GETALL, arg) == -1 && errno == EACCES, "EACCES for GETALL");
        set.sem_perm.mode = 0400;
        arg.buf = &set;
        check(semctl(id, 0, IPC_SET, arg) == 0, "IPC_SET read-only");
        arg.val = 1;
        check(semctl(id, 0, SETVAL, arg) == -1 && errno == EACCES, "EACCES for SETVAL");
        arg.array = vals;
        check(semctl(id, 0, SETALL, arg) == -1 && errno == EACCES, "EACCES for SETALL");
    } else {
        printf("running as root: not checking EACCES\n");
    }

    /* IPC_RMID removes the set */
    check(semctl(id, 0, IPC_RMID) == 0, "IPC_RMID returns 0");
    drop(id);
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == -1 && errno == EINVAL, "gone");
    check(semctl(id, 0, IPC_RMID) == -1 && errno == EINVAL, "IPC_RMID twice");

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
