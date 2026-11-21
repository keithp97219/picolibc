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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>

/*
 * msgctl reads and sets every member of msqid_ds. On 32-bit targets
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

/* Every queue made here, removed on every way out */
static int ids[8];
static int nids;

static void
cleanup(void)
{
    while (nids > 0)
        msgctl(ids[--nids], IPC_RMID, NULL);
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

#ifdef __arm__
/* QEMU's user-mode emulation identifies itself through BogoMIPS */
static bool
is_qemu(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char  line[256];
    bool  ret = false;

    if (!f)
        return false;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strncmp(line, "BogoMIPS", 8) == 0) {
            ret = strstr(line, "100.00") != NULL;
            break;
        }
    }
    fclose(f);
    return ret;
}
#endif

/* Waits until the kernel's clock has passed t, so a new timestamp differs */
static void
pass_second(time_t t)
{
    struct timespec ts = { 0, 10 * 1000 * 1000 };

    while (time(NULL) <= t + 1)
        nanosleep(&ts, NULL);
}

struct msg {
    long mtype;
    char mtext[24];
};

int
main(void)
{
    struct msqid_ds ds, set;
    struct msg      m;
    int             id, status;
    time_t          before, sent, created;
    pid_t           pid;
    uid_t           other_uid;
    gid_t           other_gid;
    msglen_t        qbytes;

    signal(SIGALRM, watchdog);
    alarm(30);
    check(atexit(cleanup) == 0, "atexit");

    before = time(NULL);
    id = keep(msgget(IPC_PRIVATE, IPC_CREAT | 0600));
    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "msgget");
#ifdef __arm__
    if (is_qemu()) {
        printf("QEMU misplaces 32-bit ARM's msqid_ds fields, skipping\n");
        return 77;
    }
