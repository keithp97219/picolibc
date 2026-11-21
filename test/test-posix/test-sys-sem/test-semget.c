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
#if defined(__PICOLIBC_XSI_IPC)

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <time.h>
#include "../is-qemu.h"

/* semget creates and finds semaphore sets, and initializes them */

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

static char dir[] = "/tmp/semget-XXXXXX";
static char path[sizeof(dir) + 8];

static void
remove_key_file(void)
{
    unlink(path);
    rmdir(dir);
}

int
main(void)
{
    struct semid_ds ds;
    union semun     arg;
    unsigned short  vals[3];
    key_t           key = -1;
    int             a, b, id, proj, i;
    time_t          before;
    FILE           *f;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's semid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's semid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(remove_key_file) == 0 && atexit(cleanup) == 0, "atexit");

    a = keep(semget(IPC_PRIVATE, 1, IPC_CREAT | 0600));
    if (a < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(a >= 0, "semget");

    /* IPC_PRIVATE always makes a new set, with or without IPC_CREAT */
    b = keep(semget(IPC_PRIVATE, 1, 0600));
    check(b >= 0 && b != a, "a second private set");

    /* EINVAL when creating with nsems <= 0 or past the limit */
    check(semget(IPC_PRIVATE, 0, IPC_CREAT | 0600) == -1 && errno == EINVAL, "EINVAL for 0");
    check(semget(IPC_PRIVATE, -1, IPC_CREAT | 0600) == -1 && errno == EINVAL, "EINVAL for -1");
    check(semget(IPC_PRIVATE, INT_MAX, IPC_CREAT | 0600) == -1 && errno == EINVAL,
          "EINVAL past the limit");

    /* A key from a file of our own; take one nobody else is using */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/key", dir);
    f = fopen(path, "w");
    check(f != NULL, "key file");
    fclose(f);
    for (proj = 1; proj < 256; proj++) {
        key = ftok(path, proj);
        check(key != (key_t)-1, "ftok");
        if (semget(key, 0, 0600) == -1 && errno == ENOENT)
            break;
    }
    check(proj < 256, "an unused key");

    /* Without IPC_CREAT, a key without a set is ENOENT */
    check(semget(key, 3, 0600) == -1 && errno == ENOENT, "ENOENT");

    before = time(NULL);
    id = keep(semget(key, 3, IPC_CREAT | IPC_EXCL | 0640));
    check(id >= 0, "create with a key");
    check(semget(key, 3, 0600) == id, "found again");
    check(semget(key, 0, 0) == id, "found with nsems 0");
    check(semget(key, 2, IPC_CREAT | 0600) == id, "found asking for fewer semaphores");
    check(semget(key, 4, 0600) == -1 && errno == EINVAL, "EINVAL asking for more");
    check(semget(key, 3, IPC_CREAT | IPC_EXCL | 0600) == -1 && errno == EEXIST, "EEXIST");

    /* The new set's semid_ds comes from the flags and the caller */
    arg.buf = &ds;
    check(semctl(id, 0, IPC_STAT, arg) == 0, "IPC_STAT");
    check(ds.sem_perm.cuid == geteuid() && ds.sem_perm.uid == geteuid(), "cuid and uid");
    check(ds.sem_perm.cgid == getegid() && ds.sem_perm.gid == getegid(), "cgid and gid");
    check((ds.sem_perm.mode & 0777) == 0640, "the low 9 bits of the mode");
    check(ds.sem_nsems == 3, "sem_nsems");
    check(ds.sem_otime == 0, "sem_otime");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.sem_ctime >= before - 1 && ds.sem_ctime <= time(NULL), "sem_ctime");

    /* Each semaphore starts with semval, sempid, semncnt and semzcnt zero */
    vals[0] = vals[1] = vals[2] = 0xffff;
    arg.array = vals;
    check(semctl(id, 0, GETALL, arg) == 0 && vals[0] == 0 && vals[1] == 0 && vals[2] == 0,
          "semval");
    for (i = 0; i < 3; i++) {
        check(semctl(id, i, GETVAL) == 0, "GETVAL");
        check(semctl(id, i, GETPID) == 0, "sempid");
        check(semctl(id, i, GETNCNT) == 0, "semncnt");
        check(semctl(id, i, GETZCNT) == 0, "semzcnt");
    }

    /* EACCES when the set doesn't grant the permissions asked for */
    if (geteuid() != 0) {
        ds.sem_perm.mode = 0400;
        arg.buf = &ds;
        check(semctl(id, 0, IPC_SET, arg) == 0, "IPC_SET");
        check(semget(key, 0, 0200) == -1 && errno == EACCES, "EACCES for alter");
        check(semget(key, 0, 0400) == id, "read is still granted");
    } else {
        printf("running as root: not checking EACCES\n");
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
