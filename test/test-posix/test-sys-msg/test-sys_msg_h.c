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

/* What <sys/msg.h> provides */

#define _POSIX_C_SOURCE 202405L
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include <sys/msg.h>

#define same_type(a, T)       _Generic((a), T: 1, default: 0)
#define member_is(type, m, T) same_type(((type *)0)->m, T)

/* msgqnum_t and msglen_t are unsigned and hold at least an unsigned short */
_Static_assert((msgqnum_t)-1 > 0 && (msgqnum_t)-1 >= (unsigned short)-1, "msgqnum_t");
_Static_assert((msglen_t)-1 > 0 && (msglen_t)-1 >= (unsigned short)-1, "msglen_t");

/* The other types, without any other header */
_Static_assert((pid_t)-1 < 0, "pid_t");
_Static_assert((size_t)-1 > 0 && (ssize_t)-1 < 0, "size_t and ssize_t");
_Static_assert((time_t)1 / 2 == 0 || (time_t)1 / 2 > 0, "time_t");

/* MSG_NOERROR is a flag apart from IPC_NOWAIT and the permission bits */
_Static_assert(MSG_NOERROR != 0 && (MSG_NOERROR & (IPC_NOWAIT | 0777)) == 0, "MSG_NOERROR");
#if MSG_NOERROR == 0
#error MSG_NOERROR can be used in #if
#endif

/* struct msqid_ds */
_Static_assert(member_is(struct msqid_ds, msg_perm, struct ipc_perm)
                   && member_is(struct msqid_ds, msg_qnum, msgqnum_t)
                   && member_is(struct msqid_ds, msg_qbytes, msglen_t)
                   && member_is(struct msqid_ds, msg_lspid, pid_t)
                   && member_is(struct msqid_ds, msg_lrpid, pid_t)
                   && member_is(struct msqid_ds, msg_stime, time_t)
                   && member_is(struct msqid_ds, msg_rtime, time_t)
                   && member_is(struct msqid_ds, msg_ctime, time_t),
               "struct msqid_ds members");

/* <sys/msg.h> includes <sys/ipc.h> */
_Static_assert(IPC_CREAT != IPC_EXCL && IPC_RMID != IPC_STAT, "<sys/ipc.h> constants");
_Static_assert(same_type(&ftok, key_t (*)(const char *, int)), "ftok");

/* The prototypes */
_Static_assert(same_type(&msgctl, int (*)(int, int, struct msqid_ds *)), "msgctl");
_Static_assert(same_type(&msgget, int (*)(key_t, int)), "msgget");
_Static_assert(same_type(&msgrcv, ssize_t (*)(int, void *, size_t, long, int)), "msgrcv");
_Static_assert(same_type(&msgsnd, int (*)(int, const void *, size_t, int)), "msgsnd");

#include <stdio.h>

int
main(void)
{
    printf("success\n");
    return 0;
}
