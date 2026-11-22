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

#ifndef _LOCAL_IPC_H_
#define _LOCAL_IPC_H_

#include "local-freebsd.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <freebsd/freebsd-ipc.h>
#include <freebsd/freebsd-ipc_perm-struct.h>
#include <freebsd/freebsd-msqid_ds-struct.h>
#include <freebsd/freebsd-semid_ds-struct.h>
#include <freebsd/freebsd-sembuf-struct.h>
#include <freebsd/freebsd-shmid_ds-struct.h>

/* The flags and the commands common to all three are FreeBSD's */
#define _IPC_SAME(name) _Static_assert(name == FREEBSD_##name, #name)
_IPC_SAME(IPC_CREAT);
_IPC_SAME(IPC_EXCL);
_IPC_SAME(IPC_NOWAIT);
_IPC_SAME(IPC_PRIVATE);
_IPC_SAME(IPC_RMID);
_IPC_SAME(IPC_SET);
_IPC_SAME(IPC_STAT);
_IPC_SAME(MSG_NOERROR);
_IPC_SAME(SEM_UNDO);
_IPC_SAME(SHM_RDONLY);
_IPC_SAME(SHM_RND);
#undef _IPC_SAME

/* semop hands the array to the kernel as it is */
_Static_assert(sizeof(struct sembuf) == sizeof(struct __freebsd_sembuf), "sembuf size");
_Static_assert(__builtin_offsetof(struct sembuf, sem_num)
                   == __builtin_offsetof(struct __freebsd_sembuf, sem_num),
               "sembuf.sem_num");
_Static_assert(__builtin_offsetof(struct sembuf, sem_op)
                   == __builtin_offsetof(struct __freebsd_sembuf, sem_op),
               "sembuf.sem_op");
_Static_assert(__builtin_offsetof(struct sembuf, sem_flg)
                   == __builtin_offsetof(struct __freebsd_sembuf, sem_flg),
               "sembuf.sem_flg");

/* The kernel's keys are longs; keep picolibc's bits as they are */
static inline long
_key_to_freebsd(key_t key)
{
    return (long)(__uint32_t)key;
}

static inline void
_ipc_perm_from_freebsd(struct ipc_perm *perm, const struct __freebsd_ipc_perm *k)
{
    perm->uid = k->uid;
    perm->gid = k->gid;
    perm->cuid = k->cuid;
    perm->cgid = k->cgid;
    perm->mode = k->mode;
}

/* The IPC_STAT, IPC_SET and IPC_RMID commands take no flags */
#define FREEBSD_IPC_64 0

/* The kernel's times are time_t */
#define _ipc_time(k, name) ((time_t)(k)->name)

/* IPC_SET only changes the owner and the permission bits */
static inline void
_ipc_perm_to_freebsd(struct __freebsd_ipc_perm *k, const struct ipc_perm *perm)
{
    k->uid = perm->uid;
    k->gid = perm->gid;
    k->mode = (__uint16_t)perm->mode;
}

#endif /* _LOCAL_IPC_H_ */
