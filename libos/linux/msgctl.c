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

#include "local-ipc.h"
#include <string.h>
#include <sys/msg.h>

int
msgctl(int id, int cmd, struct msqid_ds *buf)
{
    struct __kernel_msqid64_ds k;

    switch (cmd) {
    case IPC_STAT:
        /*
         * The kernel writes the whole structure, but QEMU's user-mode
         * emulation may fill in only the low half of each time and of
         * the mode; start from zeros so the rest reads as zero
         */
        memset(&k, 0, sizeof(k));
        if (_syscall(LINUX_SYS_msgctl, id, cmd | LINUX_IPC_64, &k) < 0)
            return -1;
        _ipc_perm_from_linux(&buf->msg_perm, &k.msg_perm);
        buf->msg_qnum = k.msg_qnum;
        buf->msg_qbytes = k.msg_qbytes;
        buf->msg_lspid = k.msg_lspid;
        buf->msg_lrpid = k.msg_lrpid;
        buf->msg_stime = _ipc_time(&k, msg_stime);
        buf->msg_rtime = _ipc_time(&k, msg_rtime);
        buf->msg_ctime = _ipc_time(&k, msg_ctime);
        return 0;
    case IPC_SET:
        memset(&k, 0, sizeof(k));
        _ipc_perm_to_linux(&k.msg_perm, &buf->msg_perm);
        k.msg_qbytes = buf->msg_qbytes;
        return (int)_syscall(LINUX_SYS_msgctl, id, cmd | LINUX_IPC_64, &k);
    case IPC_RMID:
        return (int)_syscall(LINUX_SYS_msgctl, id, cmd, NULL);
    default:
        errno = EINVAL;
        return -1;
    }
}
