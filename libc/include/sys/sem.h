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

#ifndef _SYS_SEM_H_
#define _SYS_SEM_H_

#include <sys/cdefs.h>
#include <sys/ipc.h>

_BEGIN_STD_C

#define SEM_UNDO 0x1000 /* Set up adjust on exit entry */

/* semctl commands */
#define GETPID  11 /* Get sempid */
#define GETVAL  12 /* Get semval */
#define GETALL  13 /* Get all cases of semval */
#define GETNCNT 14 /* Get semncnt */
#define GETZCNT 15 /* Get semzcnt */
#define SETVAL  16 /* Set semval */
#define SETALL  17 /* Set all cases of semval */

struct semid_ds {
    struct ipc_perm sem_perm;  /* Operation permission structure */
    unsigned short  sem_nsems; /* Number of semaphores in set */
    time_t          sem_otime; /* Last semop() time */
    time_t          sem_ctime; /* Last time changed by semctl() */
};

struct sembuf {
    unsigned short sem_num; /* Semaphore number */
    short          sem_op;  /* Semaphore operation */
    short          sem_flg; /* Operation flags */
};

int semctl(int, int, int, ...) __picolibc_export;
int semget(key_t, int, int) __picolibc_export;
int semop(int, struct sembuf *, size_t) __picolibc_export;

_END_STD_C

#endif /* _SYS_SEM_H_ */
