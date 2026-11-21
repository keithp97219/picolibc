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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#include "../is-qemu.h"

/* msgget creates and finds message queues, and initializes msqid_ds */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

/* Every queue made here, removed on every way out */
static int  ids[8];
static int  nids;
static char dir[] = "/tmp/msgget-XXXXXX";
static char path[sizeof(dir) + 8];

static void
cleanup(void)
{
    while (nids > 0)
        msgctl(ids[--nids], IPC_RMID, NULL);
    unlink(path);
    rmdir(dir);
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

/* A value from /proc/sys/kernel, or -1 */
static long
kernel_limit(const char *name)
{
    char  file[64];
    FILE *f;
    long  v = -1;

    snprintf(file, sizeof(file), "/proc/sys/kernel/%s", name);
    f = fopen(file, "r");
    if (f) {
        if (fscanf(f, "%ld", &v) != 1)
            v = -1;
        fclose(f);
    }
    return v;
}

int
main(void)
{
    struct msqid_ds ds;
    key_t           key = -1;
    int             a, b, id, proj;
    time_t          before;
    FILE           *f;
    long            mnb;

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(cleanup) == 0, "atexit");

    a = keep(msgget(IPC_PRIVATE, IPC_CREAT | 0600));
    if (a < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(a >= 0, "msgget");

    /* IPC_PRIVATE always makes a new queue, with or without IPC_CREAT */
    b = keep(msgget(IPC_PRIVATE, 0600));
    check(b >= 0 && b != a, "a second private queue");

    /* A key from a file of our own; take one nobody else is using */
    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/key", dir);
    f = fopen(path, "w");
    check(f != NULL, "key file");
    fclose(f);
    for (proj = 1; proj < 256; proj++) {
        key = ftok(path, proj);
        check(key != (key_t)-1, "ftok");
        if (msgget(key, 0600) == -1 && errno == ENOENT)
            break;
    }
    check(proj < 256, "an unused key");

    /* Without IPC_CREAT, a key without a queue is ENOENT */
    check(msgget(key, 0600) == -1 && errno == ENOENT, "ENOENT");

    /* IPC_CREAT makes it, initialized from the flags and the caller */
    before = time(NULL);
    id = keep(msgget(key, IPC_CREAT | IPC_EXCL | 0640));
    check(id >= 0, "create with a key");
    check(msgget(key, 0) == id, "found again with no flags");
    check(msgget(key, 0600) == id, "found again asking for permissions granted");
    check(msgget(key, IPC_CREAT | 0600) == id, "IPC_CREAT finds the existing queue");
    check(msgget(key, IPC_CREAT | IPC_EXCL | 0600) == -1 && errno == EEXIST, "EEXIST");

#ifdef __arm__
    if (is_qemu()) {
        /* QEMU's 32-bit ARM emulation misplaces the msqid_ds fields */
        printf("QEMU: not checking the msqid_ds fields\n");
        printf("success\n");
        return 0;
    }
#endif
    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.msg_perm.cuid == geteuid() && ds.msg_perm.uid == geteuid(), "cuid and uid");
    check(ds.msg_perm.cgid == getegid() && ds.msg_perm.gid == getegid(), "cgid and gid");
    check((ds.msg_perm.mode & 0777) == 0640, "the low 9 bits of the mode");
    check(ds.msg_qnum == 0 && ds.msg_lspid == 0 && ds.msg_lrpid == 0,
          "empty, no sender or receiver");
    check(ds.msg_stime == 0 && ds.msg_rtime == 0, "no send or receive time");
    /* The kernel's clock in seconds may trail time() by a tick */
    check(ds.msg_ctime >= before - 1 && ds.msg_ctime <= time(NULL), "ctime is the current time");
    mnb = kernel_limit("msgmnb");
    if (mnb > 0)
        check(ds.msg_qbytes == (msglen_t)mnb, "msg_qbytes is the system limit");
    else
        check(ds.msg_qbytes > 0, "msg_qbytes is the system limit");

    /* EACCES when the queue doesn't grant the permissions asked for */
    if (geteuid() != 0) {
        ds.msg_perm.mode = 0400;
        check(msgctl(id, IPC_SET, &ds) == 0, "IPC_SET");
        check(msgget(key, 0200) == -1 && errno == EACCES, "EACCES for write");
        check(msgget(key, 0600) == -1 && errno == EACCES, "EACCES for read and write");
        check(msgget(key, 0400) == id, "read is still granted");
        check(msgget(key, 0) == id, "asking for nothing is granted");
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
