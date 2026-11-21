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

#ifndef _LINUX_SHMID64_DS_STRUCT_H_
#define _LINUX_SHMID64_DS_STRUCT_H_

struct __kernel_shmid64_ds {
    struct __kernel_ipc64_perm shm_perm;
    __uint64_t                 shm_segsz;
    __int64_t                  shm_atime;
    __int64_t                  shm_dtime;
    __int64_t                  shm_ctime;
    __int32_t                  shm_cpid;
    __int32_t                  shm_lpid;
    __uint64_t                 shm_nattch;
    __uint8_t                  __adjust_96[16];
};

#define SIMPLE_MAP_SHMID64_DS(_t, _f)        \
    do {                                     \
        (_t)->shm_segsz = (_f)->shm_segsz;   \
        (_t)->shm_atime = (_f)->shm_atime;   \
        (_t)->shm_dtime = (_f)->shm_dtime;   \
        (_t)->shm_ctime = (_f)->shm_ctime;   \
        (_t)->shm_cpid = (_f)->shm_cpid;     \
        (_t)->shm_lpid = (_f)->shm_lpid;     \
        (_t)->shm_nattch = (_f)->shm_nattch; \
    } while (0)

#endif /* _LINUX_SHMID64_DS_STRUCT_H_ */