#endif
    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    created = ds.msg_ctime;
    check(created >= before - 1 && created <= time(NULL), "msg_ctime");
    qbytes = ds.msg_qbytes;

    /* Three messages from this process, so the times move apart */
    pass_second(created);
    m.mtype = 1;
    memset(m.mtext, 'a', sizeof(m.mtext));
    check(msgsnd(id, &m, 5, 0) == 0, "msgsnd");
    check(msgsnd(id, &m, 7, 0) == 0, "msgsnd");
    check(msgsnd(id, &m, 11, 0) == 0, "msgsnd");
    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    sent = ds.msg_stime;
    check(sent > created && sent <= time(NULL), "msg_stime is the time of the send");
    check(ds.msg_rtime == 0 && ds.msg_ctime == created, "rtime and ctime untouched by msgsnd");

    /* One received by a child, so the two process IDs differ */
    pass_second(sent);
    fflush(stdout);
    pid = fork();
    check(pid >= 0, "fork");
    if (pid == 0)
        _exit(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == 5 ? 0 : 1);
    check(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the child's msgrcv");

    /* Every member holds its own value */
    memset(&ds, 0x55, sizeof(ds));
    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check(ds.msg_perm.uid == geteuid() && ds.msg_perm.cuid == geteuid(), "uid and cuid");
    check(ds.msg_perm.gid == getegid() && ds.msg_perm.cgid == getegid(), "gid and cgid");
    check((ds.msg_perm.mode & 0777) == 0600, "mode");
    check(ds.msg_qnum == 2, "msg_qnum");
    check(ds.msg_qbytes == qbytes, "msg_qbytes");
    check(ds.msg_lspid == getpid(), "msg_lspid");
    check(ds.msg_lrpid == pid, "msg_lrpid");
    check(ds.msg_stime == sent, "msg_stime");
    check(ds.msg_rtime > sent && ds.msg_rtime <= time(NULL), "msg_rtime");
    check(ds.msg_ctime == created, "msg_ctime");

    /* IPC_SET changes the mode, msg_qbytes, uid and gid, and msg_ctime */
    pass_second(ds.msg_rtime);
    set = ds;
    set.msg_perm.mode = 0604;
    set.msg_qbytes = qbytes / 2;
    /* These are ignored */
    set.msg_perm.cuid = set.msg_perm.uid + 1;
    set.msg_perm.cgid = set.msg_perm.gid + 1;
    set.msg_qnum = 99;
    set.msg_lspid = 1;
    set.msg_stime = 1;
    set.msg_ctime = 1;
    check(msgctl(id, IPC_SET, &set) == 0, "IPC_SET");
    memset(&ds, 0x55, sizeof(ds));
    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    check((ds.msg_perm.mode & 0777) == 0604, "mode set");
    check(ds.msg_qbytes == qbytes / 2, "msg_qbytes set");
    check(ds.msg_perm.cuid == geteuid() && ds.msg_perm.cgid == getegid(),
          "cuid and cgid don't change");
    check(ds.msg_qnum == 2 && ds.msg_lspid == getpid() && ds.msg_stime == sent,
          "the other members don't change");
    check(ds.msg_ctime > created && ds.msg_ctime <= time(NULL), "msg_ctime set to now");

    /* The owner can give the queue away and, as its creator, still change it */
    other_uid = geteuid() == 65534 ? 65533 : 65534;
    other_gid = getegid() == 65534 ? 65533 : 65534;
    set = ds;
    set.msg_perm.uid = other_uid;
    set.msg_perm.gid = other_gid;
    if (msgctl(id, IPC_SET, &set) == 0) {
        check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
        check(ds.msg_perm.uid == other_uid && ds.msg_perm.gid == other_gid, "uid and gid set");
        check(ds.msg_perm.cuid == geteuid() && ds.msg_perm.cgid == getegid(), "cuid and cgid stay");
        set.msg_perm.uid = geteuid();
        set.msg_perm.gid = getegid();
        check(msgctl(id, IPC_SET, &set) == 0, "the creator sets the owner back");
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_perm.uid == geteuid()
                  && ds.msg_perm.gid == getegid(),
              "owner back");
    } else {
        /* An ID that doesn't map in this user namespace */
        check(errno == EINVAL, "IPC_SET of another owner");
        printf("uid %ld isn't valid here: not checking a change of owner\n", (long)other_uid);
    }

    if (geteuid() != 0) {
        /* Only a privileged process can raise msg_qbytes past the limit */
        set = ds;
        set.msg_qbytes = qbytes + 1;
        check(msgctl(id, IPC_SET, &set) == -1 && errno == EPERM, "EPERM raising msg_qbytes");
        check(msgctl(id, IPC_STAT, &ds) == 0 && ds.msg_qbytes == qbytes / 2, "unchanged");

        /* IPC_STAT needs read permission, and IPC_SET only ownership */
        set = ds;
        set.msg_perm.mode = 0200;
        check(msgctl(id, IPC_SET, &set) == 0, "IPC_SET write-only");
        check(msgctl(id, IPC_STAT, &ds) == -1 && errno == EACCES, "EACCES");
        set.msg_perm.mode = 0600;
        check(msgctl(id, IPC_SET, &set) == 0, "IPC_SET without read permission");
        check(msgctl(id, IPC_STAT, &ds) == 0, "readable again");
    } else {
        printf("running as root: not checking EPERM and EACCES\n");
    }

    /* EINVAL for a command or an identifier that isn't valid */
    check(msgctl(id, 0x7654, &ds) == -1 && errno == EINVAL, "EINVAL for the command");
    check(msgctl(-1, IPC_STAT, &ds) == -1 && errno == EINVAL, "EINVAL for the id");

    /* IPC_RMID destroys the queue and its messages */
    check(msgctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    nids--;
    check(msgctl(id, IPC_STAT, &ds) == -1 && errno == EINVAL, "gone");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1
              && (errno == EINVAL || errno == EIDRM),
          "no messages");
    check(msgctl(id, IPC_RMID, NULL) == -1 && errno == EINVAL, "IPC_RMID twice");

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
