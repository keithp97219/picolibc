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
#include <stdarg.h>
#include <string.h>
#include <sys/sem.h>

/* The fourth argument, when there is one, is the caller's union semun */
union __semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

int
semctl(int id, int num, int cmd, ...)
{
    struct __kernel_semid64_ds k;
    union __semun              arg = { 0 };
    va_list                    ap;

    switch (cmd) {
    case IPC_STAT:
    case IPC_SET:
    case GETALL:
    case SETALL:
    case SETVAL:
        va_start(ap, cmd);
        arg = va_arg(ap, union __semun);
        va_end(ap);
        break;
    default:
        break;
    }

    switch (cmd) {
    case IPC_STAT:
        /*
         * The kernel writes the whole structure, but QEMU's user-mode
         * emulation may fill in only the low half of each time and of
         * the mode; start from zeros so the rest reads as zero
         */
        memset(&k, 0, sizeof(k));
        if (_syscall(LINUX_SYS_semctl, id, num, cmd | LINUX_IPC_64, &k) < 0)
            return -1;
        _ipc_perm_from_linux(&arg.buf->sem_perm, &k.sem_perm);
        arg.buf->sem_nsems = (unsigned short)k.sem_nsems;
        arg.buf->sem_otime = _ipc_time(&k, sem_otime);
        arg.buf->sem_ctime = _ipc_time(&k, sem_ctime);
        return 0;
    case IPC_SET:
        memset(&k, 0, sizeof(k));
        _ipc_perm_to_linux(&k.sem_perm, &arg.buf->sem_perm);
        return (int)_syscall(LINUX_SYS_semctl, id, num, cmd | LINUX_IPC_64, &k);
    case GETALL:
    case SETALL:
        return (int)_syscall(LINUX_SYS_semctl, id, num, cmd, arg.array);
    case SETVAL:
        return (int)_syscall(LINUX_SYS_semctl, id, num, cmd, arg.val);
    case IPC_RMID:
    case GETPID:
    case GETVAL:
    case GETNCNT:
    case GETZCNT:
        return (int)_syscall(LINUX_SYS_semctl, id, num, cmd, 0);
    default:
        errno = EINVAL;
        return -1;
    }
}
