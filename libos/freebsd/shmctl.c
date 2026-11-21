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

int
shmctl(int id, int cmd, struct shmid_ds *buf)
{
    struct __freebsd_shmid_ds k;

    switch (cmd) {
    case IPC_STAT:
        if (_syscall(FREEBSD_SYS_shmctl, id, FREEBSD_IPC_STAT, &k) < 0)
            return -1;
        _ipc_perm_from_freebsd(&buf->shm_perm, &k.shm_perm);
        buf->shm_segsz = k.shm_segsz;
        buf->shm_lpid = k.shm_lpid;
        buf->shm_cpid = k.shm_cpid;
        buf->shm_nattch = k.shm_nattch;
        buf->shm_atime = k.shm_atime;
        buf->shm_dtime = k.shm_dtime;
        buf->shm_ctime = k.shm_ctime;
        return 0;
    case IPC_SET:
        /* The kernel takes only the owner and the mode from this */
        memset(&k, 0, sizeof(k));
        _ipc_perm_to_freebsd(&k.shm_perm, &buf->shm_perm);
        return (int)_syscall(FREEBSD_SYS_shmctl, id, FREEBSD_IPC_SET, &k);
    case IPC_RMID:
        return (int)_syscall(FREEBSD_SYS_shmctl, id, FREEBSD_IPC_RMID, NULL);
    default:
        errno = EINVAL;
        return -1;
    }
}
