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

/* What <sys/shm.h> provides */

#define _POSIX_C_SOURCE 202405L
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include <sys/shm.h>

#define same_type(a, T)       _Generic((a), T: 1, default: 0)
#define member_is(type, m, T) same_type(((type *)0)->m, T)

/* The types, without any other header */
_Static_assert((pid_t)-1 < 0, "pid_t");
_Static_assert((size_t)-1 > 0, "size_t");
_Static_assert((time_t)1 / 2 == 0 || (time_t)1 / 2 > 0, "time_t");
/* glibc 2.43 doesn't define intptr_t here (new in POSIX.1-2024) */
#ifndef __GLIBC__
_Static_assert((intptr_t)-1 < 0 && sizeof(intptr_t) >= sizeof(void *), "intptr_t");
#endif

/* shmatt_t is unsigned and holds at least an unsigned short */
_Static_assert((shmatt_t)-1 > 0 && (shmatt_t)-1 >= (unsigned short)-1, "shmatt_t");

/* The flags are distinct, and apart from the permission bits */
_Static_assert(SHM_RDONLY != 0 && SHM_RND != 0 && (SHM_RDONLY & SHM_RND) == 0
                   && ((SHM_RDONLY | SHM_RND) & 0777) == 0,
               "SHM_RDONLY and SHM_RND");

/* SHM_FAILED is ((void *)(intptr_t)-1); new in POSIX.1-2024 */
#ifdef SHM_FAILED
_Static_assert(same_type(SHM_FAILED, void *), "SHM_FAILED is a pointer");
#endif

/* struct shmid_ds */
_Static_assert(member_is(struct shmid_ds, shm_perm, struct ipc_perm)
                   && member_is(struct shmid_ds, shm_segsz, size_t)
                   && member_is(struct shmid_ds, shm_lpid, pid_t)
                   && member_is(struct shmid_ds, shm_cpid, pid_t)
                   && member_is(struct shmid_ds, shm_nattch, shmatt_t)
                   && member_is(struct shmid_ds, shm_atime, time_t)
                   && member_is(struct shmid_ds, shm_dtime, time_t)
                   && member_is(struct shmid_ds, shm_ctime, time_t),
               "struct shmid_ds members");

/* <sys/shm.h> includes <sys/ipc.h> */
_Static_assert(IPC_CREAT != IPC_EXCL, "<sys/ipc.h> constants");
_Static_assert(same_type(&ftok, key_t (*)(const char *, int)), "ftok");

/* The prototypes */
_Static_assert(same_type(&shmat, void *(*)(int, const void *, int)), "shmat");
_Static_assert(same_type(&shmctl, int (*)(int, int, struct shmid_ds *)), "shmctl");
_Static_assert(same_type(&shmdt, int (*)(const void *)), "shmdt");
_Static_assert(same_type(&shmget, int (*)(key_t, size_t, int)), "shmget");

#include <stdio.h>

int
main(void)
{
    /* glibc 2.43 doesn't have SHM_FAILED */
#ifdef SHM_FAILED
    if (SHM_FAILED != (void *)(intptr_t)-1) {
        printf("SHM_FAILED is %p\n", SHM_FAILED);
        return 1;
    }
#elif !defined(__GLIBC__)
    printf("no SHM_FAILED\n");
    return 1;
#endif
    /* SHMLBA is positive */
    if (SHMLBA <= 0) {
        printf("SHMLBA %ld\n", (long)SHMLBA);
        return 1;
    }
    printf("success\n");
    return 0;
}
