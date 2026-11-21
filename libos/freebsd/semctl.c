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

/* The fourth argument, when there is one, is the caller's union semun */
union __semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

/* The kernel's __semctl takes a pointer to its own union semun */
union __freebsd_semun {
    int                        val;
    struct __freebsd_semid_ds *buf;
    unsigned short            *array;
};

int
semctl(int id, int num, int cmd, ...)
{
    struct __freebsd_semid_ds k;
    union __semun             arg = { 0 };
    union __freebsd_semun     karg = { 0 };
    int                       kcmd;
    long                      ret;
    va_list                   ap;

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
        karg.buf = &k;
        if (_syscall(FREEBSD_SYS___semctl, id, num, FREEBSD_IPC_STAT, &karg) < 0)
            return -1;
        _ipc_perm_from_freebsd(&arg.buf->sem_perm, &k.sem_perm);
        arg.buf->sem_nsems = k.sem_nsems;
        arg.buf->sem_otime = k.sem_otime;
        arg.buf->sem_ctime = k.sem_ctime;
        return 0;
    case IPC_SET:
        /* The kernel takes only the owner and the mode from this */
        memset(&k, 0, sizeof(k));
        karg.buf = &k;
        _ipc_perm_to_freebsd(&k.sem_perm, &arg.buf->sem_perm);
        return (int)_syscall(FREEBSD_SYS___semctl, id, num, FREEBSD_IPC_SET, &karg);
    case GETALL:
        kcmd = FREEBSD_GETALL;
        karg.array = arg.array;
        break;
    case SETALL:
        kcmd = FREEBSD_SETALL;
        karg.array = arg.array;
        break;
    case SETVAL:
        kcmd = FREEBSD_SETVAL;
        karg.val = arg.val;
        break;
    case IPC_RMID:
        kcmd = FREEBSD_IPC_RMID;
        break;
    case GETPID:
        kcmd = FREEBSD_GETPID;
        break;
    case GETVAL:
        kcmd = FREEBSD_GETVAL;
        break;
    case GETNCNT:
        kcmd = FREEBSD_GETNCNT;
        break;
    case GETZCNT:
        kcmd = FREEBSD_GETZCNT;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    ret = _syscall(FREEBSD_SYS___semctl, id, num, kcmd, &karg);
    /*
     * POSIX has SETVAL and SETALL set sem_ctime, which FreeBSD leaves.
     * Setting the owner and mode to what they are does; only the owner
     * may, so this is as far as it can go for others
     */
    if (ret == 0 && (cmd == SETVAL || cmd == SETALL)) {
        union __freebsd_semun sarg = { .buf = &k };

        if (_syscall_raw(FREEBSD_SYS___semctl, id, 0, FREEBSD_IPC_STAT, &sarg) == 0)
            (void)_syscall_raw(FREEBSD_SYS___semctl, id, 0, FREEBSD_IPC_SET, &sarg);
    }
    return (int)ret;
}
