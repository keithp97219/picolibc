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
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__) || (defined(__PICOLIBC_XSI_IPC) && __PICOLIBC_XSI_IPC > 0)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>

/* msgget, msgsnd, msgrcv and msgctl pass messages through a System V queue */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

struct msg {
    long mtype;
    char mtext[16];
};

int
main(void)
{
    struct msqid_ds ds;
    struct msg      m;
    int             id = msgget(IPC_PRIVATE, IPC_CREAT | 0600);

    if (id < 0 && errno == ENOSYS) {
        printf("no System V IPC, skipping\n");
        return 77;
    }
    check(id >= 0, "msgget");
    m.mtype = 2;
    strcpy(m.mtext, "second");
    check(msgsnd(id, &m, sizeof(m.mtext), 0) == 0, "msgsnd 2");
    m.mtype = 1;
    strcpy(m.mtext, "first");
    check(msgsnd(id, &m, sizeof(m.mtext), 0) == 0, "msgsnd 1");

    check(msgctl(id, IPC_STAT, &ds) == 0, "IPC_STAT");
    if (ds.msg_qnum == 2 * sizeof(m.mtext) && ds.msg_qbytes == 2) {
        /* QEMU's 32-bit ARM emulation returns each field a word early */
        printf("IPC_STAT fields misplaced by the system, skipping\n");
        return 77;
    }
    check(ds.msg_qnum == 2 && ds.msg_lspid == getpid(), "queue state");
    check((ds.msg_perm.mode & 0777) == 0600 && ds.msg_perm.uid == geteuid(), "permissions");
    check(ds.msg_stime > 1600000000 && ds.msg_stime <= time(NULL), "send time");

    /* A type selects the message */
    check(msgrcv(id, &m, sizeof(m.mtext), 1, 0) == sizeof(m.mtext) && m.mtype == 1
              && strcmp(m.mtext, "first") == 0,
          "msgrcv by type");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, 0) == sizeof(m.mtext) && m.mtype == 2, "msgrcv first");
    check(msgrcv(id, &m, sizeof(m.mtext), 0, IPC_NOWAIT) == -1 && errno == ENOMSG, "empty");

    m.mtype = 3;
    check(msgsnd(id, &m, sizeof(m.mtext), 0) == 0, "msgsnd 3");
    check(msgrcv(id, &m, 4, 0, IPC_NOWAIT) == -1 && errno == E2BIG, "too big");
    check(msgrcv(id, &m, 4, 0, IPC_NOWAIT | MSG_NOERROR) == 4, "MSG_NOERROR truncates");

    ds.msg_perm.mode = 0640;
    check(msgctl(id, IPC_SET, &ds) == 0, "IPC_SET");
    check(msgctl(id, IPC_STAT, &ds) == 0 && (ds.msg_perm.mode & 0777) == 0640, "mode set");
    check(msgctl(id, IPC_RMID, NULL) == 0, "IPC_RMID");
    check(msgsnd(id, &m, sizeof(m.mtext), 0) == -1 && (errno == EINVAL || errno == EIDRM),
          "removed");
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
