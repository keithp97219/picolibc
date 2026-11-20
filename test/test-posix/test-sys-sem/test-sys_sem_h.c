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

/* What <sys/sem.h> provides */

#define _POSIX_C_SOURCE 202405L
/* System V IPC is XSI */
#define _XOPEN_SOURCE 800
#include <sys/sem.h>

#define same_type(a, T)       _Generic((a), T: 1, default: 0)
#define member_is(type, m, T) same_type(((type *)0)->m, T)

/* The types, without any other header */
_Static_assert((pid_t)-1 < 0, "pid_t");
_Static_assert((size_t)-1 > 0, "size_t");
_Static_assert((time_t)1 / 2 == 0 || (time_t)1 / 2 > 0, "time_t");

/* SEM_UNDO goes in sem_flg next to IPC_NOWAIT, so it fits in a short */
_Static_assert(SEM_UNDO != 0 && (SEM_UNDO & IPC_NOWAIT) == 0, "SEM_UNDO");
_Static_assert((short)SEM_UNDO == SEM_UNDO && (short)IPC_NOWAIT == IPC_NOWAIT,
               "the flags fit sem_flg");

/* The commands are distinct, from each other and from <sys/ipc.h>'s */
static const int commands[]
    = { GETNCNT, GETPID, GETVAL, GETALL, GETZCNT, SETVAL, SETALL, IPC_RMID, IPC_SET, IPC_STAT };
#if GETNCNT == GETPID || SEM_UNDO == 0
#error the constants can be used in #if
#endif

/* struct semid_ds; glibc 2.43's sem_nsems is an unsigned long */
#ifndef __GLIBC__
_Static_assert(member_is(struct semid_ds, sem_nsems, unsigned short), "sem_nsems");
#endif
_Static_assert(member_is(struct semid_ds, sem_perm, struct ipc_perm)
                   && member_is(struct semid_ds, sem_otime, time_t)
                   && member_is(struct semid_ds, sem_ctime, time_t),
               "struct semid_ds members");

/* struct sembuf */
_Static_assert(member_is(struct sembuf, sem_num, unsigned short)
                   && member_is(struct sembuf, sem_op, short)
                   && member_is(struct sembuf, sem_flg, short),
               "struct sembuf members");

/* <sys/sem.h> includes <sys/ipc.h> */
_Static_assert(IPC_CREAT != IPC_EXCL, "<sys/ipc.h> constants");
_Static_assert(same_type(&ftok, key_t (*)(const char *, int)), "ftok");

/* The prototypes */
_Static_assert(same_type(&semctl, int (*)(int, int, int, ...)), "semctl");
_Static_assert(same_type(&semget, int (*)(key_t, int, int)), "semget");
_Static_assert(same_type(&semop, int (*)(int, struct sembuf *, size_t)), "semop");

/* The application declares union semun; the header must not */
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
};

#include <stdio.h>

int
main(void)
{
    size_t i, j;

    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        for (j = 0; j < i; j++)
            if (commands[i] == commands[j]) {
                printf("commands %zu and %zu are both %d\n", j, i, commands[i]);
                return 1;
            }
    printf("success\n");
    return 0;
}
