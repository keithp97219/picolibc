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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>
#include "../is-qemu.h"

/* shmget creates and finds segments, initialized to zero */

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
static char dir[] = "/tmp/shmget-XXXXXX";
static char path[sizeof(dir) + 8];

static void
remove_key_file(void)
{
    unlink(path);
    rmdir(dir);
}

/* shmmax from /proc/sys/kernel, or 0 */
static unsigned long long
shmmax(void)
{
    FILE              *f = fopen("/proc/sys/kernel/shmmax", "r");
    unsigned long long v = 0;

    if (f) {
        if (fscanf(f, "%llu", &v) != 1)
            v = 0;
        fclose(f);
    }
    return v;
}

#define SIZE 5000

int
main(void)
{
    struct shmid_ds    ds;
    key_t              key = -1;
    int                a, b, id, proj;
    time_t             before;
    FILE              *f;
    unsigned char     *p;
    size_t             i;
    unsigned long long max;

#ifdef __arm__
    /* QEMU misplaces the fields of 32-bit ARM's shmid_ds */
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's shmid_ds fields, skipping\n");
        return 77;
    }
#endif

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(remove_key_file) == 0 && atexit(cleanup) == 0, "atexit");

    a = keep(shmget(IPC_PRIVATE, SIZE, IPC_CREAT | 0600));
    if (a < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(a >= 0, "shmget");

    /* IPC_PRIVATE always makes a new segment, with or without IPC_CREAT */
    b = keep(shmget(IPC_PRIVATE, SIZE, 0600));
    check(b >= 0 && b != a, "a second private segment");

    /* EINVAL for a size below the minimum, or above the maximum */
    check(shmget(IPC_PRIVATE, 0, IPC_CREAT | 0600) == -1 && errno == EINVAL, "EINVAL for 0");
    max = shmmax();
    if (max > 0 && max < SIZE_MAX)
        check(shmget(IPC_PRIVATE, SIZE_MAX, IPC_CREAT | 0600) == -1 && errno == EINVAL,
              "EINVAL past shmmax");
    else
        printf("shmmax %llu: not checking EINVAL past it\n", max);

    /* A key from a file of our own; take one nobody else is using */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/key", dir);
    f = fopen(path, "w");
    check(f != NULL, "key file");
    fclose(f);
    for (proj = 1; proj < 256; proj++) {
        key = ftok(path, proj);
        check(key != (key_t)-1, "ftok");
        if (shmget(key, 0, 0600) == -1 && errno == ENOENT)
            break;
    }
    check(proj < 256, "an unused key");

    /* Without IPC_CREAT, a key without a segment is ENOENT */
    check(shmget(key, SIZE, 0600) == -1 && errno == ENOENT, "ENOENT");

    before = time(NULL);
    id = keep(shmget(key, SIZE, IPC_CREAT | IPC_EXCL | 0640));
    check(id >= 0, "create with a key");
    check(shmget(key, SIZE, 0600) == id, "found again");
    check(shmget(key, 0, 0) == id, "found with size 0");
    check(shmget(key, 100, IPC_CREAT | 0600) == id, "found asking for less");
    check(shmget(key, SIZE + 1, 0600) == -1 && errno == EINVAL, "EINVAL asking for more");
    check(shmget(key, SIZE, IPC_CREAT | IPC_EXCL | 0600) == -1 && errno == EEXIST, "EEXIST");

    /* The new segment's shmid_ds comes from the flags, the size and the caller */
    check(shmctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.shm_perm.cuid == geteuid() && ds.shm_perm.uid == geteuid(), "cuid and uid");
    check(ds.shm_perm.cgid == getegid() && ds.shm_perm.gid == getegid(), "cgid and gid");
    check((ds.shm_perm.mode & 0777) == 0640, "the low 9 bits of the mode");
    check(ds.shm_segsz == SIZE, "shm_segsz is the size asked for");
    check(ds.shm_lpid == 0 && ds.shm_nattch == 0, "shm_lpid and shm_nattch");
    check(ds.shm_atime == 0 && ds.shm_dtime == 0, "shm_atime and shm_dtime");
    check(ds.shm_cpid == getpid(), "shm_cpid");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.shm_ctime >= before - 1 && ds.shm_ctime <= time(NULL), "shm_ctime");

    /* The segment starts out zero */
    p = shmat(id, NULL, 0);
    check(p != (void *)-1, "shmat");
    for (i = 0; i < SIZE; i++)
        check(p[i] == 0, "zero");
    check(shmdt(p) == 0, "shmdt");

    /* EACCES when the segment doesn't grant the permissions asked for */
    if (geteuid() != 0) {
        ds.shm_perm.mode = 0400;
        check(shmctl(id, IPC_SET, &ds) == 0, "IPC_SET");
        check(shmget(key, 0, 0600) == -1 && errno == EACCES, "EACCES for write");
        check(shmget(key, 0, 0400) == id, "read is still granted");
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
