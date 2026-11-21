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

#ifndef _FREEBSD_SEMID_DS_STRUCT_H_
#define _FREEBSD_SEMID_DS_STRUCT_H_

struct __freebsd_semid_ds {
    struct __freebsd_ipc_perm sem_perm;
    __uint8_t                 __adjust_32[8];
    unsigned short            sem_nsems;
    __uint8_t                 __adjust_42[6];
    __int64_t                 sem_otime;
    __int64_t                 sem_ctime;
};

_Static_assert(sizeof(struct __freebsd_semid_ds) == 64, "struct semid_ds size");
_Static_assert(__builtin_offsetof(struct __freebsd_semid_ds, sem_perm) == 0,
               "struct semid_ds.sem_perm");
_Static_assert(__builtin_offsetof(struct __freebsd_semid_ds, sem_nsems) == 40,
               "struct semid_ds.sem_nsems");
_Static_assert(__builtin_offsetof(struct __freebsd_semid_ds, sem_otime) == 48,
               "struct semid_ds.sem_otime");
_Static_assert(__builtin_offsetof(struct __freebsd_semid_ds, sem_ctime) == 56,
               "struct semid_ds.sem_ctime");

#endif /* _FREEBSD_SEMID_DS_STRUCT_H_ */
