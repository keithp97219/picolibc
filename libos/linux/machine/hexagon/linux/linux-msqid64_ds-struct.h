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

#ifndef _LINUX_MSQID64_DS_STRUCT_H_
#define _LINUX_MSQID64_DS_STRUCT_H_

struct __kernel_msqid64_ds {
    struct __kernel_ipc64_perm msg_perm;
    __uint32_t                 msg_stime;
    __uint32_t                 msg_stime_high;
    __uint32_t                 msg_rtime;
    __uint32_t                 msg_rtime_high;
    __uint32_t                 msg_ctime;
    __uint32_t                 msg_ctime_high;
    __uint32_t                 msg_cbytes;
    __uint32_t                 msg_qnum;
    __uint32_t                 msg_qbytes;
    __int32_t                  msg_lspid;
    __int32_t                  msg_lrpid;
    __uint8_t                  __adjust_80[8];
};

#define SIMPLE_MAP_MSQID64_DS(_t, _f)                \
    do {                                             \
        (_t)->msg_stime = (_f)->msg_stime;           \
        (_t)->msg_stime_high = (_f)->msg_stime_high; \
        (_t)->msg_rtime = (_f)->msg_rtime;           \
        (_t)->msg_rtime_high = (_f)->msg_rtime_high; \
        (_t)->msg_ctime = (_f)->msg_ctime;           \
        (_t)->msg_ctime_high = (_f)->msg_ctime_high; \
        (_t)->msg_cbytes = (_f)->msg_cbytes;         \
        (_t)->msg_qnum = (_f)->msg_qnum;             \
        (_t)->msg_qbytes = (_f)->msg_qbytes;         \
        (_t)->msg_lspid = (_f)->msg_lspid;           \
        (_t)->msg_lrpid = (_f)->msg_lrpid;           \
    } while (0)

#endif /* _LINUX_MSQID64_DS_STRUCT_H_ */
