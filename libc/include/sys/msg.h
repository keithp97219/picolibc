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

#ifndef _SYS_MSG_H_
#define _SYS_MSG_H_

#include <sys/cdefs.h>
#include <sys/ipc.h>

_BEGIN_STD_C

typedef unsigned long msgqnum_t;
typedef unsigned long msglen_t;

#define MSG_NOERROR 010000 /* No error if big message */

struct msqid_ds {
    struct ipc_perm msg_perm;   /* Operation permission structure */
    msgqnum_t       msg_qnum;   /* Number of messages currently on queue */
    msglen_t        msg_qbytes; /* Maximum number of bytes allowed on queue */
    pid_t           msg_lspid;  /* Process ID of last msgsnd() */
    pid_t           msg_lrpid;  /* Process ID of last msgrcv() */
    time_t          msg_stime;  /* Time of last msgsnd() */
    time_t          msg_rtime;  /* Time of last msgrcv() */
    time_t          msg_ctime;  /* Time of last change */
};

int     msgctl(int, int, struct msqid_ds *) __picolibc_export;
int     msgget(key_t, int) __picolibc_export;
ssize_t msgrcv(int, void *, size_t, long, int) __picolibc_export;
int     msgsnd(int, const void *, size_t, int) __picolibc_export;

_END_STD_C

#endif /* _SYS_MSG_H_ */
