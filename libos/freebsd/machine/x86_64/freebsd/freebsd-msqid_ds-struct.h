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

#ifndef _FREEBSD_MSQID_DS_STRUCT_H_
#define _FREEBSD_MSQID_DS_STRUCT_H_

struct __freebsd_msqid_ds {
    struct __freebsd_ipc_perm msg_perm;
    __uint8_t                 __adjust_32[16];
    unsigned long             msg_cbytes;
    unsigned long             msg_qnum;
    unsigned long             msg_qbytes;
    int                       msg_lspid;
    int                       msg_lrpid;
    __int64_t                 msg_stime;
    __int64_t                 msg_rtime;
    __int64_t                 msg_ctime;
};

_Static_assert(sizeof(struct __freebsd_msqid_ds) == 104, "struct msqid_ds size");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_perm) == 0,
               "struct msqid_ds.msg_perm");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_cbytes) == 48,
               "struct msqid_ds.msg_cbytes");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_qnum) == 56,
               "struct msqid_ds.msg_qnum");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_qbytes) == 64,
               "struct msqid_ds.msg_qbytes");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_lspid) == 72,
               "struct msqid_ds.msg_lspid");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_lrpid) == 76,
               "struct msqid_ds.msg_lrpid");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_stime) == 80,
               "struct msqid_ds.msg_stime");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_rtime) == 88,
               "struct msqid_ds.msg_rtime");
_Static_assert(__builtin_offsetof(struct __freebsd_msqid_ds, msg_ctime) == 96,
               "struct msqid_ds.msg_ctime");

#endif /* _FREEBSD_MSQID_DS_STRUCT_H_ */
