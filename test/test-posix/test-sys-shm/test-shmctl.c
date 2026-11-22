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
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include "../is-qemu.h"

/*
 * shmctl reads and sets every member of shmid_ds. On 32-bit targets
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

/* Waits until the kernel's clock has passed t, so a new timestamp differs */
static void
pass_second(time_t t)
{
    struct timespec ts = { 0, 10 * 1000 * 1000 };

    while (time(NULL) <= t + 1)
        nanosleep(&ts, NULL);
}

static int
child_status(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

#define SIZE 12345

int
main(void)
{
    struct shmid_ds ds, set;
    int             id, fds[2];
    time_t          before, created, attached, detached;
    pid_t           pid;
    uid_t           other_uid;
    gid_t           other_gid;
    char           *p, c;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's shmid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's shmid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(60);
    check(atexit(cleanup) == 0, "atexit");

    before = time(NULL);
    id = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "shmget");
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    created = ds.shm_ctime;
    check(created >= before - 1 && created <= time(NULL), "shm_ctime");

    /* Attached here a second later */
    pass_second(created);
    p = shmat(id, NULL, 0);
    check(p != (void *)-1, "shmat");
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_atime > created && ds.shm_atime <= time(NULL), "shm_atime");
    check(ds.shm_dtime == 0 && ds.shm_nattch == 1 && ds.shm_lpid == getpid(), "attached");

    /*
     * A child inherits the attachment, which Linux counts as an attach,
     * and a second later detaches it
     */
    check(pipe(fds) == 0, "pipe");
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0) {
        close(fds[1]);
        while (read(fds[0], &c, 1) > 0)
            ;
        _exit(shmdt(p) == 0 ? 0 : 1);
    }
    close(fds[0]);
    check(shmctl(id, IPC_STAT, &ds) == 0 && ds.shm_nattch == 2, "the child is attached");
    attached = ds.shm_atime;
    pass_second(attached);
    close(fds[1]);
    check(child_status(pid) == 0, "the child detached");

    /* Every member holds its own value */
    memset(&ds, 0x55, sizeof(ds));
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_perm.uid == geteuid() && ds.shm_perm.cuid == geteuid(), "uid and cuid");
    check(ds.shm_perm.gid == getegid() && ds.shm_perm.cgid == getegid(), "gid and cgid");
    check((ds.shm_perm.mode & 0777) == 0600, "mode");
    check(ds.shm_segsz == SIZE, "shm_segsz");
    check(ds.shm_lpid == pid, "shm_lpid is the child, which detached last");
    check(ds.shm_cpid == getpid(), "shm_cpid");
    check(ds.shm_nattch == 1, "shm_nattch");
    check(ds.shm_atime == attached, "shm_atime");
    detached = ds.shm_dtime;
    check(detached > attached && detached <= time(NULL), "shm_dtime");
    check(ds.shm_ctime == created, "shm_ctime");

    /* IPC_SET changes the owner and the low nine mode bits, and shm_ctime */
    pass_second(detached);
    set = ds;
    set.shm_perm.mode = 0604;
    /* These are ignored */
    set.shm_perm.cuid = set.shm_perm.uid + 1;
    set.shm_perm.cgid = set.shm_perm.gid + 1;
    set.shm_segsz = 1;
    set.shm_lpid = 1;
    set.shm_cpid = 1;
    set.shm_nattch = 9;
    set.shm_atime = 1;
    set.shm_dtime = 1;
    set.shm_ctime = 1;
    check(shmctl(id, IPC_SET, &set) == 0, "IPC_SET");
    memset(&ds, 0x55, sizeof(ds));
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check((ds.shm_perm.mode & 0777) == 0604, "mode set");
    check(ds.shm_perm.cuid == geteuid() && ds.shm_perm.cgid == getegid(), "cuid and cgid stay");
    check(ds.shm_segsz == SIZE && ds.shm_lpid == pid && ds.shm_cpid == getpid()
              && ds.shm_nattch == 1 && ds.shm_atime == attached && ds.shm_dtime == detached,
          "the other members stay");
    check(ds.shm_ctime > detached && ds.shm_ctime <= time(NULL), "IPC_SET sets shm_ctime");

    /* The owner can give the segment away and, as its creator, still change it */
    other_uid = geteuid() == 65534 ? 65533 : 65534;
    other_gid = getegid() == 65534 ? 65533 : 65534;
    set = ds;
    set.shm_perm.uid = other_uid;
    set.shm_perm.gid = other_gid;
    if (shmctl(id, IPC_SET, &set) == 0) {
        check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
        check(ds.shm_perm.uid == other_uid && ds.shm_perm.gid == other_gid, "uid and gid set");
        check(ds.shm_perm.cuid == geteuid() && ds.shm_perm.cgid == getegid(), "cuid and cgid stay");
        set.shm_perm.uid = geteuid();
        set.shm_perm.gid = getegid();
        check(shmctl(id, IPC_SET, &set) == 0, "the creator sets the owner back");
    } else {
        check(errno == EINVAL, "IPC_SET of another owner");
        printf("uid %ld isn't valid here: not checking a change of owner\n", (long)other_uid);
    }

    /* EACCES: IPC_STAT needs read permission, IPC_SET only ownership */
    if (geteuid() != 0) {
        set.shm_perm.mode = 0200;
        check(shmctl(id, IPC_SET, &set) == 0, "IPC_SET write-only");
        check(shmctl(id, IPC_STAT, &ds) == -1 && errno == EACCES, "EACCES");
        set.shm_perm.mode = 0600;
        check(shmctl(id, IPC_SET, &set) == 0, "IPC_SET without read permission");
    } else {
        printf("running as root: not checking EACCES\n");
    }

    /* EINVAL for a command or an identifier that isn't valid */
    check(shmctl(id, 0x7654, &ds) == -1 && errno == EINVAL, "EINVAL for the command");
    check(shmctl(-1, IPC_STAT, &ds) == -1 && errno == EINVAL, "EINVAL for the identifier");

    /* IPC_RMID while attached: the memory stays until the last detach */
    strcpy(p, "still here");
    check(shmctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    drop(id);
    check(strcmp(p, "still here") == 0, "still attached");
    check(shmdt(p) == 0, "shmdt");
    check(shmctl(id, IPC_STAT, &ds) == -1 && errno == EINVAL, "destroyed at the last detach");

    /* IPC_RMID with nothing attached destroys it at once */
    id = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0600));
    check(id >= 0, "shmget");
    check(shmctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    drop(id);
    check(shmctl(id, IPC_STAT, &ds) == -1 && errno == EINVAL, "destroyed at once");
    check(shmat(id, NULL, 0) == (void *)-1 && errno == EINVAL, "can't attach");

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
