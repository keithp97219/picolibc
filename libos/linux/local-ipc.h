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

#include "local-linux.h"
#include <sys/ipc.h>

/*
 * The control calls use the kernel's 64-bit structures. 32-bit ARM's
 * direct system calls parse the structure version from this flag in
 * the command; elsewhere they always use those structures, and the
 * flag makes the command invalid.
 */
#ifdef __arm__
#define LINUX_IPC_64 0x100
#else
#define LINUX_IPC_64 0
#endif

/*
 * The kernel's 64-bit IPC structures. 32-bit targets split each time
 * into low and high words; the mode is 16 bits on i386 and arm.
 */
#if defined(__i386__) || defined(__arm__)
typedef unsigned short __kernel_ipc_mode_t;
#else
typedef unsigned int __kernel_ipc_mode_t;
#endif

struct __kernel_ipc64_perm {
    __int32_t           key;
    __uint32_t          uid;
    __uint32_t          gid;
    __uint32_t          cuid;
    __uint32_t          cgid;
    __kernel_ipc_mode_t mode;
    unsigned char       __pad1[4 - sizeof(__kernel_ipc_mode_t)];
    unsigned short      seq;
    unsigned short      __pad2;
    unsigned long       __unused1;
    unsigned long       __unused2;
};

#if __SIZEOF_LONG__ == 8
#define __KERNEL_IPC_TIME(name) long name
#else
#define __KERNEL_IPC_TIME(name) \
    unsigned long name;         \
    unsigned long name##_high
#endif

struct __kernel_msqid64_ds {
    struct __kernel_ipc64_perm msg_perm;
    __KERNEL_IPC_TIME(msg_stime);
    __KERNEL_IPC_TIME(msg_rtime);
    __KERNEL_IPC_TIME(msg_ctime);
    unsigned long msg_cbytes;
    unsigned long msg_qnum;
    unsigned long msg_qbytes;
    int           msg_lspid;
    int           msg_lrpid;
    unsigned long __unused4;
    unsigned long __unused5;
};

struct __kernel_semid64_ds {
    struct __kernel_ipc64_perm sem_perm;
    /* x86_64 pads each time; other 64-bit targets don't */
    __KERNEL_IPC_TIME(sem_otime);
#ifdef __x86_64__
    unsigned long __unused1;
#endif
    __KERNEL_IPC_TIME(sem_ctime);
#ifdef __x86_64__
    unsigned long __unused2;
#endif
    unsigned long sem_nsems;
    unsigned long __unused3;
    unsigned long __unused4;
};

struct __kernel_shmid64_ds {
    struct __kernel_ipc64_perm shm_perm;
    unsigned long              shm_segsz;
    __KERNEL_IPC_TIME(shm_atime);
    __KERNEL_IPC_TIME(shm_dtime);
    __KERNEL_IPC_TIME(shm_ctime);
    int           shm_cpid;
    int           shm_lpid;
    unsigned long shm_nattch;
    unsigned long __unused4;
    unsigned long __unused5;
};

#if __SIZEOF_LONG__ == 8
#define _ipc_time(k, name) ((time_t)(k)->name)
#else
#define _ipc_time(k, name) ((time_t)((__uint64_t)(k)->name##_high << 32 | (k)->name))
#endif

static inline void
_ipc_perm_from_linux(struct ipc_perm *perm, const struct __kernel_ipc64_perm *k)
{
    perm->uid = k->uid;
    perm->gid = k->gid;
    perm->cuid = k->cuid;
    perm->cgid = k->cgid;
    perm->mode = k->mode;
}

/* IPC_SET only changes the owner and the permission bits */
static inline void
_ipc_perm_to_linux(struct __kernel_ipc64_perm *k, const struct ipc_perm *perm)
{
    k->uid = perm->uid;
    k->gid = perm->gid;
    k->mode = (__kernel_ipc_mode_t)perm->mode;
}

#endif /* _LOCAL_IPC_H_ */
