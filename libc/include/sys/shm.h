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

#ifndef _SYS_SHM_H_
#define _SYS_SHM_H_

#include <sys/cdefs.h>
#include <sys/ipc.h>

_BEGIN_STD_C

#define SHM_RDONLY 010000 /* Attach read-only (else read-write) */
#define SHM_RND    020000 /* Round attach address to SHMLBA */

/* Segment low boundary address multiple; ARM Linux needs four pages */
#ifdef __arm__
#define SHMLBA 16384
#else
#define SHMLBA 4096
#endif

typedef unsigned long shmatt_t;

struct shmid_ds {
    struct ipc_perm shm_perm;   /* Operation permission structure */
    size_t          shm_segsz;  /* Size of segment in bytes */
    pid_t           shm_lpid;   /* Process ID of last shared memory operation */
    pid_t           shm_cpid;   /* Process ID of creator */
    shmatt_t        shm_nattch; /* Number of current attaches */
    time_t          shm_atime;  /* Time of last shmat() */
    time_t          shm_dtime;  /* Time of last shmdt() */
    time_t          shm_ctime;  /* Time of last change by shmctl() */
};

void *shmat(int, const void *, int) __picolibc_export;
int   shmctl(int, int, struct shmid_ds *) __picolibc_export;
int   shmdt(const void *) __picolibc_export;
int   shmget(key_t, size_t, int) __picolibc_export;

_END_STD_C

#endif /* _SYS_SHM_H_ */
